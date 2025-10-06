# game_data_collection.gd
@tool
class_name GameDataCollection
extends TrackedResource

@export var game_data : Dictionary[String, GameDataEntry]
@export var should_be_gobbled : bool = false
#-------------------------------------------------------------------------------
func _init(p_name: String = ""):
	set_name(p_name)
#-------------------------------------------------------------------------------
func gobble(p_collection: GameDataCollection) -> void:
	game_data.merge(p_collection.game_data, true)

func gobble_entry(p_entry: GameDataEntry, p_path: String = "") -> void:
	game_data.set(p_entry.path if p_path.is_empty() else p_path, p_entry)
#-------------------------------------------------------------------------------
func has_entry(p_path) -> bool:
	return game_data.has(p_path)

func get_entry(p_path: String) -> GameDataEntry:
	return game_data.get(p_path)

func set_entry(p_path: String, p_entry: GameDataEntry) -> void:
	game_data.set(p_path, p_entry)
