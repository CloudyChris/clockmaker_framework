# code_graph_node.gd
@tool
class_name CodeGraphNode
extends CodeGraphElement

var input_slots : Array[CodeGraphNodeSlot]
var input_slot_names : Dictionary[String, int]
var input_slot_holes : Array[int]
var base_input_slot_id : int = 0
#-------------------------------------------------------------------------------
var output_slots : Array[CodeGraphNodeSlot]
var output_slot_names : Dictionary[String, int]
var output_slot_holes : Array[int]
var base_output_slot_id : int = 0
#-------------------------------------------------------------------------------
var run_once : bool = false
var has_been_run : bool = false
#-------------------------------------------------------------------------------
var inputs_connected : int = 0
var inputs_updated : int = 0

var outputs_connected : int = 0
#-------------------------------------------------------------------------------
var node_color : Color
var input_slot_ui_builder_data : Array[CodeGraphDisplay.SlotDisplayGroup]
var output_slot_ui_builder_data : Array[CodeGraphDisplay.SlotDisplayGroup]
var node_ui_builder_data : CodeGraphDisplay.NodeElementsGrid
var node_ui : PanelContainer
#-------------------------------------------------------------------------------
func build_node_ui() -> void:
	pass
#-------------------------------------------------------------------------------
func reset_slots() -> void:
	input_slots.clear()
	input_slot_names.clear()
	input_slot_holes.clear()
	base_input_slot_id = 0
	
	output_slots.clear()
	output_slot_names.clear()
	output_slot_holes.clear()
	base_output_slot_id = 0

func set_slots(p_input_slots: Dictionary[String, PropertyInfo] = {}, p_output_slots: Dictionary[String, PropertyInfo] = {}) -> void:
	reset_slots()
	if not p_input_slots.is_empty():
		for input_slot_key in p_input_slots.keys():
			add_input_slot(input_slot_key, p_input_slots.get(input_slot_key))
	if not p_output_slots.is_empty():
		for output_slot_key in p_output_slots.keys():
			add_output_slot(output_slot_key, p_output_slots.get(output_slot_key))
#-------------------------------------------------------------------------------
func make_builder_data() -> void:
	var slot_group : CodeGraphDisplay.SlotDisplayGroup = null
	var slot_group_index : int = -1
	var slot_keys : Array[String] = []
	if not input_slot_names.is_empty():
		slot_keys = input_slot_names.keys()
		slot_keys.sort_custom(func (a: String, b: String): return int(a.split(":")[0]) < int(b.split(":")[0]))
		for input_slot_key in slot_keys:
			slot_group_index = int(input_slot_key.split(":")[0])
			if slot_group_index >= input_slot_ui_builder_data.size():
				slot_group = CodeGraphDisplay.SlotDisplayGroup.new([input_slots.get(input_slot_names.get(input_slot_key, -1))])
				input_slot_ui_builder_data.append(slot_group)
			else:
				input_slot_ui_builder_data[slot_group_index].slots.append(input_slots.get(input_slot_names.get(input_slot_key, -1)))
	
	if not output_slot_names.is_empty():
		slot_keys = output_slot_names.keys()
		slot_keys.sort_custom(func (a: String, b: String): return int(a.split(":")[0]) < int(b.split(":")[0]))
		for output_slot_key in slot_keys:
			slot_group_index = int(output_slot_key.split(":")[0])
			if slot_group_index >= output_slot_ui_builder_data.size():
				slot_group = CodeGraphDisplay.SlotDisplayGroup.new([output_slots.get(output_slot_names.get(output_slot_key, -1))])
				output_slot_ui_builder_data.append(slot_group)
			else:
				output_slot_ui_builder_data[slot_group_index].slots.append(output_slots.get(output_slot_names.get(output_slot_key, -1)))
#-------------------------------------------------------------------------------
func has_input_slot(p_name: String) -> bool:
	return input_slot_names.has(p_name)

func get_input_slot(p_name: String) -> CodeGraphNodeSlot:
	var slot_id : int = input_slot_names.get(p_name, -1)
	if slot_id < 0:
		return null
	return input_slots.get(slot_id)

func add_input_slot(p_name: String, p_type: PropertyInfo) -> int:
	var assigned_id : int
	if input_slot_names.has(p_name):
		return -1
	if not input_slot_holes.is_empty():
		assigned_id = input_slot_holes.pop_back()
	else:
		assigned_id = base_input_slot_id
		base_input_slot_id += 1
	
	if assigned_id >= input_slots.size():
		input_slots.resize(assigned_id+1)
	
	input_slots.set(assigned_id, CodeGraphNodeSlot.new(p_name, CodeGraphNodeSlot.SlotType.INPUT, p_type))
	input_slot_names.set(p_name, assigned_id)
	
	input_slots.get(assigned_id).parent_node = self
	
	return assigned_id

func remove_input_slot(p_name: String) -> bool:
	if not input_slot_names.has(p_name):
		return false
	var id_to_remove : int = input_slot_names.get(p_name, -1)
	input_slot_names.erase(p_name)
	if id_to_remove < 0:
		return false
	input_slots.set(id_to_remove, null)
	input_slot_holes.append(id_to_remove)
	return true
#-------------------------------------------------------------------------------
func has_output_slot(p_name: String) -> bool:
	return output_slot_names.has(p_name)

func get_output_slot(p_name: String) -> CodeGraphNodeSlot:
	var slot_id : int = output_slot_names.get(p_name, -1)
	if slot_id < 0:
		return null
	return output_slots.get(slot_id)

func add_output_slot(p_name: String, p_type: PropertyInfo) -> int:
	var assigned_id : int
	if output_slot_names.has(p_name):
		return -1
	if not output_slot_holes.is_empty():
		assigned_id = output_slot_holes.pop_back()
	else:
		assigned_id = base_output_slot_id
		base_output_slot_id += 1
	
	if assigned_id >= output_slots.size():
		output_slots.resize(assigned_id+1)
	
	output_slots.set(assigned_id, CodeGraphNodeSlot.new(p_name, CodeGraphNodeSlot.SlotType.OUTPUT, p_type))
	output_slot_names.set(p_name, assigned_id)
	
	output_slots.get(assigned_id).parent_node = self
	
	return assigned_id

func remove_output_slot(p_name: String) -> bool:
	if not output_slot_names.has(p_name):
		return false
	var id_to_remove : int = output_slot_names.get(p_name, -1)
	output_slot_names.erase(p_name)
	if id_to_remove < 0:
		return false
	output_slots.set(id_to_remove, null)
	output_slot_holes.append(id_to_remove)
	return true
#-------------------------------------------------------------------------------
## virtual, super this at the beginning
func validate() -> bool:
	is_valid = true
	if not super():
		is_valid = false
		return false
	
	for slot_id in input_slot_names.values():
		if not input_slots[slot_id].active_connection and not input_slots[slot_id].has_default_value:
			is_valid = false
			return false
	
	if has_parent_graph():
		if not has_valid_graph_id():
			is_valid = false
			return false
	
	return true

## virtual, call super at start
func reset() -> void:
	for slot_id in input_slot_names.values():
		input_slots[slot_id].reset()
#-------------------------------------------------------------------------------
## virtual, call super first
func can_run() -> bool:
	if inputs_updated != inputs_connected:
		return false
	return true

## virtual, override this for node functionality
func node_run() -> bool:
	return true

## main routine
func run() -> bool:
	if not is_valid or SessionManager.is_paused:
		return false
	if not run_once:
		has_been_run = false
	if has_been_run:
		return true
	if not can_run():
		return false
	if not node_run():
		return false
	tidy_state()
	has_been_run = true
	return true

## virtual, call super at start
func tidy_state() -> void:
	if inputs_updated > 0:
		inputs_updated = 0
