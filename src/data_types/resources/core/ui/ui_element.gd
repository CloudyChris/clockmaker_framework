@tool
class_name UIElement
extends GameDataEntry

var ui_tree : PackedScene = null
#-------------------------------------------------------------------------------
## virtual
func build_ui() -> void:
	pass
#-------------------------------------------------------------------------------
func get_ui() -> PackedScene:
	return ui_tree
