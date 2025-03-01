/* game_data_specifications.cpp */

#include "modules/clockmakers_framework/data_manager/game_data_specifications.h"
#include "core/variant/variant.h"

FieldSpecification::FieldSpecification()
{
}

FieldSpecification::~FieldSpecification()
{
}

void FieldSpecification::from_dict(Dictionary p_dict)
{
	if (!p_dict.has("type"))
	{
		ERR_PRINT_ED("Field Specification dictionary doesn't have required fields");
		return;
	}

	type = cm_enums::CM_DataFieldType(int(p_dict["type"]));
}

Dictionary FieldSpecification::to_dict() const
{
	Dictionary r_dict;

	r_dict["type"] = type;

	return r_dict;
}

TableSpecification::TableSpecification()
{
}

TableSpecification::~TableSpecification()
{
	fields.~VectorHashMapPair();
}

void TableSpecification::from_dict(Dictionary p_dict)
{
	name = p_dict["name"];

	Dictionary l_fields = Dictionary(p_dict["fields"]);
	Array l_keys = l_fields.keys();

	String key;
	Dictionary f_spec_dict;
	FieldSpecification l_field;
	FieldSpecification *r_field = nullptr;

	for (int i = 0; i < l_keys.size(); i++)
	{
		key = l_keys[i];
		if (l_fields[key].get_type() != Variant::Type::DICTIONARY)
		{
			continue;
		}
		f_spec_dict = Dictionary(l_fields[key]);
		if (f_spec_dict.is_empty())
		{
			continue;
		}

		l_field = FieldSpecification();
		l_field.from_dict(f_spec_dict);
		l_field.name = key;

		r_field = nullptr;

		if (fields.has(l_field.name))
		{
			r_field = fields.get_one(l_field.name);
		}

		if (!r_field)
		{
			r_field = fields.create_one(l_field.name);
		}

		r_field->name = l_field.name;
		r_field->type = l_field.type;
	}
}

Dictionary TableSpecification::to_dict() const
{
	Dictionary r_dict;

	r_dict["name"] = name;

	Dictionary l_fields_dictionary;

	HashMap<String, FieldSpecification *> l_fields = fields.get_const();

	for (KeyValue<String, FieldSpecification *> kv : l_fields)
	{
		if (!kv.value)
		{
			continue;
		}

		l_fields_dictionary[kv.value->name] = kv.value->to_dict();
	}

	r_dict["fields"] = l_fields_dictionary;

	return r_dict;
}
