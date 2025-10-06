@tool
class_name SceneUIElement
extends UIElement

@export var scene_path : String = ""

func build_ui() -> void:
	if ResourceLoader.exists(scene_path):
		ui_tree = load(scene_path)
