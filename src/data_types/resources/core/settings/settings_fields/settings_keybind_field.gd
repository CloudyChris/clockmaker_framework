@tool
class_name SettingsKeybindField
extends SettingsDataField

@export var action_name : String = ""
@export_range(0, 1, 0.1) var deadzone : float = -1
@export var keybinds : Array[InputEventKey] = []

func apply() -> void:
	if not InputMap.has_action(action_name):
		InputMap.add_action(action_name, deadzone)
	else:
		InputMap.action_set_deadzone(action_name, deadzone)
	InputMap.action_erase_events(action_name)
	for keybind in keybinds:
		InputMap.action_add_event(action_name, keybind)
