# property_info.gd
# this just reflects the PropertyInfo from the engine, I just like type safety
@tool
class_name PropertyInfo
extends RefCounted

@export var name : String = ""
@export var type : Variant.Type = TYPE_NIL
@export var hint : PropertyHint = PROPERTY_HINT_NONE
@export var hint_string : String = ""
#-------------------------------------------------------------------------------
func _init(p_name: String = "", p_type: Variant.Type = TYPE_NIL, p_hint: PropertyHint = PROPERTY_HINT_NONE, p_hint_string: String = ""):
	name = p_name
	type = p_type
	hint = p_hint
	hint_string = p_hint_string 
