/* game_data_manager.cpp */

#include "../cm_enums.h"
#pragma region godot_includes
#include "../../../core/error/error_list.h"
#include "../../../core/io/json.h"
#pragma endregion godot_includes

#pragma region cm_includes
#include "game_data_manager.h"
#pragma endregion cm_includes

VectorHashMapPair<String, TableSpecification> GameDataManager::table_specifications;

VectorHashMapPair<String, GameDataEntry *> GameDataManager::core_entries_by_uuid;
VectorHashMapPair<String, LinkedList<GameDataEntry *>> GameDataManager::game_entries_by_uuid;

VectorHashMapPair<String, GameDataEntry *> GameDataManager::entries_by_path;

GameDataCollection GameDataManager::core_collection;
GameDataCollection GameDataManager::game_collection;
VectorHashMapPair<String, GameDataCollection> GameDataManager::mods_collections;
VectorHashMapPair<String, GameDataCollection> GameDataManager::tools_collections;

RWLock GameDataManager::data_lock;

RWLock GameDataManager::io_lock;
RWLock GameDataManager::validate_lock;

VectorHashMapPair<String, Dictionary> GameDataManager::data_to_be_validated;
VectorHashMapPair<String, GameDataManager::ThreadTask> GameDataManager::validation_tasks;
VectorHashMapPair<String, GameDataManager::ThreadTask> GameDataManager::io_tasks;

bool GameDataManager::cleaning_tasks = false;

////////////////////////////////////////////////////////////////////////////////////////////
// DataInfo
////////////////////////////////////////////////////////////////////////////////////////////
Dictionary DataInfo::to_dict() const
{
	Dictionary res;

	res["data_type"] = data_type;
	res["collection_type"] = collection_type;
	res["path"] = path;
	res["collection"] = collection;
	res["table"] = table;
	res["entry"] = entry;

	return res;
}

void DataInfo::from_dict(Dictionary p_dict)
{
	data_type = cm_enums::CM_DataType(int(p_dict["data_type"]));
	collection_type = cm_enums::CM_DataCollectionType(int(p_dict["collection_type"]));
	path = p_dict["path"];
	collection = p_dict["collection"];
	table = p_dict["table"];
	entry = p_dict["entry"];
	return;
}

////////////////////////////////////////////////////////////////////////////////////////////
// THREADED
////////////////////////////////////////////////////////////////////////////////////////////
void GameDataManager::_load_threaded(void *p_userdata)
{
	ThreadTask *thread_task = (ThreadTask *)p_userdata;
	DataInfo info;
	{
		io_lock.read_lock();
		info = thread_task->data_info;
		io_lock.read_unlock();
	}

	if (!FileAccess::exists(info.path))
	{
		io_lock.write_lock();
		thread_task->error = ERR_FILE_NOT_FOUND;
		io_lock.write_unlock();
		return;
	}

	if(info.data_type == cm_enums::CM_DataType::CM_DATA_TYPE_NONE)
	{
		io_lock.write_lock();
		thread_task->error = ERR_BUG;
		io_lock.write_unlock();
		return;
	}

	Dictionary collection_dict = dict_from_json(FileAccess::get_file_as_string(info.path));

	if (collection_dict.is_empty())
	{
		io_lock.write_lock();
		thread_task->error = ERR_QUERY_FAILED;
		io_lock.write_unlock();
		return;
	}

	Error validation_result = validate(info, collection_dict);
	if (validation_result != OK)
	{
		io_lock.write_lock();
		thread_task->error = ERR_INVALID_DATA;
		io_lock.write_unlock();
		return;
	}

	set_bind(info.to_dict(), collection_dict);

	return;
}


void GameDataManager::_save_threaded(void *p_userdata)
{
	ThreadTask *thread_task = (ThreadTask *)p_userdata;
	String resolve_group_id;
	DataInfo info;
	{
		io_lock.read_lock();
		info = thread_task->data_info;
		io_lock.read_unlock();
	}

	Dictionary collection_dict;

	if(info.data_type == cm_enums::CM_DataType::CM_DATA_TYPE_NONE)
	{
			io_lock.write_lock();
			thread_task->error = ERR_BUG;
			io_lock.write_unlock();
			return;
	}

	collection_dict = get_bind(info.to_dict());

	if (collection_dict.is_empty())
	{
		io_lock.write_lock();
		thread_task->error = ERR_QUERY_FAILED;
		io_lock.write_unlock();
		return;
	}

	Error validation_result = validate(info, collection_dict);
	if (validation_result != OK)
	{
		io_lock.write_lock();
		thread_task->error = ERR_INVALID_DATA;
		io_lock.write_unlock();
		return;
	}

	String collection_json = dict_to_json(collection_dict, false, true);

	Error err;
	Ref<FileAccess> file = FileAccess::open(info.path, FileAccess::WRITE, &err);
	file->store_string(collection_json);
	file->close();
	return;
}

void GameDataManager::_validate_threaded(void *p_userdata) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{

}

String GameDataManager::request_load(DataInfo p_data_info)
{
	if (!FileAccess::exists(p_data_info.path))
	{
		ERR_PRINT_ED("[ ERROR ] Path does not exist");
		return UUID::empty().get_uuid_string_bind();
	}

	io_lock.write_lock();

	UUID io_task_uuid = UUID();
	ThreadTask *t_task = io_tasks.create_one(io_task_uuid.get_uuid_string_bind());
	t_task->data_info = p_data_info;
	t_task->task_id = WorkerThreadPool::get_singleton()->add_native_task(&GameDataManager::_load_threaded, t_task);
	io_lock.write_unlock();

	return io_task_uuid.get_uuid_string_bind();
}

String GameDataManager::request_save(DataInfo p_data_info)
{
	io_lock.write_lock();

	UUID io_task_uuid = UUID();
	ThreadTask *t_task = io_tasks.create_one(io_task_uuid.get_uuid_string_bind());
	t_task->data_info = p_data_info;
	t_task->task_id = WorkerThreadPool::get_singleton()->add_native_task(&GameDataManager::_save_threaded, t_task);
	io_lock.write_unlock();

	return io_task_uuid.get_uuid_string_bind();
}

String GameDataManager::request_validation(DataInfo p_data_info, String p_data_uuid)
{
	// NOTE: instead of parsing the collection dict make a vector_hm_pair of <String (uuid), Dictionary (collection_dict)>
	// accessed under task_lock to avoid sending a shitload of data at once (if it matters at all, for now, leave as is)

	validate_lock.write_lock();

	UUID validation_task_uuid = UUID();

	Dictionary data = data_to_be_validated.get_one_const(p_data_uuid);

	ThreadTask *v_task = validation_tasks.create_one(validation_task_uuid.get_uuid_string_bind());
	v_task->data_info = p_data_info;
	v_task->data_uuid = validation_task_uuid;
	v_task->task_id = WorkerThreadPool::get_singleton()->add_native_task(&GameDataManager::_validate_threaded, v_task);
	validate_lock.write_unlock();

	return validation_task_uuid.get_uuid_string_bind();
}

String GameDataManager::request_validation(DataInfo p_data_info, Dictionary p_collection_dict)
{
	// NOTE: instead of parsing the collection dict make a vector_hm_pair of <String (uuid), Dictionary (collection_dict)>
	// accessed under task_lock to avoid sending a shitload of data at once (if it matters at all, for now, leave as is)

	validate_lock.write_lock();

	UUID validation_task_uuid = UUID();

	Dictionary *data = data_to_be_validated.create_one(validation_task_uuid.get_uuid_string_bind());
	*data = p_collection_dict;

	ThreadTask *v_task = validation_tasks.create_one(validation_task_uuid.get_uuid_string_bind());
	v_task->data_info = p_data_info;
	v_task->data_uuid = validation_task_uuid;
	v_task->task_id = WorkerThreadPool::get_singleton()->add_native_task(&GameDataManager::_validate_threaded, v_task);
	validate_lock.write_unlock();

	return validation_task_uuid.get_uuid_string_bind();
}


Error GameDataManager::get_io_task_status(String p_task_uuid) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{
	io_lock.read_lock();
	io_lock.read_unlock();
	return OK;
}

Error GameDataManager::get_validation_task_status(String p_task_uuid) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{
}

Error GameDataManager::complete_load(String p_task_uuid) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{
}

Error GameDataManager::complete_save(String p_task_uuid) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{
}

Error GameDataManager::complete_validation(String p_task_uuid) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{
}

Error GameDataManager::load(DataInfo p_data_info)
{
	if (!FileAccess::exists(p_data_info.path))
	{
		return ERR_FILE_NOT_FOUND;
	}

	if(p_data_info.data_type == cm_enums::CM_DataType::CM_DATA_TYPE_NONE)
	{
		return ERR_BUG;
	}

	Dictionary collection_dict = dict_from_json(FileAccess::get_file_as_string(p_data_info.path));

	if (collection_dict.is_empty())
	{
		return ERR_QUERY_FAILED;
	}

	Error validation_result = validate(p_data_info, collection_dict);
	if (validation_result != OK)
	{
		return ERR_INVALID_DATA;
	}

	set_bind(p_data_info.to_dict(), collection_dict);

	return OK;
}

Error GameDataManager::save(DataInfo p_data_info)
{
	Dictionary collection_dict;

	if(p_data_info.data_type == cm_enums::CM_DataType::CM_DATA_TYPE_NONE)
	{
		return ERR_BUG;
	}

	collection_dict = get_bind(p_data_info.to_dict());

	if (collection_dict.is_empty())
	{
		return ERR_QUERY_FAILED;
	}

	Error validation_result = validate(p_data_info, collection_dict);
	if (validation_result != OK)
	{
		return ERR_INVALID_DATA;
	}

	String collection_json = dict_to_json(collection_dict, false, true);

	Error err;
	Ref<FileAccess> file = FileAccess::open(p_data_info.path, FileAccess::WRITE, &err);
	file->store_string(collection_json);
	file->close();
	return OK;
}

Error GameDataManager::validate(DataInfo p_data_info, String p_data_uuid) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{
	return OK;
}

Error GameDataManager::validate(DataInfo p_data_info, Dictionary p_collection_dict) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{
	return OK;
}

bool GameDataManager::has(DataInfo p_data_info)
{
	bool res = false;

	switch (p_data_info.data_type)
	{
		case cm_enums::CM_DataType::CM_DATA_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - data type none");
			return false;
		case cm_enums::CM_DataType::CM_DATA_TYPE_COLLECTION:
			switch (p_data_info.collection_type)
			{
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
					ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
					return false;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
					if (p_data_info.collection != "CORE")
					{
						return false;
					}
					return true;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
					if (p_data_info.collection != "GAME")
					{
						return false;
					}
					return true;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
					data_lock.read_lock();
					res = mods_collections.has(p_data_info.collection);
					data_lock.read_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
					data_lock.read_lock();
					res = tools_collections.has(p_data_info.collection);
					data_lock.read_unlock();
					return res;
			}
		case cm_enums::CM_DataType::CM_DATA_TYPE_TABLE_SPECIFICATION:
			data_lock.read_lock();
			res = table_specifications.has(p_data_info.table);
			data_lock.read_unlock();
			return res;
		case cm_enums::CM_DataType::CM_DATA_TYPE_TABLE:
			switch (p_data_info.collection_type)
			{
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
					ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
					return false;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
					if (p_data_info.collection != "CORE")
					{
						return false;
					}
					data_lock.read_lock();
					res = core_collection.has(p_data_info.table);
					data_lock.read_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
					if (p_data_info.collection != "GAME")
					{
						return false;
					}
					data_lock.read_lock();
					res = game_collection.has(p_data_info.table);
					data_lock.read_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
					data_lock.read_lock();
					if (const GameDataCollection *collection_ptr = mods_collections.get_one_const(p_data_info.collection))
					{
						res = collection_ptr->has(p_data_info.table);
					}
					data_lock.read_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
					data_lock.read_lock();
					if (const GameDataCollection *collection_ptr = tools_collections.get_one_const(p_data_info.collection))
					{
						res = collection_ptr->has(p_data_info.table);
					}
					data_lock.read_unlock();
					return res;
			}
			return false;

		case cm_enums::CM_DataType::CM_DATA_TYPE_ENTRY:
			switch (p_data_info.collection_type)
			{
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
					ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
					return false;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
					data_lock.read_lock();
					res = core_entries_by_uuid.has(p_data_info.entry);
					data_lock.read_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
					data_lock.read_lock();
					res = game_entries_by_uuid.has(p_data_info.entry);
					data_lock.read_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
					data_lock.read_lock();
					if (GameDataCollection *collection_ptr = get_data_collection(p_data_info))
					{
						if (GameDataTable *table_ptr = collection_ptr->get_one(p_data_info.table))
						{
							res = table_ptr->has(p_data_info.entry);
						}
					}
					data_lock.read_unlock();
					return res;
			}
	}
}

Error GameDataManager::remove(DataInfo p_data_info)
{
	Error res = ERR_QUERY_FAILED;

	switch (p_data_info.data_type)
	{
		case cm_enums::CM_DataType::CM_DATA_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - data type none");
			return ERR_BUG;
		case cm_enums::CM_DataType::CM_DATA_TYPE_COLLECTION:
			switch (p_data_info.collection_type)
			{
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
					ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
					return ERR_BUG;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
					if (p_data_info.collection != "CORE")
					{
						ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection_type - collection mismatch");
						return ERR_BUG;
					}
					ERR_PRINT_ED("[ ERROR ] Can't delete core collection");
					return ERR_UNAVAILABLE;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
					if (p_data_info.collection != "GAME")
					{
						ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection_type - collection mismatch");
						return ERR_BUG;
					}
					ERR_PRINT_ED("[ ERROR ] Can't delete game collection");
					return ERR_UNAVAILABLE;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
					data_lock.write_lock();
					res = mods_collections.delete_one(p_data_info.collection) ? OK : ERR_QUERY_FAILED;
					data_lock.write_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
					data_lock.write_lock();
					res = tools_collections.delete_one(p_data_info.collection) ? OK : ERR_QUERY_FAILED;
					data_lock.write_unlock();
					return res;
			}
			break;
		case cm_enums::CM_DataType::CM_DATA_TYPE_TABLE_SPECIFICATION:
			return table_specifications.delete_one(p_data_info.table) ? OK : ERR_QUERY_FAILED;
		case cm_enums::CM_DataType::CM_DATA_TYPE_TABLE:
			switch (p_data_info.collection_type)
			{
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
					ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
					return ERR_BUG;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
					if (p_data_info.collection != "CORE")
					{
						ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection_type - collection mismatch");
						return ERR_BUG;
					}
					data_lock.write_lock();
					res = core_collection.delete_one(p_data_info.table) ? OK : ERR_QUERY_FAILED;
					data_lock.write_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
					if (p_data_info.collection != "GAME")
					{
						ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection_type - collection mismatch");
						return ERR_BUG;
					}
					data_lock.write_lock();
					res = game_collection.delete_one(p_data_info.table) ? OK : ERR_QUERY_FAILED;
					data_lock.write_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
					data_lock.write_lock();
					if (GameDataCollection *collection_ptr = mods_collections.get_one(p_data_info.collection))
					{
						res = collection_ptr->delete_one(p_data_info.table) ? OK : ERR_QUERY_FAILED;
					}
					data_lock.write_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
					data_lock.write_lock();
					if (GameDataCollection *collection_ptr = tools_collections.get_one(p_data_info.collection))
					{
						res = collection_ptr->delete_one(p_data_info.table) ? OK : ERR_QUERY_FAILED;
					}
					data_lock.write_unlock();
					return res;
			}
			break;

		case cm_enums::CM_DataType::CM_DATA_TYPE_ENTRY:
			switch (p_data_info.collection_type)
			{
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
					ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
					return ERR_BUG;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
					if (p_data_info.collection != "CORE")
					{
						ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection_type - collection mismatch");
						return ERR_BUG;
					}
					data_lock.write_lock();
					if (GameDataTable *table_ptr = core_collection.get_one(p_data_info.table))
					{
						res = table_ptr->delete_one(p_data_info.entry) ? OK : ERR_QUERY_FAILED;
					}
					data_lock.write_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
					if (p_data_info.collection != "GAME")
					{
						ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection_type - collection mismatch");
						return ERR_BUG;
					}
					data_lock.write_lock();
					if (GameDataTable *table_ptr = game_collection.get_one(p_data_info.table))
					{
						res = table_ptr->delete_one(p_data_info.entry) ? OK : ERR_QUERY_FAILED;
					}
					data_lock.write_unlock();
					return res;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
					data_lock.write_lock();
					if (GameDataCollection *collection_ptr = get_data_collection(p_data_info))
					{
						if (GameDataTable *table_ptr = collection_ptr->get_one(p_data_info.table))
						{
							res = table_ptr->delete_one(p_data_info.entry) ? OK : ERR_QUERY_FAILED;
						}
					}
					data_lock.write_unlock();
					return res;
			}
			break;
	}
	return ERR_QUERY_FAILED;
}

////////////////////////////////////////////////////////////////////////////////////////////
// INTERNAL - specific methods --- THESE DON'T HAVE MUTEXES
////////////////////////////////////////////////////////////////////////////////////////////
GameDataCollection *GameDataManager::get_data_collection(DataInfo p_data_info)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_COLLECTION)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return nullptr;
	}
	switch (p_data_info.collection_type)
	{
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
			return nullptr;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
			if (p_data_info.collection != "CORE")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return nullptr;
			}
			return &core_collection;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return nullptr;
			}
			return &game_collection;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
			return mods_collections.get_one(p_data_info.collection);
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
			return tools_collections.get_one(p_data_info.collection);
	}
}

Error GameDataManager::set_data_collection(DataInfo p_data_info, const GameDataCollection &p_data_collection)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_COLLECTION)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return ERR_BUG;
	}
	switch (p_data_info.collection_type)
	{
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
			return ERR_BUG;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
			if (p_data_info.collection != "CORE")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}
			data_lock.write_lock();
			core_collection.replace(p_data_collection);
			data_lock.write_unlock();
			return OK;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}
			data_lock.write_lock();
			game_collection.replace(p_data_collection);
			data_lock.write_unlock();
			return OK;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
		{
			data_lock.write_lock();
			GameDataCollection *collection_to_replace = mods_collections.get_one(p_data_info.collection);
			collection_to_replace->replace(p_data_collection);
			collection_to_replace = nullptr;
			data_lock.write_unlock();
			return OK;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
		{
			data_lock.write_lock();
			GameDataCollection *collection_to_replace = tools_collections.get_one(p_data_info.collection);
			collection_to_replace->replace(p_data_collection);
			collection_to_replace = nullptr;
			data_lock.write_unlock();
			return OK;
		}
	}
}

Error GameDataManager::merge_data_collection(DataInfo p_data_info, const GameDataCollection &p_data_collection)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_COLLECTION)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return ERR_BUG;
	}
	switch (p_data_info.collection_type)
	{
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
			return ERR_BUG;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
			if (p_data_info.collection != "CORE")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}
			data_lock.write_lock();
			if (core_collection.merge(p_data_collection))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}
			data_lock.write_lock();
			if (game_collection.merge(p_data_collection))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
		{
			data_lock.write_lock();
			GameDataCollection *collection_to_replace = mods_collections.get_one(p_data_info.collection);
			if (collection_to_replace->merge(p_data_collection))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
		{
			data_lock.write_lock();
			GameDataCollection *collection_to_replace = tools_collections.get_one(p_data_info.collection);
			if(collection_to_replace->merge(p_data_collection))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		}
	}
}

TableSpecification *GameDataManager::get_data_table_specification(DataInfo p_data_info)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_TABLE_SPECIFICATION)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return nullptr;
	}

	return table_specifications.get_one(p_data_info.table);
}

Error GameDataManager::set_data_table_specification(DataInfo p_data_info, const TableSpecification &p_table_specification)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_TABLE_SPECIFICATION)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return ERR_BUG;
	}

	data_lock.write_lock();

	table_specifications.delete_one(p_data_info.table);

	TableSpecification *new_table_spec = table_specifications.create_one(p_data_info.table);
	new_table_spec->name = p_table_specification.name;
	new_table_spec->path = p_table_specification.path;
	new_table_spec->fields = p_table_specification.fields;

	data_lock.write_unlock();
	return OK;
}

GameDataTable *GameDataManager::get_data_table(DataInfo p_data_info)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_TABLE)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return nullptr;
	}

	switch (p_data_info.collection_type)
	{
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
			return nullptr;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
		{
			if (p_data_info.collection != "CORE")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return nullptr;
			}

			return core_collection.get_one(p_data_info.table);
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
		{
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return nullptr;
			}

			return game_collection.get_one(p_data_info.table);
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
		{
			GameDataCollection *collection_ptr = mods_collections.get_one(p_data_info.collection);
			if (!collection_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No collection found that matches DataInfo");
				return nullptr;
			}

			return collection_ptr->get_one(p_data_info.table);
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
		{
			GameDataCollection *collection_ptr = tools_collections.get_one(p_data_info.collection);
			if (!collection_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No collection found that matches DataInfo");
				return nullptr;
			}

			return collection_ptr->get_one(p_data_info.table);
		}
	}
}

Error GameDataManager::set_data_table(DataInfo p_data_info, const GameDataTable &p_data_table)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_TABLE)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return ERR_BUG;
	}
	switch (p_data_info.collection_type)
	{
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
			return ERR_BUG;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
		{
			if (p_data_info.collection != "CORE")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}

			data_lock.write_lock();
			GameDataTable *table_ptr = core_collection.get_one(p_data_info.table);
			if (!table_ptr)
			{
				table_ptr = core_collection.create_one(p_data_info.table);
			}
			table_ptr->replace(p_data_table);
			data_lock.write_unlock();
			return OK;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
		{
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}

			data_lock.write_lock();
			GameDataTable *table_ptr = game_collection.get_one(p_data_info.table);
			if (!table_ptr)
			{
				table_ptr = game_collection.create_one(p_data_info.table);
			}
			table_ptr->replace(p_data_table);
			data_lock.write_unlock();
			return OK;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
		{
			data_lock.write_lock();
			GameDataCollection *collection_ptr = mods_collections.get_one(p_data_info.collection);
			if (!collection_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection not found");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataTable *table_ptr = collection_ptr->get_one(p_data_info.table);
			if (!table_ptr)
			{
				table_ptr = collection_ptr->create_one(p_data_info.table);
			}

			table_ptr->replace(p_data_table);
			data_lock.write_unlock();
			return OK;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
		{
			data_lock.write_lock();
			GameDataCollection *collection_ptr = tools_collections.get_one(p_data_info.collection);
			if (!collection_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection not found");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataTable *table_ptr = collection_ptr->get_one(p_data_info.table);
			if (!table_ptr)
			{
				table_ptr = collection_ptr->create_one(p_data_info.table);
			}

			table_ptr->replace(p_data_table);
			data_lock.write_unlock();
			return OK;
		}
	}
}

Error GameDataManager::merge_data_table(DataInfo p_data_info, const GameDataTable &p_data_table)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_TABLE)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return ERR_BUG;
	}
	switch (p_data_info.collection_type)
	{
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
			return ERR_BUG;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
		{
			if (p_data_info.collection != "CORE")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}

			data_lock.write_lock();
			GameDataTable *table_ptr = core_collection.get_one(p_data_info.table);
			if (!table_ptr)
			{
				table_ptr = core_collection.create_one(p_data_info.table);
			}
			if (table_ptr->merge(p_data_table))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
		{
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}

			data_lock.write_lock();
			GameDataTable *table_ptr = game_collection.get_one(p_data_info.table);
			if (!table_ptr)
			{
				table_ptr = game_collection.create_one(p_data_info.table);
			}
			if (table_ptr->merge(p_data_table))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
		{
			data_lock.write_lock();
			GameDataCollection *collection_ptr = mods_collections.get_one(p_data_info.collection);
			if (!collection_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection not found");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataTable *table_ptr = collection_ptr->get_one(p_data_info.table);
			if (!table_ptr)
			{
				table_ptr = collection_ptr->create_one(p_data_info.table);
			}

			if (table_ptr->merge(p_data_table))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
		{
			data_lock.write_lock();
			GameDataCollection *collection_ptr = tools_collections.get_one(p_data_info.collection);
			if (!collection_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection not found");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataTable *table_ptr = collection_ptr->get_one(p_data_info.table);
			if (!table_ptr)
			{
				table_ptr = collection_ptr->create_one(p_data_info.table);
			}

			if (table_ptr->merge(p_data_table))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		}
	}
}

GameDataEntry *GameDataManager::get_data_entry(DataInfo p_data_info)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_ENTRY)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return nullptr;
	}

	switch (p_data_info.collection_type)
	{
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
			return nullptr;
			break;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
		{

			if (!p_data_info.full_resolve)
			{
				return core_entries_by_uuid.get_one(p_data_info.entry);
			}

			if (p_data_info.collection != "CORE")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return nullptr;
			}

			GameDataTable *table_ptr = core_collection.get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				return nullptr;
			}

			return table_ptr->get_one(p_data_info.entry);
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
		{
			if (!p_data_info.full_resolve)
			{
				return game_entries_by_uuid.get_one(p_data_info.entry).head->value;
			}

			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return nullptr;
			}

			GameDataTable *table_ptr = game_collection.get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				return nullptr;
			}

			return table_ptr->get_one(p_data_info.entry);
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
		{
			if (!p_data_info.full_resolve)
			{
				return game_entries_by_uuid.get_one(p_data_info.entry).head->value;
			}

			GameDataCollection *collection_ptr = mods_collections.get_one(p_data_info.collection);
			if (!collection_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No collection found that matches DataInfo");
				return nullptr;
			}

			GameDataTable *table_ptr = collection_ptr->get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				return nullptr;
			}

			return table_ptr->get_one(p_data_info.entry);
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
		{
			GameDataCollection *collection_ptr = tools_collections.get_one(p_data_info.collection);
			if (!collection_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No collection found that matches DataInfo");
				return nullptr;
			}

			GameDataTable *table_ptr = collection_ptr->get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				return nullptr;
			}

			return table_ptr->get_one(p_data_info.entry);
		}
	}
}

Error GameDataManager::set_data_entry(DataInfo p_data_info, const GameDataEntry &p_data_entry)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_ENTRY)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return ERR_BUG;
	}

	switch (p_data_info.collection_type)
	{
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
			return ERR_BUG;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
		{
			if (p_data_info.collection != "CORE")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}

			data_lock.write_lock();
			GameDataTable *table_ptr = core_collection.get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataEntry *entry_ptr = table_ptr->get_one(p_data_info.entry);

			if (!entry_ptr)
			{
				entry_ptr = table_ptr->create_one(p_data_info.entry);
			}
			entry_ptr->replace(p_data_entry);

			// Handling quick search lists
			if (!entries_by_path.has(p_data_info.path))
			{
				GameDataEntry **path_list_entry = entries_by_path.create_one(p_data_info.path);
				*path_list_entry = entry_ptr;
			}

			data_lock.write_unlock();
			return OK;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
		{
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}

			data_lock.write_lock();
			GameDataTable *table_ptr = game_collection.get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataEntry *entry_ptr = table_ptr->get_one(p_data_info.entry);

			if (!entry_ptr)
			{
				entry_ptr = table_ptr->create_one(p_data_info.entry);
			}
			entry_ptr->replace(p_data_entry);

			// Handling quick search lists
			if (!entries_by_path.has(p_data_info.path))
			{
				GameDataEntry **path_list_entry = entries_by_path.create_one(p_data_info.path);
				*path_list_entry = entry_ptr;

				game_entries_by_uuid.get_one(p_data_info.entry).prepend(entry_ptr);
			}

			data_lock.write_unlock();
			return OK;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
		{
			data_lock.write_lock();
			GameDataTable *table_ptr = mods_collections.get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataEntry *entry_ptr = table_ptr->get_one(p_data_info.entry);

			if (!entry_ptr)
			{
				entry_ptr = table_ptr->create_one(p_data_info.entry);
			}
			entry_ptr->replace(p_data_entry);

			// Handling quick search lists
			if (!entries_by_path.has(p_data_info.path))
			{
				GameDataEntry **path_list_entry = entries_by_path.create_one(p_data_info.path);
				*path_list_entry = entry_ptr;

				game_entries_by_uuid.get_one(p_data_info.entry).prepend(entry_ptr);
			}

			data_lock.write_unlock();
			return OK;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
		{
			data_lock.write_lock();
			GameDataTable *table_ptr = tools_collections.get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataEntry *entry_ptr = table_ptr->get_one(p_data_info.entry);

			if (!entry_ptr)
			{
				entry_ptr = table_ptr->create_one(p_data_info.entry);
			}
			entry_ptr->replace(p_data_entry);

			// Handling quick search lists
			if (!entries_by_path.has(p_data_info.path))
			{
				GameDataEntry **path_list_entry = entries_by_path.create_one(p_data_info.path);
				*path_list_entry = entry_ptr;
			}

			data_lock.write_unlock();
			return OK;
		}
	}
}

Error GameDataManager::merge_data_entry(DataInfo p_data_info, const GameDataEntry &p_data_entry)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_ENTRY)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return ERR_BUG;
	}
	switch (p_data_info.collection_type)
	{
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type none");
			return ERR_BUG;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
		{
			if (p_data_info.collection != "CORE")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}

			data_lock.write_lock();
			GameDataTable *table_ptr = core_collection.get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataEntry *entry_ptr = table_ptr->get_one(p_data_info.entry);

			if (!entry_ptr)
			{
				entry_ptr = table_ptr->create_one(p_data_info.entry);
			}
			if (entry_ptr->merge(p_data_entry))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
		{
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}

			data_lock.write_lock();
			GameDataTable *table_ptr = game_collection.get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataEntry *entry_ptr = table_ptr->get_one(p_data_info.entry);

			if (!entry_ptr)
			{
				entry_ptr = table_ptr->create_one(p_data_info.entry);
			}
			if (entry_ptr->merge(p_data_entry))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
		{
			data_lock.write_lock();
			GameDataTable *table_ptr = mods_collections.get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataEntry *entry_ptr = table_ptr->get_one(p_data_info.entry);

			if (!entry_ptr)
			{
				entry_ptr = table_ptr->create_one(p_data_info.entry);
			}
			if (entry_ptr->merge(p_data_entry))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
		{
			data_lock.write_lock();
			GameDataTable *table_ptr = tools_collections.get_one(p_data_info.table);

			if (!table_ptr)
			{
				ERR_PRINT_ED("[ ERROR ] No table found that matches DataInfo");
				data_lock.write_unlock();
				return ERR_QUERY_FAILED;
			}

			GameDataEntry *entry_ptr = table_ptr->get_one(p_data_info.entry);

			if (!entry_ptr)
			{
				entry_ptr = table_ptr->create_one(p_data_info.entry);
			}
			if (entry_ptr->merge(p_data_entry))
			{
				data_lock.write_unlock();
				return OK;
			}
			data_lock.write_unlock();
			return ERR_CANT_RESOLVE;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC BINDS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
String GameDataManager::request_load_bind(Dictionary p_data_info)
{
	DataInfo data_info;
	data_info.from_dict(p_data_info);
	return request_load(data_info);
}

String GameDataManager::request_save_bind(Dictionary p_data_info)
{

	DataInfo data_info;
	data_info.from_dict(p_data_info);
	return request_save(data_info);
}

bool GameDataManager::has_bind(Dictionary p_data_info)
{

	DataInfo data_info;
	data_info.from_dict(p_data_info);
	return has(data_info);
}

Dictionary GameDataManager::get_bind(Dictionary p_data_info)
{
	DataInfo data_info;
	data_info.from_dict(p_data_info);

	Dictionary res;

	switch(data_info.data_type)
	{
		case cm_enums::CM_DataType::CM_DATA_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
			return Dictionary();

		case cm_enums::CM_DataType::CM_DATA_TYPE_TABLE_SPECIFICATION:
		{
			data_lock.read_lock();
			TableSpecification *t_spec = get_data_table_specification(data_info);
			if (!t_spec)
			{
				ERR_PRINT_ED("[ ERROR ] No table found for DataInfo");
				return Dictionary();
			}
			res = t_spec->to_dict();
			t_spec = nullptr;
			data_lock.read_unlock();
			return res;
		}
		case cm_enums::CM_DataType::CM_DATA_TYPE_COLLECTION:
		{
			data_lock.read_lock();
			GameDataCollection *collection = get_data_collection(data_info);
			if(!collection)
			{
				ERR_PRINT_ED("[ ERROR ] No collection found for DataInfo");
				return Dictionary();
			}
			res = collection->to_dict();
			collection = nullptr;
			data_lock.read_unlock();
			return res;
		}
		case cm_enums::CM_DataType::CM_DATA_TYPE_TABLE:
		{
			data_lock.read_lock();
			GameDataTable *table = get_data_table(data_info);
			if (!table)
			{
				ERR_PRINT_ED("[ ERROR ] No table found for DataInfo");
				return Dictionary();
			}
			res = table->to_dict();
			table = nullptr;
			data_lock.read_unlock();
			return res;
		}
		case cm_enums::CM_DataType::CM_DATA_TYPE_ENTRY:
		{
			data_lock.read_lock();
			GameDataEntry *entry = get_data_entry(data_info);
			if (!entry)
			{
				ERR_PRINT_ED("[ ERROR ] No entry found for DataInfo");
				return Dictionary();
			}

			res = entry->to_dict();
			entry = nullptr;
			data_lock.read_unlock();
			return res;
		}
	}
}

Error GameDataManager::set_bind(Dictionary p_data_info, Dictionary p_data_dict)
{
	DataInfo data_info;
	data_info.from_dict(p_data_info);

	switch(data_info.data_type)
	{
		case cm_enums::CM_DataType::CM_DATA_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
			return ERR_BUG;

		case cm_enums::CM_DataType::CM_DATA_TYPE_TABLE_SPECIFICATION:
		{
			TableSpecification t_spec;
			t_spec.from_dict(p_data_dict);
			return set_data_table_specification(data_info, t_spec);
		}
		case cm_enums::CM_DataType::CM_DATA_TYPE_COLLECTION:
		{
			GameDataCollection collection;
			collection.from_dict(p_data_dict);
			return set_data_collection(data_info, collection);
		}
		case cm_enums::CM_DataType::CM_DATA_TYPE_TABLE:
		{
			GameDataTable table;
			table.from_dict(p_data_dict);
			return set_data_table(data_info, table);
		}
		case cm_enums::CM_DataType::CM_DATA_TYPE_ENTRY:
		{
			GameDataEntry entry;
			entry.from_dict(p_data_dict);
			return set_data_entry(data_info, entry);
		}
	}
}

Error GameDataManager::merge_bind(Dictionary p_data_info, Dictionary p_data_dict)
{
	DataInfo data_info;
	data_info.from_dict(p_data_info);

	switch(data_info.data_type)
	{
		case cm_enums::CM_DataType::CM_DATA_TYPE_NONE:
			ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
			return ERR_BUG;

		case cm_enums::CM_DataType::CM_DATA_TYPE_TABLE_SPECIFICATION:
		{
			TableSpecification t_spec;
			t_spec.from_dict(p_data_dict);
			return set_data_table_specification(data_info, t_spec);
		}
		case cm_enums::CM_DataType::CM_DATA_TYPE_COLLECTION:
		{
			GameDataCollection collection;
			collection.from_dict(p_data_dict);
			return merge_data_collection(data_info, collection);
		}
		case cm_enums::CM_DataType::CM_DATA_TYPE_TABLE:
		{
			GameDataTable table;
			table.from_dict(p_data_dict);
			return merge_data_table(data_info, table);
		}
		case cm_enums::CM_DataType::CM_DATA_TYPE_ENTRY:
		{
			GameDataEntry entry;
			entry.from_dict(p_data_dict);
			return merge_data_entry(data_info, entry);
		}
	}
}

Error GameDataManager::remove_bind(Dictionary p_data_info)
{
	DataInfo data_info;
	data_info.from_dict(p_data_info);
	return remove(data_info);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UTILITY
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
Dictionary GameDataManager::dict_from_json(String p_json_string)
{
	Ref<JSON > json;
	json.instantiate();
	Error err = json->parse(p_json_string, false);
	if (err != OK)
	{
		_err_print_error("", "", json->get_error_line(), json->get_error_message().utf8().get_data(), false, ERR_HANDLER_SCRIPT);
		return Dictionary();
	}

	return json->get_data();
}

String GameDataManager::dict_to_json(Dictionary p_dict, bool p_sort_keys, bool p_full_precision)
{
	return JSON::stringify(Variant(p_dict), "\t", p_sort_keys, p_full_precision);
}

void GameDataManager::initialize() {}

void GameDataManager::finalize() {}
