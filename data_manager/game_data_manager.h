/* game_data_manager.h */

#ifndef GAME_DATA_MANAGER_H
#define GAME_DATA_MANAGER_H

#pragma region godot_includes
#include "../../../core/string/ustring.h"
#include "../../../core/error/error_list.h"
#include "../../../core/object/worker_thread_pool.h"
#include "../../../core/os/thread.h"
#pragma endregion godot_includes

#pragma region cm_includes
#include "game_data_collections.h"
#include "game_data_specifications.h"
#include "../vector_hashmap_pair.h"
#include "../linked_list.h"
#include "../cm_enums.h"
#pragma endregion cm_includes

// TODO: gotta implement binary serialization for data files
// and add a toggle so that for development you can use text and for release you can use bin

struct DataInfo
{
	cm_enums::CM_DataType data_type = cm_enums::CM_DataType::CM_DATA_TYPE_NONE;
	cm_enums::CM_DataCollectionType collection_type = cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE;
	String path = "";
	String collection = "";
	String table = "";
	String entry = UUID::empty().get_uuid_string_bind();
	bool full_resolve = false;

	Dictionary to_dict() const;
	void from_dict(Dictionary p_dict);
};

class GameDataManager
{
	friend GameDataCollection;
	friend GameDataTable;
	friend GameDataEntry;
public:

	struct ThreadTask
	{
		WorkerThreadPool::TaskID task_id = 0;
		Error error = ERR_UNCONFIGURED;
		DataInfo data_info;
		String data_uuid = UUID::empty().get_uuid_string_bind();
	};

private:
	// These define data types. Used for validation, tools ui, and they also serve as type reservations.
	static VectorHashMapPair<String, TableSpecification> table_specifications;

	// Core entries cannot be overriden
	static VectorHashMapPair<String, GameDataEntry *> core_entries_by_uuid;

	// Game entries can be overriden by mods
	// Overrides happen in order, overriden resources will be unloaded
	static VectorHashMapPair<String, LinkedList<GameDataEntry *>> game_entries_by_uuid;

	// Paths at least should be unique... so we only need this
	static VectorHashMapPair<String, GameDataEntry *> entries_by_path;

	// core tables: settings, saves, ui, etc
	static GameDataCollection core_collection;

	// game data tables
	static GameDataCollection game_collection;

	// Mods can load their own collections (mod collections will be named with the mod uuid)
	static VectorHashMapPair<String, GameDataCollection> mods_collections;

	// Tools (separate container for editing mods using the builtin tools)
	static VectorHashMapPair<String, GameDataCollection> tools_collections;

	// Mutexes
	// for data getters and setters
	// TODO make a differential locking logic since this is stupid
	// locking the whole db whenever you're modifying one lil thing is stupid
	// don't resize the vector in the vhmp every operation to be able to use it as cow data, as god intended
	// and only resize it on downtime, allowing finegrained locking on each layer (collection, table, entity, field)
	// to enable a proper interface with gdscript that holds a pointer to the resource needed that has its own lock
	static RWLock data_lock;

	// Resolve groups, and cleaning tasks bool
	static RWLock io_lock;
	static RWLock validate_lock;

	// to allow for defered loads
	static VectorHashMapPair<String, Dictionary> data_to_be_validated;
	static VectorHashMapPair<String, ThreadTask> validation_tasks;

	static VectorHashMapPair<String, ThreadTask> io_tasks;

	// to notify to drop/not process load tasks specifically. Saves are always processed
	static bool cleaning_tasks;

private:
	// Threaded stuff to run on worker thread pool
	static void _load_threaded(void *p_userdata);
	static void _save_threaded(void *p_userdata);
	static void _validate_threaded(void *p_userdata); // TODO

	// Load & Save data - these launch the threaded methods
	static String request_load(DataInfo p_data_info);
	static String request_save(DataInfo p_data_info);
	static String request_validation(DataInfo p_data_info, String p_data_uuid);
	static String request_validation(DataInfo p_data_info, Dictionary p_collection_dict);

	// Get the error code from io and validation task
	static Error get_io_task_status(String p_task_uuid); // TODO
	static Error get_validation_task_status(String p_task_uuid); // TODO

	// Wait for threaded task to finish NOW (blocking)
	static Error complete_load(String p_task_uuid); // TODO
	static Error complete_save(String p_task_uuid); // TODO
	static Error complete_validation(String p_task_uuid); // TODO

	// Non-threaded stuff for immediate loads (blocking)
	static Error load(DataInfo p_data_info);
	static Error save(DataInfo p_data_info);
	static Error validate(DataInfo p_data_info, String p_data_uuid); // TODO
	static Error validate(DataInfo p_data_info, Dictionary p_collection_dict); // TODO

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Private data getters and setters
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ALL
	static bool has(DataInfo p_data_info);
	static Error remove(DataInfo p_data_info);

	// ONLY SET AND MERGE HAVE LOCKS BELOW, GET DOESN"T
	// COLLECTION
	static GameDataCollection *get_data_collection(DataInfo p_data_info);
	static Error set_data_collection(DataInfo p_data_info, const GameDataCollection &p_data_collection);
	static Error merge_data_collection(DataInfo p_data_info, const GameDataCollection &p_data_collection);

	// TABLE SPECIFICATION
	static TableSpecification *get_data_table_specification(DataInfo p_data_info);
	static Error set_data_table_specification(DataInfo p_data_info, const TableSpecification &p_table_specification);

	// TABLE
	static GameDataTable *get_data_table(DataInfo p_data_info);
	static Error set_data_table(DataInfo p_data_info, const GameDataTable &p_data_table);
	static Error merge_data_table(DataInfo p_data_info, const GameDataTable &p_data_table);

	// ENTRY
	static GameDataEntry *get_data_entry(DataInfo p_data_info);
	static Error set_data_entry(DataInfo p_data_info, const GameDataEntry &p_data_entry);
	static Error merge_data_entry(DataInfo p_data_info, const GameDataEntry &p_data_entry);
	// NOTE: we should be able to manually specify whether we want a certain entry to remain on top of the stack

public:
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Load & Save Data (these launch threaded methods)
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// TODO *_bind should be renamed to *_dict
	static String request_load_bind(Dictionary p_data_info);
	static String request_save_bind(Dictionary p_data_info);
	static String request_validation_bind(Dictionary p_data_info);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Load & Save Data (these use the immediate, blocking methods)
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static Dictionary load_now_bind(Dictionary p_data_info);
	static Dictionary save_now_bind(Dictionary p_data_info);
	static Dictionary validate_now_bind(Dictionary p_data_info);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Data getters and setters (these have mutexes)
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ALL
	static bool has_bind(Dictionary p_data_info);

	static Dictionary get_bind(Dictionary p_data_info);
	static Error set_bind(Dictionary p_data_info, Dictionary p_data_dict);
	static Error merge_bind(Dictionary p_data_info, Dictionary p_data_dict);

	static Error remove_bind(Dictionary p_data_info);

	// SERIALIZATION
	static Dictionary dict_from_json(String p_json_string);
	static String dict_to_json(Dictionary p_dict, bool p_sort_keys = false, bool p_full_precision = false);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SINGLETON BOILERPLATING
	static void initialize();
	static void finalize();
};

#endif // GAME_DATA_MANAGER_H
