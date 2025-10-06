@tool
extends ColorRect

const slot_size : int = 16

var slot : CodeGraphNodeSlot

@export var slot_group_index : int = 0
@export var slot_group_size : int = 0

var slot_value_type : PropertyInfo = null

func _ready() -> void:
	if slot:
		slot.value_type_changed.connect(on_value_type_changed)
#-------------------------------------------------------------------------------
func on_value_type_changed(p_value_type: PropertyInfo) -> void:
	slot_value_type = p_value_type
	color = CodeGraphDisplay.slot_colors.get(slot_value_type.type)
