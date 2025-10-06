class_name StringUtils
extends Object

static func str_split(p_string: String, p_delimiter: String = "", p_allow_empty: bool = true, p_maxsplit: int = 0) -> Array[String]:
	return Array(Array(p_string.split(p_delimiter, p_allow_empty, p_maxsplit)), TYPE_STRING, "", null)
