# code_graph_element.gd
@tool
class_name CodeGraphElement
extends GameDataEntry

var parent_graph : CodeGraph = null
var graph_id : int = -1
var is_valid : bool = true
#-------------------------------------------------------------------------------
@export var position_start : Vector2i = Vector2i(0, 0)
@export var position_end : Vector2i = Vector2i(0, 0)
#-------------------------------------------------------------------------------
func has_parent_graph() -> bool:
	return parent_graph != null

func set_parent_graph(p_graph: CodeGraph) -> void:
	parent_graph = p_graph

func get_parent_graph() -> CodeGraph:
	return parent_graph

func has_valid_graph_id() -> bool:
	return graph_id >= 0

func get_graph_id() -> int:
	return graph_id

func set_graph_id(p_id: int) -> void:
	graph_id = p_id
#-------------------------------------------------------------------------------
func validate() -> bool:
	is_valid = true
	return true
