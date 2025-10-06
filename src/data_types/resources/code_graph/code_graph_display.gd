@tool
class_name CodeGraphDisplay
extends RefCounted

# TODO make a real theme
static var slot_colors : Dictionary[Variant.Type, Color] = {
	TYPE_NIL : Color("#ffff"),
	TYPE_BOOL : Color("#f00f"),
	TYPE_INT : Color("#00ff"),
	TYPE_FLOAT : Color("#0f0f")
}
#-------------------------------------------------------------------------------
class SlotDisplayGroup:
	var slots : Array[CodeGraphNodeSlot]
	
	func _init(p_slots: Array[CodeGraphNodeSlot] = []) -> void:
		slots = p_slots

class NodeElement:
	const column_width_px : int = 32
	var columns : int = 1
	var rows : int = 1
	var on_change : Callable

class NodeElementsRow:
	const row_height_px : int = 32
	var height : int = 1
	var elements : Array[NodeElement] = []
	
	func _init(p_height: int = 1, p_elements: Array[NodeElement] = []) -> void:
		height = p_height
		elements = p_elements
	
class NodeElementsGrid:
	var rows : Array[NodeElementsRow]
	
	func _init(p_rows: Array[NodeElementsRow] = []) -> void:
		rows = p_rows
#-------------------------------------------------------------------------------
class BoolNodeElement extends NodeElement:
	var is_true : bool = false
	
	func _init(p_is_true: bool = false):
		is_true = p_is_true
#-------------------------------------------------------------------------------
class LabelNodeElement extends NodeElement:
	var string : String = ""
	## if charset empty, no filter
	var charset : String = ""
	var color : Color
	var wrap_lines : bool = true
	
	func _init(p_string: String = "", p_charset: String = "", p_color: Color = Color("#fff"), p_wrap_lines: bool = false):
		string = p_string
		charset = p_charset
		color = p_color
		wrap_lines = p_wrap_lines

class TextInputLineElement extends LabelNodeElement:
	var placeholder : String = ""
	
	func _init(p_string: String = "", p_placeholder: String = "", p_charset: String = "", p_color: Color = Color("#fff"), p_wrap_lines: bool = false):
		super(p_string, p_charset, p_color, p_wrap_lines)
		placeholder = p_placeholder

class TextInputAreaElement extends LabelNodeElement:
	var placeholder : String = ""
	
	func _init(p_string: String = "", p_placeholder: String = "", p_charset: String = "", p_color: Color = Color("#fff"), p_wrap_lines: bool = false):
		super(p_string, p_charset, p_color, p_wrap_lines)
		placeholder = p_placeholder
#-------------------------------------------------------------------------------
class NumberInputElement extends TextInputLineElement:
	var is_int : bool = false
	func _init(p_number: float = 0, p_is_int: bool = false, p_placeholder: float = 0, p_color: Color = Color("#fff")):
		var num_charset : String = "0123456789."
		is_int = p_is_int
		if is_int:
			num_charset = "0123456789"
			
		super(str(p_number), str(p_placeholder), num_charset, p_color, false)
#-------------------------------------------------------------------------------
class SelectNodeElement extends LabelNodeElement:
	var selected_element_index : int = -1
	var elements : Array
	var to_str : Callable
