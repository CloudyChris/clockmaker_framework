/* game_data_specifications.h */

#ifndef GAME_DATA_SPECIFICATIONS_H
#define GAME_DATA_SPECIFICATIONS_H

#pragma region godot_includes
#include "../../../core/string/ustring.h"
#include "../../../core/variant/dictionary.h"
#pragma endregion godot_includes

#pragma region cm_includes
#include "../cm_enums.h"
#include "../vector_hashmap_pair.h"
#pragma endregion cm_includes

class GameDataTable;

// FieldSpecification serialization
// JSON
//---------------------------------
// {
// 	name : <String>,
// 	type : <CM_DataFieldType>,
// 	load_policy : <CM_DataLoadPolicy>,
// }
//---------------------------------
struct FieldSpecification
{
	cm_enums::CM_DataFieldType type = cm_enums::CM_DataFieldType::CM_DATA_FIELD_TYPE_NONE;
	String name = "";

	void from_dict(Dictionary p_dict);
	Dictionary to_dict() const;

	FieldSpecification();
	~FieldSpecification();
};

// TableSpecification serialization
// JSON
//---------------------------------
// {
// 	name: <String>,
// 	load_policy: <CM_DataLoadPolicy>
// 	fields: {
// 		field_name: FieldSpecification,
// 		.
// 		.
// 	}
// }
//---------------------------------
struct TableSpecification
{
	String name = "";
	String path = "";

	VectorHashMapPair<String, FieldSpecification> fields;

	void from_dict(Dictionary p_dict);
	Dictionary to_dict() const;

	TableSpecification();
	~TableSpecification();
};
#endif // GAME_DATA_SPECIFICATIONS_H
