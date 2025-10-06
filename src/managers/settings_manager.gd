# settings_manager.gd
extends BaseManager

var data : SettingsData
#-------------------------------------------------------------------------------
static func matches_mod_keys(p_action_name: String) -> bool:
	var event : InputEvent = InputMap.action_get_events(p_action_name)[0]
	var mod_mask : int = event.get_modifiers_mask()
	var mods : int = 0
	if Input.is_physical_key_pressed(KEY_ALT):
		mods += KEY_MASK_ALT
	if Input.is_physical_key_pressed(KEY_CTRL):
		mods += KEY_MASK_CTRL
	if Input.is_physical_key_pressed(KEY_SHIFT):
		mods += KEY_MASK_SHIFT
	if Input.is_physical_key_pressed(KEY_META):
		mods += KEY_MASK_META
	
	if mods == mod_mask:
		return true
	
	return false

static func is_action_just_released(p_action_name: String) -> bool:
	return Input.is_action_just_released(p_action_name) and matches_mod_keys(p_action_name)

static func is_action_pressed(p_action_name: String) -> bool:
	return Input.is_action_pressed(p_action_name) and matches_mod_keys(p_action_name)
#-------------------------------------------------------------------------------
func load_manager() -> bool:
	return true

func reset_manager() -> bool:
	return true
#-------------------------------------------------------------------------------
func get_field(p_field_name: String) -> SettingsDataField:
	if data:
		return data.get_field(p_field_name)
	else:
		push_error("No settings data")
		return null

func get_field_ui(p_field_name: String) -> SettingsDataField:
	if data:
		return data.get_field_ui(p_field_name)
	else:
		push_error("No settings data")
		return null

func set_field(p_field_name: String, p_field: SettingsDataField) -> void:
	if data:
		data.set_field(p_field_name, p_field)
	else:
		push_error("No settings data")

func set_field_ui(p_field_name: String, p_field: SettingsDataField) -> void:
	if data:
		data.set_field_ui(p_field_name, p_field)
	else:
		push_error("No settings data")

func remove_field(p_field_name: String) -> bool:
	if data:
		return data.remove_field(p_field_name)
	else:
		push_error("No settings data")
		return false
#-------------------------------------------------------------------------------
func has_changes() -> bool:
	if data:
		return data.has_changes()
	else:
		push_error("No settings data")
		return false

func revert() -> void:
	if data:
		data.revert()
	else:
		push_error("No settings data")

func apply_settings() -> void:
	if data:
		data.apply()
	else:
		push_error("No settings data")
