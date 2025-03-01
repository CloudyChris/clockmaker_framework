/* game_data_manager.cpp */

#include "core/error/error_list.h"
#pragma region godot_includes
#include "core/io/json.h"
#pragma endregion godot_includes

#pragma region cm_includes
#include "game_data_manager.h"
#pragma endregion cm_includes

VectorHashMapPair<String, TableSpecification> GameDataManager::table_specifications;

VectorHashMapPair<String, GameDataEntry *> GameDataManager::core_entries_by_uuid;
VectorHashMapPair<String, GameDataEntry *> GameDataManager::game_entries_by_uuid;

VectorHashMapPair<String, GameDataEntry *> GameDataManager::entries_by_path;

GameDataCollection GameDataManager::core_collection;
GameDataCollection GameDataManager::game_collection;
VectorHashMapPair<String, GameDataCollection> GameDataManager::mods_collections;
VectorHashMapPair<String, GameDataCollection> GameDataManager::tools_collections;

RWLock GameDataManager::tasks_lock;
RWLock GameDataManager::data_lock;

VectorHashMapPair<String, GameDataManager::ValidationTask> GameDataManager::validation_tasks;
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
		tasks_lock.read_lock();
		info = thread_task->data_info;
		tasks_lock.read_unlock();
	}

	if (!FileAccess::exists(info.path))
	{
		tasks_lock.write_lock();
		thread_task->error = ERR_FILE_NOT_FOUND;
		tasks_lock.write_unlock();
		return;
	}

	if(info.data_type == cm_enums::CM_DataType::CM_DATA_TYPE_NONE)
	{
		tasks_lock.write_lock();
		thread_task->error = ERR_BUG;
		tasks_lock.write_unlock();
		return;
	}

	Dictionary collection_dict = dict_from_json(FileAccess::get_file_as_string(info.path));

	if (collection_dict.is_empty())
	{
		tasks_lock.write_lock();
		thread_task->error = ERR_QUERY_FAILED;
		tasks_lock.write_unlock();
		return;
	}

	// TODO instead of directly setting this, put it in resolve_group::unvalidated
	// and add validation task
	set_bind(info.to_dict(), collection_dict);

	return;
}


void GameDataManager::_save_threaded(void *p_userdata)
{
	ThreadTask *thread_task = (ThreadTask *)p_userdata;
	String resolve_group_id;
	DataInfo info;
	{
		tasks_lock.read_lock();
		info = thread_task->data_info;
		tasks_lock.read_unlock();
	}

	Dictionary collection_dict;

	if(info.data_type == cm_enums::CM_DataType::CM_DATA_TYPE_NONE)
	{
			tasks_lock.write_lock();
			thread_task->error = ERR_BUG;
			tasks_lock.write_unlock();
			return;
	}

	collection_dict = get_bind(info.to_dict());

	if (collection_dict.is_empty())
	{
		tasks_lock.write_lock();
		thread_task->error = ERR_QUERY_FAILED;
		tasks_lock.write_unlock();
		return;
	}

	// TODO validate the dict, at some point
	// and add user preference for it

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

	// TODO validate p_data_info thoroughly

	tasks_lock.write_lock();

	UUID io_task_uuid = UUID();
	ThreadTask *t_task = io_tasks.create_one(io_task_uuid.get_uuid_string_bind());
	t_task->data_info = p_data_info;
	t_task->task_id = WorkerThreadPool::get_singleton()->add_native_task(&GameDataManager::_load_threaded, t_task);
	tasks_lock.write_unlock();

	return io_task_uuid.get_uuid_string_bind();
}

String GameDataManager::request_save(DataInfo p_data_info)
{
	// TODO validate p_data_info thoroughly

	tasks_lock.write_lock();

	UUID io_task_uuid = UUID();
	ThreadTask *t_task = io_tasks.create_one(io_task_uuid.get_uuid_string_bind());
	t_task->data_info = p_data_info;
	t_task->task_id = WorkerThreadPool::get_singleton()->add_native_task(&GameDataManager::_save_threaded, t_task);
	tasks_lock.write_unlock();

	return io_task_uuid.get_uuid_string_bind();
}

String GameDataManager::request_validation(DataInfo p_data_info, Dictionary p_collection_dict) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{
}


Error GameDataManager::get_io_task_status(String p_task_uuid) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{
	tasks_lock.read_lock();
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

	// TODO instead of directly setting this, put it in resolve_group::unvalidated
	// and add validation task
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

	// TODO validate the dict, at some point
	// and add user preference for it

	String collection_json = dict_to_json(collection_dict, false, true);

	Error err;
	Ref<FileAccess> file = FileAccess::open(p_data_info.path, FileAccess::WRITE, &err);
	file->store_string(collection_json);
	file->close();
	return OK;
}

Error GameDataManager::validate(DataInfo p_data_info) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{

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
					if (GameDataCollection *collection_ptr = get_collection(p_data_info))
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
	Error res = ERR_UNCONFIGURED;

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
			return ERR_QUERY_FAILED;

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

					return ERR_QUERY_FAILED;
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

					return ERR_QUERY_FAILED;
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
				case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
					data_lock.write_lock();
					if (GameDataCollection *collection_ptr = get_collection(p_data_info))
					{
						if (GameDataTable *table_ptr = collection_ptr->get_one(p_data_info.table))
						{
							res = table_ptr->delete_one(p_data_info.entry) ? OK : ERR_QUERY_FAILED;
						}
					}
					data_lock.write_unlock();
					return res != ERR_UNCONFIGURED ? res : ERR_QUERY_FAILED;
			}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// INTERNAL - specific methods --- THESE DON'T HAVE MUTEXES
////////////////////////////////////////////////////////////////////////////////////////////
GameDataCollection *GameDataManager::get_collection(DataInfo p_data_info)
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
			break;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_CORE:
			if (p_data_info.collection != "CORE")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return nullptr;
			}
			return &core_collection;
			break;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return nullptr;
			}
			return &game_collection;
			break;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
			return mods_collections.get_one(p_data_info.collection);
			break;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
			return tools_collections.get_one(p_data_info.collection);
	}
}

Error GameDataManager::set_collection(DataInfo p_data_info, const GameDataCollection &p_data_collection)
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
			core_collection.replace(p_data_collection);
			return OK;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}
			game_collection.replace(p_data_collection);
			return OK;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
		{
			GameDataCollection *collection_to_replace = mods_collections.get_one(p_data_info.collection);
			collection_to_replace->replace(p_data_collection);
			collection_to_replace = nullptr;
			return OK;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
		{
			GameDataCollection *collection_to_replace = tools_collections.get_one(p_data_info.collection);
			collection_to_replace->replace(p_data_collection);
			collection_to_replace = nullptr;
			return OK;
		}
	}
}

Error GameDataManager::merge_collection(DataInfo p_data_info, const GameDataCollection &p_data_collection)
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
			if (core_collection.merge(p_data_collection))
			{
				return OK;
			}
			return ERR_CANT_RESOLVE;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
			if (p_data_info.collection != "GAME")
			{
				ERR_PRINT_ED("[ ERROR ] Bad DataInfo - collection type-name mismatch");
				return ERR_BUG;
			}
			if (game_collection.merge(p_data_collection))
			{
				return OK;
			}
			return ERR_CANT_RESOLVE;
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
		{
			GameDataCollection *collection_to_replace = mods_collections.get_one(p_data_info.collection);
			if (collection_to_replace->merge(p_data_collection))
			{
				return OK;
			}
			return ERR_CANT_RESOLVE;
		}
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
		{
			GameDataCollection *collection_to_replace = tools_collections.get_one(p_data_info.collection);
			if(collection_to_replace->merge(p_data_collection))
			{
				return OK;
			}
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
	table_specifications.delete_one(p_data_info.table);

	TableSpecification *new_table_spec = table_specifications.create_one(p_data_info.table);
	new_table_spec->name = p_table_specification.name;
	new_table_spec->path = p_table_specification.path;
	new_table_spec->fields = p_table_specification.fields;

	return OK;
}

GameDataTable *GameDataManager::get_data_table(DataInfo p_data_info)
{
	if (p_data_info.data_type != cm_enums::CM_DataType::CM_DATA_TYPE_TABLE)
	{
		ERR_PRINT_ED("[ ERROR ] Bad DataInfo - wrong data_type");
		return nullptr;
	}

	GameDataCollection *collection_ptr = get_collection(p_data_info);
	if (!collection_ptr)
	{
		ERR_PRINT_ED("[ ERROR ] No collection found that matches DataInfo");
		return nullptr;
	}

	return collection_ptr->get_one(p_data_info.table);
}

Error GameDataManager::set_data_table(DataInfo p_data_info, const GameDataTable &p_data_table) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{

}

Error GameDataManager::merge_data_table(DataInfo p_data_info, const GameDataTable &p_data_table) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{

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
			return *core_entries_by_uuid.get_one(p_data_info.entry);
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_GAME:
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_MODS:
			return *game_entries_by_uuid.get_one(p_data_info.entry);
		case cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_TOOL:
			GameDataCollection *collection_ptr = get_collection(p_data_info);
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

Error GameDataManager::set_data_entry(DataInfo p_data_info, const GameDataEntry &p_data_entry) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{

}

Error GameDataManager::merge_data_entry(DataInfo p_data_info, const GameDataEntry &p_data_entry) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{

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
			data_lock.read_unlock();
			return res;
		}
		case cm_enums::CM_DataType::CM_DATA_TYPE_COLLECTION:
		{
			data_lock.read_lock();
			GameDataCollection *collection = get_collection(data_info);
			if(!collection)
			{
				ERR_PRINT_ED("[ ERROR ] No collection found for DataInfo");
				return Dictionary();
			}
			res = collection->to_dict();
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
			data_lock.read_unlock();
			return res;
		}
	}
}

Error GameDataManager::set_bind(Dictionary p_data_info, Dictionary p_data_dict) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{

}

Error GameDataManager::merge_bind(Dictionary p_data_info, Dictionary p_data_dict) // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
{

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
