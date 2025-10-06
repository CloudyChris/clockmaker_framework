# code_graph_connection.gd
@tool
class_name CodeGraphConnection
extends CodeGraphElement

var input_node_slot : CodeGraphNodeSlot = null :
	set(p_val):
		if not p_val and input_node_slot:
			input_node_slot.active_connection = null
		input_node_slot = p_val
		if input_node_slot:
			input_node_slot.active_connection = self
#-------------------------------------------------------------------------------
var output_node_slot : CodeGraphNodeSlot = null :
	set(p_val):
		if not p_val and output_node_slot:
			output_node_slot.active_connection = null
		output_node_slot = p_val
		if output_node_slot:
			output_node_slot.active_connection = self

func update() -> void:
	if is_valid:
		output_node_slot.value = input_node_slot.value
#-------------------------------------------------------------------------------
func validate() -> bool:
	is_valid = true
	if not super():
		is_valid = false
		return false
	
	if not input_node_slot or not output_node_slot:
		is_valid = false
		return false
	
	if not input_node_slot.parent_node or not output_node_slot.parent_node:
		is_valid = false
		return false

	if input_node_slot.value_type.type != TYPE_NIL and output_node_slot.value_type.type != TYPE_NIL:
		if input_node_slot.value_type.type != output_node_slot.value_type.type:
			is_valid = false
			return false
		if input_node_slot.value_type.hint != output_node_slot.value_type.hint:
			is_valid = false
			return false
		if input_node_slot.value_type.hint_string != output_node_slot.value_type.hint_string:
			is_valid = false
			return false
	
	return is_valid
