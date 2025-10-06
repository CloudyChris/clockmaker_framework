@tool
class_name GraphNodeInstance
extends PanelContainer

const slot_half_size : int = 8
@onready var input_slots_root : Control = $container_stack/body/input_slots
@onready var output_slots_root : Control = $container_stack/body/output_slots
@onready var title : Label = $container_stack/title_bar/title
#-------------------------------------------------------------------------------
static var node_slot_group_packed_scene : PackedScene = preload("res://src/instances/graph_node/node_slot_group.tscn")
#-------------------------------------------------------------------------------
@export var code_graph_node : CodeGraphNode = null
@export_tool_button("Build") var tool_build_node_button = tool_build_node
#-------------------------------------------------------------------------------
var input_slot_segments : int = 0
var output_slot_segments : int = 0
#-------------------------------------------------------------------------------
func _ready() -> void:
	tool_build_node()
#-------------------------------------------------------------------------------
func build_node(p_input_slots: Array[CodeGraphDisplay.SlotDisplayGroup], p_output_slots: Array[CodeGraphDisplay.SlotDisplayGroup], p_elements: CodeGraphDisplay.NodeElementsGrid):
	print ("Building node...")
	for child in input_slots_root.get_children():
		child.queue_free()
	for child in output_slots_root.get_children():
		child.queue_free()
	input_slot_segments = 0
	output_slot_segments = 0
	var slot_group_instance : Control = null
	var input_size : int = 0
	var input_slot_index_count : int = 0
	var output_size : int = 0
	var output_slot_index_count : int = 0
	
	var elements_size_height : int = 0
	var elements_size_width : int = 0
	
	for slot_group in p_input_slots:
		slot_group.slots.all(func(p_slot: CodeGraphNodeSlot): print("Processing slot: "+ p_slot.name))
		slot_group_instance = null
		input_size += (slot_group.slots.size()+1)*32
		slot_group_instance = node_slot_group_packed_scene.instantiate()
		slot_group_instance.slot_group = slot_group
		
		slot_group_instance.build_slot_group()
		input_slot_index_count += slot_group.slots.size()
		
		input_slots_root.add_child(slot_group_instance)
	
	input_slot_segments = input_slot_index_count
	
	for slot_group in p_output_slots:
		slot_group.slots.all(func(p_slot: CodeGraphNodeSlot): print("Processing slot: "+ p_slot.name))
		slot_group_instance = null
		output_size += (slot_group.slots.size() + 1) * 32
		slot_group_instance = node_slot_group_packed_scene.instantiate()
		slot_group_instance.slot_group = slot_group
		
		output_slots_root.add_child(slot_group_instance)
		
		slot_group_instance.build_slot_group()
		output_slot_index_count += slot_group.slots.size()
	
	output_slot_segments = output_slot_index_count
	
	size = Vector2i(max(title.size.x, elements_size_width+64), 32+max(input_size, output_size, elements_size_height))
#-------------------------------------------------------------------------------
func tool_build_node() -> void:
	if code_graph_node:
		title.text = code_graph_node.resource_name
		build_node(code_graph_node.input_slot_ui_builder_data, code_graph_node.output_slot_ui_builder_data, code_graph_node.node_ui_builder_data)
	else:
		for child in input_slots_root.get_children():
			child.queue_free()
		for child in output_slots_root.get_children():
			child.queue_free()
