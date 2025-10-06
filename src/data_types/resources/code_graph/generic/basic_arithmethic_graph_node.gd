# basic_arithmetic_graph_node.gd
@tool
class_name BasicArithmeticGraphNode
extends GenericCodeGraphNode

enum OPERATIONS {
	ADD,
	SUBTRACT,
	MULTIPLY,
	DIVIDE
}

var value
@export var operation : OPERATIONS = OPERATIONS.ADD :
	set(p_val):
		operation = p_val
		change_node_operation()
#-------------------------------------------------------------------------------
func _init() -> void:
	resource_name = "Basic Arithmetic Operation"
	
	var node_input_slots : Dictionary[String, PropertyInfo] = {
		"0:value_1" :  PropertyInfo.new("value", TYPE_FLOAT),
		"1:value_2" :  PropertyInfo.new("value", TYPE_FLOAT)
	}
	var node_output_slots : Dictionary[String, PropertyInfo] = {
		"0:result" : PropertyInfo.new("value", TYPE_FLOAT)
	}
	
	set_slots(node_input_slots, node_output_slots)
	
	make_builder_data()

#-------------------------------------------------------------------------------
func change_node_operation() -> void:
	pass
#-------------------------------------------------------------------------------
func node_run() -> bool:
	var v1_slot : CodeGraphNodeSlot = get_input_slot("value_1")
	var v2_slot : CodeGraphNodeSlot = get_input_slot("value_2")
	var result_slot : CodeGraphNodeSlot = get_output_slot("result")
	
	if v1_slot.value and v2_slot.value:
		match operation:
			OPERATIONS.ADD:
				value = v1_slot.value + v2_slot.value
			OPERATIONS.SUBTRACT:
				value = v1_slot.value - v2_slot.value
			OPERATIONS.MULTIPLY:
				value = v1_slot.value * v2_slot.value
			OPERATIONS.DIVIDE:
				value = v1_slot.value / v2_slot.value
	
	result_slot.value = value
	
	return true
