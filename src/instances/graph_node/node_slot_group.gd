@tool
extends VBoxContainer

static var node_slot_packed_scene : PackedScene = preload("res://src/instances/graph_node/node_slot.tscn")
#-------------------------------------------------------------------------------
var slot_group : CodeGraphDisplay.SlotDisplayGroup = null :
	set(p_val):
		slot_group = p_val

const slot_size : int = 16

var node_size : Vector2i = Vector2i()
var slot_group_size : int = 0
#-------------------------------------------------------------------------------
func build_slot_group() -> void:
	print("Building node slot group...")
	var slot_instance : ColorRect = null
	slot_group_size = slot_group.slots.size()
	for slot_index in range(slot_group_size):
		print("Instantiating slot: "+ slot_group.slots[slot_index].name)
		slot_instance = node_slot_packed_scene.instantiate()
		slot_instance.slot_group_index = slot_index
		slot_instance.slot_group_size = slot_group_size
		slot_instance.slot = slot_group.slots[slot_index]
		
		add_child(slot_instance)
#-------------------------------------------------------------------------------
