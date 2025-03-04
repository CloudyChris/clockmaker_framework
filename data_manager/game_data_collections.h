/* game_data_collections.h */

#ifndef GAME_DATA_COLLECTIONS_H
#define GAME_DATA_COLLECTIONS_H

#pragma region godot_includes
#include "../../../core/templates/hash_map.h"
#include "../../../core/templates/local_vector.h"
#pragma endregion godot_includes

#pragma region cm_includes
#include "game_data_specifications.h"
#include "../cm_enums.h"
#include "../uuid.h"
#include "../vector_hashmap_pair.h"
#pragma endregion cm_includes

class GameDataCollection;
class GameDataEntry;
class GameDataTable;
class GameDataManager;

// Field serialization
// JSON
//---------------------------------
// {
// 	type : <int CM_DataFieldType>,
// 	entry_uuid : <uuid string>,
// 	resource_path : <String>,
// 	data : <Variant>
// }
//---------------------------------
struct GameDataField
{
	cm_enums::CM_DataFieldType type = cm_enums::CM_DataFieldType::CM_DATA_FIELD_TYPE_NONE;
	Variant data;

	String name = "";

	void from_dict(Dictionary p_dict);
	Dictionary to_dict();
};

// Entry serialization
// JSON
//---------------------------------
// {
// 	uuid : <String>,
// 	fields : {
// 		field_name : GameDataField,
// 		.
// 		.
// 	}
// }
//---------------------------------
class GameDataEntry : public Tracked
{
private:
	VectorHashMapPair<String, GameDataField> fields;
	GameDataTable *parent;
	String path;

public:
	GameDataTable *get_parent() const;
	void set_parent(GameDataTable *p_parent);

	String get_path() const;
	void set_path(String p_path);

	bool has(String p_field_name) const;

	const GameDataField *get_one_const(String p_field_name) const;
	GameDataField *get_one(String p_field_name);
	GameDataField *create_one(String p_field_name);
	bool delete_one(String p_field_name);

	HashMap<String, GameDataField *> get_const(TightLocalVector<String> p_field_names = TightLocalVector<String>()) const;
	HashMap<String, GameDataField *> get(TightLocalVector<String> p_field_names = TightLocalVector<String>());

	bool can_merge(const GameDataEntry &p_game_data_entry) const;
	bool merge(const GameDataEntry &p_game_data_entry);
	void merge_keep(const GameDataEntry &p_game_data_entry);
	void merge_override(const GameDataEntry &p_game_data_entry);

	void replace(const GameDataEntry &p_game_data_entry);

	void from_dict(Dictionary p_dict);
	Dictionary to_dict();

	GameDataEntry();
	GameDataEntry(const GameDataEntry &p_game_data_entry);
	GameDataEntry(GameDataManager *p_manager, GameDataTable *p_parent);
	~GameDataEntry();
};

// Table serialization
// JSON
//---------------------------------
// {
// 	name : <String>,
// 	entries: {
// 		uuid_string : GameDataEntry or path,
// 		.
// 		.
// 	}
// }
//---------------------------------
class GameDataTable
{
private:
	VectorHashMapPair<String, GameDataEntry> entries;
	TableSpecification *table_specification;
	GameDataCollection *parent;

	String name;
	String path;

public:
	String get_name() const;
	void set_name(String p_name);

	TableSpecification *get_table_specification() const;
	void set_table_specification(TableSpecification *p_table_specificaiton);

	GameDataCollection *get_parent() const;
	void set_parent(GameDataCollection *p_parent);

	String get_path() const;
	void set_path(String p_path);

	bool has(String p_uuid) const;

	const GameDataEntry *get_one_const(String p_uuid) const;
	GameDataEntry *get_one(String p_uuid);
	GameDataEntry *create_one(String p_uuid);
	bool delete_one(String p_uuid);

	HashMap<String, GameDataEntry *> get_const(TightLocalVector<String> p_uuids = TightLocalVector<String>()) const;
	HashMap<String, GameDataEntry *> get(TightLocalVector<String> p_uuids = TightLocalVector<String>());

	bool can_merge(const GameDataTable &p_game_data_table) const;
	bool merge(const GameDataTable &p_game_data_table);
	void merge_keep(const GameDataTable &p_game_data_table);
	void merge_override(const GameDataTable &p_game_data_table);

	void replace(const GameDataTable &p_game_data_table);

	void from_dict(Dictionary p_dict);
	Dictionary to_dict();

	GameDataTable();
	GameDataTable(const GameDataTable &p_game_data_table);
	GameDataTable(GameDataCollection *p_parent, TableSpecification *p_table_specification);
	~GameDataTable();
};

// Collection serialization
// JSON
// {
// 	uuid : <UUID String>
// 	name : <String>,
// 	tables : {
// 		table_name : {
// 			table_specification: TableSpecification or path,
// 			table: GameDataTable or path
// 		},
// 		.
// 		.
// 	},
// }
//---------------------------------
class GameDataCollection : public Tracked
{
private:
	String name;
	String path;
	VectorHashMapPair<String, GameDataTable> tables;
	cm_enums::CM_DataCollectionType collection_type;

public:
	String get_name() const;
	void set_name(String p_name);

	String get_path() const;
	void set_path(String p_path);

	cm_enums::CM_DataCollectionType get_collection_type() const;
	void set_collection_type(cm_enums::CM_DataCollectionType p_collection_type);

	bool has(String p_table_name) const;

	const GameDataTable *get_one_const(String p_table_name) const;
	GameDataTable *get_one(String p_table_name);
	GameDataTable *create_one(String p_table_name);
	bool delete_one(String p_table_name);

	HashMap<String, GameDataTable *> get_const(TightLocalVector<String> p_table_names = TightLocalVector<String>()) const;
	HashMap<String, GameDataTable *> get(TightLocalVector<String> p_table_names = TightLocalVector<String>());

	bool can_merge(const GameDataCollection &p_game_data_collection) const;
	bool merge(const GameDataCollection &p_game_data_collection);
	void merge_keep(const GameDataCollection &p_game_data_collection);
	void merge_override(const GameDataCollection &p_game_data_collection);

	void replace(const GameDataCollection &p_game_data_collection);

	// resolve is only for io operations (load from file, save to file)
	void from_dict(Dictionary p_dict);
	Dictionary to_dict();

	GameDataCollection();
	GameDataCollection(const GameDataCollection &p_game_data_collection);
	~GameDataCollection();
};
#endif // GAME_DATA_COLLECTIONS_H
