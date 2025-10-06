# debug_graph_node.gd
@tool
class_name DebugGraphNode
extends GenericCodeGraphNode

@export var debug_string : String = ""
#-------------------------------------------------------------------------------
func _init() -> void:
	resource_name = "Debug"
	
	var node_input_slots : Dictionary[String, PropertyInfo] = {
		"0:input" :  PropertyInfo.new("value")
	}
	
	set_slots(node_input_slots)
	
	make_builder_data()
#-------------------------------------------------------------------------------
func node_run() -> bool:
	var variant_input : CodeGraphNodeSlot = get_input_slot("input")
	var previous_debug_string : String = debug_string
	if variant_input:
		debug_string = var_to_str(variant_input.value)
	
	if OS.is_debug_build():
		if debug_string != previous_debug_string:
			print(debug_string)
	
	return true
