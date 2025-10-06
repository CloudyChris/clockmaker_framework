# resource_manager.gd
extends BaseManager

var db : Dictionary[String, GameDataEntry]
#-------------------------------------------------------------------------------
## main menu stuff and settings
var core_collections : Dictionary[String, GameDataCollection]
## default game data
var game_collections : Dictionary[String, GameDataCollection]
#-------------------------------------------------------------------------------
## user data
var user_collections : Dictionary[String, GameDataCollection]
## mods (usable)
var mods_collections : Dictionary[String, GameDataCollection]
## local mods (workspace)
var user_mods_collections : Dictionary[String, GameDataCollection]
#-------------------------------------------------------------------------------
var auto_load_collections : Dictionary[String, CollectionType] = {
	"res://data/core/settings.res" : CollectionType.CORE,
	"res://data/core/ui.res" : CollectionType.CORE
}

var auto_index_dirs : Dictionary[String, String] = {
	"user://saves": "*/info.res",
	"user://mods": "*/info.res",
	"user://user_mods": "*/info.res"
}
#-------------------------------------------------------------------------------
enum CollectionType {
	NONE,
	CORE,
	GAME,
	USER,
	MODS,
	USER_MODS
}
#-------------------------------------------------------------------------------
class LoadTaskInfo:
	var type : CollectionType = CollectionType.NONE
	var task : String = ""
	
	func _init(p_type: CollectionType = CollectionType.NONE, p_task: String = "") -> void:
		type = p_type
		task = p_task
#-------------------------------------------------------------------------------
var io_save_mutex : Mutex = Mutex.new()
## protected by io_save_mutex
var io_save_tasks : Array[GameDataCollection]
#-------------------------------------------------------------------------------
var load_tasks_1 : Array[LoadTaskInfo]
var load_tasks_2 : Array[LoadTaskInfo]
var load_tasks_s : bool = true
var load_tasks_failed : Array[LoadTaskInfo]
var is_loading : bool = false
var check_load_task : LoadTaskInfo
#------------------------------------------
var save_tasks_1 : Array[int]
var save_tasks_2 : Array[int]
var save_tasks_s : bool = true
var is_saving : bool = false
var check_save_task : int
#-------------------------------------------------------------------------------
func _ready() -> void:
	for path in auto_load_collections.keys():
		if ResourceLoader.exists(path, "GameDataCollection"):
			request_load(path, auto_load_collections.get(path, CollectionType.NONE))
	
	if FileAccess.file_exists("user://settings.res"):
		request_load("user://settings.res", CollectionType.USER)
	else:
		var settings_collection : GameDataCollection = get_collection("settings", CollectionType.CORE)
		if settings_collection:
			request_save(settings_collection.duplicate(true), "user://settings.res", true)
	
	set_colleciton(GameDataCollection.new(), "campaigns", CollectionType.USER)
	set_colleciton(GameDataCollection.new(), "mods", CollectionType.USER)
	set_colleciton(GameDataCollection.new(), "user_mods", CollectionType.USER)
	
	if not DirAccess.dir_exists_absolute("user://saves"):
		DirAccess.make_dir_recursive_absolute("user://saves")
	
	if not DirAccess.dir_exists_absolute("user://mods"):
		DirAccess.make_dir_recursive_absolute("user://mods")
	
	if not DirAccess.dir_exists_absolute("user://user_mods"):
		DirAccess.make_dir_recursive_absolute("user://user_mods")
	
	for path in auto_index_dirs.keys():
		index_dir(path, auto_index_dirs.get(path, ""), CollectionType.USER)

func _process(_delta: float) -> void:
	if is_loading:
		check_load_task = LoadTaskInfo.new()
		if load_tasks_s:
			if load_tasks_1.is_empty():
				if load_tasks_2.is_empty():
					is_loading = false
				else:
					load_tasks_s = false
			else:
				check_load_task = load_tasks_1.pop_back()
				
		else:
			if load_tasks_2.is_empty():
				if load_tasks_1.is_empty():
					is_loading = false
				else:
					load_tasks_s = true
			else:
				check_load_task = load_tasks_2.pop_back()
		if not check_load_task.task.is_empty():
			match (ResourceLoader.load_threaded_get_status(check_load_task.task)):
					ResourceLoader.ThreadLoadStatus.THREAD_LOAD_FAILED, ResourceLoader.ThreadLoadStatus.THREAD_LOAD_INVALID_RESOURCE:
						load_tasks_failed.append(check_load_task)
					ResourceLoader.ThreadLoadStatus.THREAD_LOAD_IN_PROGRESS:
						if load_tasks_s:
							load_tasks_1.append(check_load_task)
						else:
							load_tasks_2.append(check_load_task)
					ResourceLoader.ThreadLoadStatus.THREAD_LOAD_LOADED:
						var loaded_collection : GameDataCollection = ResourceLoader.load_threaded_get(check_load_task.task)
						match check_load_task.type:
							CollectionType.NONE:
								pass
							CollectionType.CORE:
								core_collections.set(loaded_collection.get_collection_name(), loaded_collection)
							CollectionType.GAME:
								game_collections.set(loaded_collection.get_name(), loaded_collection)
							CollectionType.USER:
								user_collections.set(loaded_collection.get_name(), loaded_collection)
							CollectionType.MODS:
								mods_collections.set(loaded_collection.get_name(), loaded_collection)
							CollectionType.USER_MODS:
								loaded_collection.should_be_gobbled = false
								user_mods_collections.set(loaded_collection.get_name(), loaded_collection)
							
						if loaded_collection.should_be_gobbled:
							gobble(loaded_collection)
	
	if is_saving:
		check_save_task = -1
		if save_tasks_s:
			if save_tasks_1.is_empty():
				if save_tasks_2.is_empty():
					is_saving = false
				else:
					save_tasks_s = false
			else:
				check_save_task = save_tasks_1.pop_back()
				
		else:
			if save_tasks_2.is_empty():
				if save_tasks_1.is_empty():
					is_saving = false
				else:
					save_tasks_s = true
			else:
				check_save_task = save_tasks_2.pop_back()
		if check_save_task >= 0:
			if not WorkerThreadPool.is_task_completed(check_save_task):
				if save_tasks_2:
					save_tasks_1.append(check_save_task)
				else:
					save_tasks_2.append(check_save_task)
#-------------------------------------------------------------------------------
func has_entry(p_collection_name: String, p_path: String, p_type: CollectionType) -> bool:
	var collection : GameDataCollection = null
	match p_type:
		CollectionType.NONE:
			return db.has(p_path)
		CollectionType.CORE:
			collection = core_collections.get(p_collection_name)
		CollectionType.GAME:
			collection = game_collections.get(p_collection_name)
		CollectionType.USER:
			collection = user_collections.get(p_collection_name)
		CollectionType.MODS:
			collection = mods_collections.get(p_collection_name)
		CollectionType.USER_MODS:
			collection = user_mods_collections.get(p_collection_name)
	
	if collection:
		return collection.has_entry(p_path)
	
	return false

func get_entry(p_collection_name: String, p_path: String, p_type: CollectionType) -> GameDataEntry:
	var collection : GameDataCollection = null
	match p_type:
		CollectionType.NONE:
			return db.get(p_path, null)
		CollectionType.CORE:
			collection = core_collections.get(p_collection_name)
		CollectionType.GAME:
			collection = game_collections.get(p_collection_name)
		CollectionType.USER:
			collection = user_collections.get(p_collection_name)
		CollectionType.MODS:
			collection = mods_collections.get(p_collection_name)
		CollectionType.USER_MODS:
			collection = user_mods_collections.get(p_collection_name)
	
	if collection:
		return collection.get_entry(p_path)
	
	return null

func set_entry(p_entry: GameDataEntry, p_collection_name: String, p_path: String, p_type: CollectionType) -> void:
	var collection : GameDataCollection = null
	match p_type:
		CollectionType.NONE:
			return
		CollectionType.CORE:
			collection = core_collections.get(p_collection_name)
		CollectionType.GAME:
			collection = game_collections.get(p_collection_name)
		CollectionType.USER:
			collection = user_collections.get(p_collection_name)
		CollectionType.MODS:
			collection = mods_collections.get(p_collection_name)
		CollectionType.USER_MODS:
			collection = user_mods_collections.get(p_collection_name)
			collection.should_be_gobbled = false
		_:
			return
	
	if collection:
		collection.set_entry(p_path, p_entry)
		if collection.should_be_gobbled:
			db.set(p_path, p_entry)

func has_collection(p_collection_name: String, p_type: CollectionType) -> bool:
	match p_type:
		CollectionType.NONE:
			return false
		CollectionType.CORE:
			return core_collections.has(p_collection_name)
		CollectionType.GAME:
			return game_collections.has(p_collection_name)
		CollectionType.USER:
			return user_collections.has(p_collection_name)
		CollectionType.MODS:
			return mods_collections.has(p_collection_name)
		CollectionType.USER_MODS:
			return user_mods_collections.has(p_collection_name)
		_:
			return false

func get_collection(p_collection_name: String, p_type: CollectionType) -> GameDataCollection:
	var collection : GameDataCollection = null
	match p_type:
		CollectionType.NONE:
			return null
		CollectionType.CORE:
			collection = core_collections.get(p_collection_name)
		CollectionType.GAME:
			collection = game_collections.get(p_collection_name)
		CollectionType.USER:
			collection = user_collections.get(p_collection_name)
		CollectionType.MODS:
			collection = mods_collections.get(p_collection_name)
		CollectionType.USER_MODS:
			collection = user_mods_collections.get(p_collection_name)
	
	return collection

func set_colleciton(p_collection: GameDataCollection, p_collection_name: String, p_type: CollectionType) -> void:
	match p_type:
		CollectionType.NONE:
			return
		CollectionType.CORE:
			core_collections.set(p_collection_name, p_collection)
		CollectionType.GAME:
			game_collections.set(p_collection_name, p_collection)
		CollectionType.USER:
			user_collections.set(p_collection_name, p_collection)
		CollectionType.MODS:
			mods_collections.set(p_collection_name, p_collection)
		CollectionType.USER_MODS:
			user_mods_collections.set(p_collection_name, p_collection)
	
	if p_collection.should_be_gobbled:
		gobble(p_collection)

func collection_gobble_entry(p_entry:GameDataEntry, p_path: String = "", p_collection_name: String = "", p_type: CollectionType = CollectionType.NONE) -> void:
	var collection : GameDataCollection = null
	match p_type:
		CollectionType.NONE:
			gobble_entry(p_entry, p_path)
		CollectionType.CORE:
			collection = core_collections.get(p_collection_name)
		CollectionType.GAME:
			collection = game_collections.get(p_collection_name)
		CollectionType.USER:
			collection = user_collections.get(p_collection_name)
		CollectionType.MODS:
			collection = mods_collections.get(p_collection_name)
		CollectionType.USER_MODS:
			collection = user_mods_collections.get(p_collection_name)
	
	if collection:
		collection.gobble_entry(p_entry, p_path)

func make_collection_from_entry(p_entry:GameDataEntry, p_path: String = "", p_collection_name: String = "", p_should_gobble : bool = false) -> GameDataCollection:
	var collection : GameDataCollection = GameDataCollection.new(p_collection_name)
	collection.gobble_entry(p_entry, p_path)
	collection.should_be_gobbled = p_should_gobble
	
	return collection

func gobble(p_collection: GameDataCollection) -> void:
	db.merge(p_collection.game_data)

func gobble_entry(p_entry: GameDataEntry, p_path: String = "") -> void:
	db.game_data.set(p_entry.path if p_path.is_empty() else p_path, p_entry)

#-------------------------------------------------------------------------------
func get_entries(p_common_path: String) -> Array[GameDataEntry]:
	var keys : Array[String] = db.keys().filter(func(key: String): return key.begins_with(p_common_path))
	var entries : Array[GameDataEntry]
	for key in keys:
		entries.append(db.get(key))
	return entries
#-------------------------------------------------------------------------------
func index_dir(p_path: String, p_pattern: String, p_type: CollectionType) -> void:
	if p_path.is_empty() and p_pattern.is_empty():
		return
	if p_pattern.contains("**"):
		return
	if not p_path.is_absolute_path():
		return
	if not DirAccess.dir_exists_absolute(p_path):
		return
	
	var pattern_segments : Array[String] = StringUtils.str_split(p_pattern, "/", false)
	if pattern_segments.is_empty():
		return
	var search_depth : int = p_pattern.count("/")
	var dir_access : DirAccess = DirAccess.open(p_path)
	
	if dir_access:
		dir_access.include_hidden = false
		dir_access.include_navigational = false
		dir_access.list_dir_begin()
		var fs_entry_name: String = dir_access.get_next()
		while fs_entry_name != "":
			if search_depth > 0:
				if dir_access.current_is_dir():
					if fs_entry_name.match(pattern_segments[0]):
						index_dir("/".join([p_path, fs_entry_name]), "/".join(pattern_segments.slice(1)), p_type)
			else:
				if not dir_access.current_is_dir():
					if fs_entry_name.match(p_pattern):
						request_load("/".join([p_path, fs_entry_name]), p_type)
					
			fs_entry_name = dir_access.get_next()
#-------------------------------------------------------------------------------
func request_load(p_filepath: String, p_type: CollectionType) -> bool:
	if p_filepath.is_empty() or not p_filepath.is_absolute_path():
		return false
	
	if ResourceLoader.load_threaded_request(p_filepath, "GameDataCollection", false, ResourceLoader.CACHE_MODE_REUSE) != OK:
		return false
	
	if load_tasks_s:
		load_tasks_1.append(LoadTaskInfo.new(p_type, p_filepath))
	else:
		load_tasks_2.append(LoadTaskInfo.new(p_type, p_filepath))
	
	return true

func request_save(p_collection: GameDataCollection, p_path: String = "", p_take_over_path: bool = false) -> void:
	if not p_collection:
		return
	
	if not p_path.is_empty() and p_path.is_absolute_path():
		if p_take_over_path:
			p_collection.take_over_path(p_path)
	
	if p_collection.resource_path.is_empty() or not p_collection.resource_path.is_absolute_path():
		return
	
	io_save_mutex.lock()
	io_save_tasks.append(p_collection.duplicate(true))
	io_save_mutex.unlock()
	var task_id = WorkerThreadPool.add_task(_save_threaded, true, "Saving data")
	if save_tasks_s:
		save_tasks_1.append(task_id)
	else:
		save_tasks_2.append(task_id)

func _save_threaded() -> void:
	var current_collection : GameDataCollection = null
	while (true):
		io_save_mutex.lock()
		current_collection = io_save_tasks.pop_back()
		io_save_mutex.unlock()
		
		if not current_collection:
			return
		
		ResourceSaver.save(current_collection, "", ResourceSaver.FLAG_COMPRESS + ResourceSaver.FLAG_OMIT_EDITOR_PROPERTIES)
