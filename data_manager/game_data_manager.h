/* game_data_manager.h */

#ifndef GAME_DATA_MANAGER_H
#define GAME_DATA_MANAGER_H

#include "core/string/ustring.h"
#pragma region godot_includes
#include "core/error/error_list.h"
#include "core/object/worker_thread_pool.h"
#include "core/os/thread.h"
#pragma endregion godot_includes

#pragma region cm_includes
#include "modules/clockmakers_framework/data_manager/game_data_collections.h"
#include "modules/clockmakers_framework/data_manager/game_data_specifications.h"
#include "modules/clockmakers_framework/vector_hashmap_pair.h"
#include "modules/clockmakers_framework/cm_enums.h"
#pragma endregion cm_includes

struct DataInfo
{
	cm_enums::CM_DataType data_type = cm_enums::CM_DataType::CM_DATA_TYPE_NONE;
	cm_enums::CM_DataCollectionType collection_type = cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE;
	String path = "";
	String collection = "";
	String table = "";
	String entry = UUID::empty().get_uuid_string_bind();

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
	};

	struct ValidationTask
	{
		WorkerThreadPool::TaskID task_id = 0;
		Error error = ERR_UNCONFIGURED;
		DataInfo data_info;
		Dictionary collection_dict;
	};

private:
	// These define data types. Used for validation, tools ui, and they also serve as type reservations.
	static VectorHashMapPair<String, TableSpecification> table_specifications;

	// Core entries cannot be overriden
	static VectorHashMapPair<String, GameDataEntry *> core_entries_by_uuid;

	// Game entries can be overriden by mods
	// Overrides happen in order, overriden resources will be unloaded
	static VectorHashMapPair<String, GameDataEntry *> game_entries_by_uuid;

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
	static RWLock data_lock;

	// Resolve groups, and cleaning tasks bool
	static RWLock tasks_lock;

	// to allow for defered loads
	static VectorHashMapPair<String, ValidationTask> validation_tasks;
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
	static Error validate(DataInfo p_data_info); // TODO

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Private data getters and setters
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ALL
	static bool has(DataInfo p_data_info);
	static Error remove(DataInfo p_data_info);

	// Below private methods don't have locks. Don't directly call these unless you know what you're doing x2
	// COLLECTION
	static GameDataCollection *get_collection(DataInfo p_data_info);
	static Error set_collection(DataInfo p_data_info, const GameDataCollection &p_data_collection); // TODO
	static Error merge_collection(DataInfo p_data_info, const GameDataCollection &p_data_collection); // TODO

	// TABLE SPECIFICATION
	static TableSpecification *get_data_table_specification(DataInfo p_data_info);
	static Error set_data_table_specification(DataInfo p_data_info, const TableSpecification &p_table_specification); // TODO

	// TABLE
	static GameDataTable *get_data_table(DataInfo p_data_info);
	static Error set_data_table(DataInfo p_data_info, const GameDataTable &p_data_table); // TODO
	static Error merge_data_table(DataInfo p_data_info, const GameDataTable &p_data_table); // TODO

	// ENTRY
	static GameDataEntry *get_data_entry(DataInfo p_data_info);
	static Error set_data_entry(DataInfo p_data_info, const GameDataEntry &p_data_entry); // TODO
	static Error merge_data_entry(DataInfo p_data_info, const GameDataEntry &p_data_entry); // TODO

public:
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Load & Save Data (these launch threaded methods)
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static String request_load_bind(Dictionary p_data_info);

	static String request_save_bind(Dictionary p_data_info);

	static String request_validation_bind(Dictionary p_data_info);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Data getters and setters (these have mutexes)
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ALL
	static bool has_bind(Dictionary p_data_info);

	static Dictionary get_bind(Dictionary p_data_info);
	static Error set_bind(Dictionary p_data_info, Dictionary p_data_dict); // TODO
	static Error merge_bind(Dictionary p_data_info, Dictionary p_data_dict); // TODO

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
