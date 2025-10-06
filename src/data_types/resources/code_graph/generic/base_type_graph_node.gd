# base_type_graph_node.gd
@tool
class_name BaseTypeGraphNode
extends GenericCodeGraphNode

var value
@export var type : Variant.Type = TYPE_BOOL :
	set(p_val):
		if p_val > 0 and p_val < 5:
			type = p_val
			notify_property_list_changed()
			change_slot_types()
#-------------------------------------------------------------------------------
func _init() -> void:
	resource_name = "Base Type"
	
	var node_input_slots : Dictionary[String, PropertyInfo] = {
		"0:input" :  PropertyInfo.new("value", type)
	}
	var node_output_slots : Dictionary[String, PropertyInfo] = {
		"0:output" : PropertyInfo.new("value", type)
	}
	
	set_slots(node_input_slots, node_output_slots)
	
	make_builder_data()

func _get_property_list() -> Array[Dictionary]:
	var property_list : Array[Dictionary] = []
	
	property_list.append({
		"name" : "value",
		"type" : type,
		"hint" : PROPERTY_HINT_NONE,
		"hint_string" : ""
	})
	
	return property_list
#-------------------------------------------------------------------------------
func change_slot_types() -> void:
	var input_slot : CodeGraphNodeSlot = get_input_slot("input")
	var output_slot : CodeGraphNodeSlot = get_output_slot("output")
	
	input_slot.value_type.type = type
	output_slot.value_type.type = type
#-------------------------------------------------------------------------------
func node_run() -> bool:
	var input_slot : CodeGraphNodeSlot = get_input_slot("input")
	var output_slot : CodeGraphNodeSlot = get_output_slot("output")
	
	value = input_slot.value
	
	output_slot.value = value
	
	return true
