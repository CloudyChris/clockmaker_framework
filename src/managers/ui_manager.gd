# ui_manager.gd
extends BaseManager

class DoubleLinkedListNode extends RefCounted:
	var ui : Control
	#-------------------------------
	var next : DoubleLinkedListNode = null
	var prev : DoubleLinkedListNode = null
	#-------------------------------
	func _init(p_ui: Control) -> void:
		ui = p_ui
#-------------------------------------------------------------------------------
var ui_elements : Dictionary[String, UIElement] = {}
#-------------------------------------------------------------------------------
var ui_root : Control = null
var ui_base_root : Control = null
var ui_stack_root : Control = null
#-------------------------------------------------------------------------------
var ui_stack_tail : DoubleLinkedListNode
var ui_stack : Dictionary[String, DoubleLinkedListNode]
#-------------------------------------------------------------------------------
func load_manager() -> bool:
	load_core_ui()
	return true

func reset_manager() -> bool:
	if ui_base_root:
		for node in ui_base_root.get_children():
			ui_base_root.remove_child(node)
	for ui_name in ui_stack.keys():
		close_ui(ui_name)
	ui_elements.clear()
	return true
#-------------------------------------------------------------------------------
func load_core_ui() -> void:
	reset_manager()
	var ui_base : UIElement = ResourceManager.get_entry("", "/core/ui/base", ResourceManager.CollectionType.NONE)
	change_base(ui_base)
	var ui_elements_array : Array = ResourceManager.get_entries("/core/ui/elements")
	for ui_el in ui_elements_array:
		if ui_el is UIElement:
			ui_elements.set(ui_el.resource_name, ui_el)

func load_game_ui() -> void:
	reset_manager()
	var ui_base : UIElement = ResourceManager.get_entry("", "/game/ui/base", ResourceManager.CollectionType.NONE)
	change_base(ui_base)
	var ui_elements_array : Array = ResourceManager.get_entries("/game/ui/elements")
	for ui_el in ui_elements_array:
		if ui_el is UIElement:
			ui_elements.set(ui_el.resource_name, ui_el)
#-------------------------------------------------------------------------------
func _ready() -> void:
	var ui_root_instance = preload("res://src/instances/ui/ui_root.tscn").instantiate()
	add_child(ui_root_instance)
#-------------------------------------------------------------------------------
func get_ui_element(p_string: String) -> UIElement:
	return ui_elements.get(p_string, null)
#-------------------------------------------------------------------------------
func register_ui_root(p_root: Control) -> void:
	ui_root = p_root

func register_ui_base_root(p_root: Control) -> void:
	ui_base_root = p_root

func register_ui_stack_root(p_root: Control) -> void:
	ui_stack_root = p_root
#-------------------------------------------------------------------------------
func change_base(p_base: UIElement) -> void:
	if ui_base_root:
		for node in ui_base_root.get_children():
			node.queue_free()
		ui_base_root.add_child(p_base.ui_tree.instantiate())
#-------------------------------------------------------------------------------
func append_to_stack(p_lln: DoubleLinkedListNode) -> void:
	if is_instance_valid(ui_stack_tail):
		ui_stack_tail.next = p_lln
		p_lln.prev = ui_stack_tail
	ui_stack_tail = p_lln
	ui_stack.set(p_lln.ui.resource_name, p_lln)

func pop_stack() -> void:
	if ui_stack_root:
		if is_instance_valid(ui_stack_tail):
			close_ui(ui_stack_tail.ui.name)
		else:
			# TODO add pause menu to stack
			SessionManager.pause()
#-------------------------------------------------------------------------------
func open_ui(p_ui_name: String) -> void:
	var ui_el : UIElement = get_ui_element(p_ui_name)
	if not ui_el:
		return
	var control : Control = ui_el.ui_tree.instantiate()
	control.name = p_ui_name
	if ui_stack_root:
		var dlln : DoubleLinkedListNode = DoubleLinkedListNode.new(control)
		append_to_stack(dlln)
		ui_stack_root.add_child(control)

func close_ui(p_ui_name: String) -> void:
	var ui_el_lln : DoubleLinkedListNode = ui_stack.get(p_ui_name, null)
	if is_instance_valid(ui_el_lln):
		if ui_el_lln.prev:
			ui_el_lln.prev.next = ui_el_lln.next
		if ui_el_lln.next:
			ui_el_lln.next.prev = ui_el_lln.prev
		if ui_el_lln.ui:
			ui_stack_root.remove_child(ui_el_lln.ui)
		ui_stack.erase(p_ui_name)

func focus(p_ui_name: String) -> void:
	var ui_el_lln : DoubleLinkedListNode = ui_stack.get(p_ui_name, null)
	if is_instance_valid(ui_el_lln):
		if ui_el_lln.prev:
			ui_el_lln.prev.next = ui_el_lln.next
		if ui_el_lln.next:
			ui_el_lln.next.prev = ui_el_lln.prev
		append_to_stack(ui_el_lln)
