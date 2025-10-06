# tracked_resource.gd
@tool
class_name TrackedResource
extends Resource

@export_group("TrackedResource", "tracked_res_")
@export var tracked_res_uuid : UUID
#-------------------------------------------------------------------------------
func _init():
	tracked_res_uuid = UUID.new()
#-------------------------------------------------------------------------------
func get_uuid() -> UUID:
	return tracked_res_uuid

func get_uuid_string() -> String:
	return tracked_res_uuid.get_string()

func get_uuid_array() -> Array[int]:
	return tracked_res_uuid.get_array()

func get_uuid_dict() -> Dictionary[String, int]:
	return tracked_res_uuid.get_dict()
