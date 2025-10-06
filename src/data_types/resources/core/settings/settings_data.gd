# settings_data_entry.gd
@tool
class_name SettingsData
extends ManagerData

@export var settings : Dictionary[String, SettingsDataField] = {}
@export var settings_changed : Dictionary[String, SettingsDataField]
#-------------------------------------------------------------------------------
func get_field(p_field_name: String) -> SettingsDataField:
	return settings.get(p_field_name)

func set_field(p_field_name: String, p_field: SettingsDataField) -> void:
	settings.set(p_field_name, p_field)

func remove_field(p_field_name: String) -> bool:
	settings_changed.erase(p_field_name)
	return settings.erase(p_field_name)
#-------------------------------------------------------------------------------
func get_field_ui(p_field_name: String) -> SettingsDataField:
	var settings_field : SettingsDataField = settings_changed.get(p_field_name)
	if not settings_field:
		settings_field = settings.get(p_field_name)
	return settings_field

func set_field_ui(p_field_name: String, p_field: SettingsDataField) -> void:
	if settings.has(p_field_name):
		settings_changed.set(p_field_name, p_field)
#-------------------------------------------------------------------------------
func has_changes() -> bool:
	return not settings_changed.is_empty()

func revert() -> void:
	settings_changed.clear()

func apply() -> void:
	var settings_field_to_apply : SettingsDataField = null
	for changed_field_key in settings_changed.keys():
		settings_field_to_apply = settings_changed.get(changed_field_key)
		if settings_field_to_apply:
			settings_field_to_apply.apply()
		settings.set(changed_field_key, settings_field_to_apply)
	settings_changed.clear()
