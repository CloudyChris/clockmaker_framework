# code_graph_slot.gd
class_name CodeGraphNodeSlot
extends RefCounted

enum SlotType {
	UNINITIALIZED,
	INPUT,
	OUTPUT
}
#-------------------------------------------------------------------------------
var name : String = ""
var slot_type : CodeGraphNodeSlot.SlotType = CodeGraphNodeSlot.SlotType.UNINITIALIZED
#-------------------------------------------------------------------------------
var value_type : PropertyInfo = null:
	set(p_val):
		value_type = p_val
		value_type_changed.emit(value_type)
var default_value :
	set(p_val):
		default_value = p_val
		has_default_value = true
var has_default_value : bool = false
var value :
	set(p_val):
		value = p_val
		match(slot_type):
			CodeGraphNodeSlot.SlotType.UNINITIALIZED:
				pass
			CodeGraphNodeSlot.SlotType.INPUT:
				if parent_node:
					parent_node.inputs_updated += 1
			CodeGraphNodeSlot.SlotType.OUTPUT:
				if active_connection:
					active_connection.update()
	get():
		if active_connection and value:
			return value
		if has_default_value and default_value:
			return default_value
		return null
#-------------------------------------------------------------------------------
signal value_type_changed(p_value_type: PropertyInfo)
#-------------------------------------------------------------------------------
var parent_node : CodeGraphNode = null

var active_connection : CodeGraphConnection = null :
	set(p_val):
		active_connection = p_val
		if parent_node:
			match slot_type:
				CodeGraphNodeSlot.SlotType.INPUT:
					if active_connection:
						parent_node.inputs_connected += 1
					else:
						parent_node.inputs_connected -= 1
				CodeGraphNodeSlot.SlotType.OUTPUT:
					if active_connection:
						parent_node.outputs_connected += 1
					else:
						parent_node.outputs_connected -= 1
				_:
					pass
		else:
			push_warning("Connected slot that doesn't belong to a node")
#-------------------------------------------------------------------------------
func _init(p_name: String, p_slot_type: CodeGraphNodeSlot.SlotType, p_value_type: PropertyInfo = null) -> void:
	name = p_name
	slot_type = p_slot_type
	
	value_type = p_value_type
	notify_property_list_changed()
	value_type_changed.emit(value_type)

func _get_property_list() -> Array[Dictionary]:
	var properties: Array[Dictionary] = []
	
	properties.append({
		"name": "value",
		"type": value_type.type,
		"hint": value_type.hint,
		"hint_string": value_type.hint_string
	})
	
	properties.append({
		"name": "default_value",
		"type": value_type.type,
		"hint": value_type.hint,
		"hint_string": value_type.hint_string
	})
	
	return properties
#-------------------------------------------------------------------------------
func reset() -> void:
	if has_default_value:
		value = default_value

func reset_default_value() -> void:
	has_default_value = false
