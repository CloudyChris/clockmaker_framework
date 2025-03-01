/* game_data_collections.cpp */

#include "modules/clockmakers_framework/data_manager/game_data_collections.h"
#include "modules/clockmakers_framework/data_manager/game_data_manager.h"
#include "modules/clockmakers_framework/cm_enums.h"

void GameDataField::from_dict(Dictionary p_dict)
{
	type = cm_enums::CM_DataFieldType(int(p_dict["type"]));

	switch (type)
	{
		case cm_enums::CM_DataFieldType::CM_DATA_FIELD_TYPE_NONE:
			break;
		case cm_enums::CM_DataFieldType::CM_DATA_FIELD_TYPE_ENTRY:
		case cm_enums::CM_DataFieldType::CM_DATA_FIELD_TYPE_RESOURCE:
		case cm_enums::CM_DataFieldType::CM_DATA_FIELD_TYPE_BASIC:
			data = p_dict["data"];
			break;
	}
}

Dictionary GameDataField::to_dict()
{
	Dictionary r_dict;

	r_dict["type"] = int(type);
	r_dict["data"] = data;

	return r_dict;
}

GameDataEntry::GameDataEntry()
	: parent(nullptr)
	, path("")
{
}

GameDataEntry::GameDataEntry(const GameDataEntry &p_game_data_entry)
	: fields(p_game_data_entry.fields)
	, parent(p_game_data_entry.parent)
	, path(p_game_data_entry.path)
{
}

GameDataEntry::GameDataEntry(GameDataManager *p_manager, GameDataTable *p_parent)
	: parent(p_parent)
	, path("")
{
}

GameDataEntry::~GameDataEntry()
{
	parent = nullptr;

	fields.~VectorHashMapPair();
}

GameDataTable *GameDataEntry::get_parent() const
{
	return parent;
}

void GameDataEntry::set_parent(GameDataTable *p_parent)
{
	parent = p_parent;
}

String GameDataEntry::get_path() const
{
	return path;
}

void GameDataEntry::set_path(String p_path)
{
	path = p_path;
}

const GameDataField *GameDataEntry::get_one_const(String p_field_name) const
{
	return fields.get_one_const(p_field_name);
}

GameDataField *GameDataEntry::get_one(String p_field_name)
{
	return fields.get_one(p_field_name);
}

GameDataField *GameDataEntry::create_one(String p_field_name)
{
	return fields.create_one(p_field_name);
}

HashMap<String, GameDataField *> GameDataEntry::get_one_const(LocalVector<String> p_field_names) const
{
	return fields.get_const(p_field_names);
}

HashMap<String, GameDataField *> GameDataEntry::get_one(LocalVector<String> p_field_names)
{
	return fields.get(p_field_names);
}

bool GameDataEntry::can_merge(const GameDataEntry &p_game_data_entry) const
{
	return fields.can_merge(p_game_data_entry.fields);
}

bool GameDataEntry::merge(const GameDataEntry &p_game_data_entry)
{
	return fields.merge(p_game_data_entry.fields);
}

void GameDataEntry::merge_keep(const GameDataEntry &p_game_data_entry)
{
	fields.merge_keep(p_game_data_entry.fields);
	return;
}

void GameDataEntry::merge_override(const GameDataEntry &p_game_data_entry)
{
	fields.merge_override(p_game_data_entry.fields);
	return;
}

void GameDataEntry::replace(const GameDataEntry &p_game_data_entry)
{
	fields.replace(p_game_data_entry.fields);
	return;
}

void GameDataEntry::from_dict(Dictionary p_dict)
{
	set_uuid_from_string(String(p_dict["uuid"]));

	Dictionary fields_dict = Dictionary(p_dict["fields"]);
	Array l_keys = fields_dict.keys();

	String key;
	Dictionary f_dict;

	GameDataField l_field, *r_field = nullptr;

	for (int i = 0; i < l_keys.size(); i++)
	{
		key = String(l_keys[i]);
		f_dict = Dictionary(fields_dict[key]);
		if (f_dict.is_empty())
		{
			ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
			continue;
		}

		l_field = GameDataField();
		l_field.from_dict(f_dict);
		l_field.name = key;

		r_field = nullptr;

		if (!fields.has(key))
		{
			r_field = fields.create_one(key);
		}
		else
		{
			r_field = fields.get_one(key);
			if (!r_field)
			{
				fields.delete_one(key);
				r_field = fields.create_one(key);
			}
		}

		r_field->name = l_field.name;
		r_field->type = l_field.type;
		r_field->data = l_field.data;
	}
}

Dictionary GameDataEntry::to_dict()
{
	Dictionary r_dict;
	r_dict["uuid"] = uuid.get_uuid_string_bind();

	Dictionary fields_dict;

	HashMap<String, GameDataField *> l_fields = fields.get_const();

	for (KeyValue<String, GameDataField *> kv : l_fields)
	{
		if (!kv.value)
		{
			ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
			continue;
		}

		fields_dict[kv.key] = kv.value->to_dict();
	}

	r_dict["fields"] = fields_dict;

	return r_dict;
}

GameDataTable::GameDataTable()
	: table_specification(nullptr)
	, parent(nullptr)
	, path("")
{
}

GameDataTable::GameDataTable(const GameDataTable &p_game_data_table)
	: entries(p_game_data_table.entries)
	, table_specification(p_game_data_table.table_specification)
	, parent(p_game_data_table.parent)
	, path(p_game_data_table.path)
{
}

GameDataTable::GameDataTable(GameDataCollection *p_parent, TableSpecification *p_table_specificaiton)
	: table_specification(p_table_specificaiton)
	, parent(p_parent)
	, path("")
{
}

GameDataTable::~GameDataTable()
{
	table_specification = nullptr;
	parent = nullptr;
	entries.~VectorHashMapPair();
}

TableSpecification *GameDataTable::get_table_specification() const
{
	return table_specification;
}

void GameDataTable::set_table_specification(TableSpecification *p_table_specification)
{
	table_specification = p_table_specification;
}

GameDataCollection *GameDataTable::get_parent() const
{
	return parent;
}

void GameDataTable::set_parent(GameDataCollection *p_parent)
{
	parent = p_parent;
	return;
}

String GameDataTable::get_name() const
{
	return name;
}

void GameDataTable::set_name(String p_name)
{
	if (p_name.is_empty())
	{
		return;
	}

	name = p_name;
	return;
}

String GameDataTable::get_path() const
{
	return path;
}

void GameDataTable::set_path(String p_path)
{
	path = p_path;
}

bool GameDataTable::has(String p_uuid) const
{
	return entries.has(p_uuid);
}

const GameDataEntry *GameDataTable::get_one_const(String p_uuid) const
{
	return entries.get_one_const(p_uuid);
}

GameDataEntry *GameDataTable::get_one(String p_uuid)
{
	return entries.get_one(p_uuid);
}

GameDataEntry *GameDataTable::create_one(String p_uuid)
{
	return entries.create_one(p_uuid);
}

bool GameDataTable::delete_one(String p_uuid)
{
	return entries.delete_one(p_uuid);
}

HashMap<String, GameDataEntry *> GameDataTable::get_const(Vector<String> p_uuids) const
{
	return entries.get_const(p_uuids);
}

HashMap<String, GameDataEntry *> GameDataTable::get(Vector<String> p_uuids)
{
	return entries.get(p_uuids);
}

bool GameDataTable::can_merge(const GameDataTable &p_game_data_table) const
{
	return entries.can_merge(p_game_data_table.entries);
}

bool GameDataTable::merge(const GameDataTable &p_game_data_table)
{
	return entries.merge(p_game_data_table.entries);
}

void GameDataTable::merge_keep(const GameDataTable &p_game_data_table)
{
	entries.merge_keep(p_game_data_table.entries);
	return;
}

void GameDataTable::merge_override(const GameDataTable &p_game_data_table)
{
	entries.merge_override(p_game_data_table.entries);
	return;
}

void GameDataTable::replace(const GameDataTable &p_game_data_table)
{
	entries.replace(p_game_data_table.entries);
	return;
}

void GameDataTable::from_dict(Dictionary p_dict)
{
	set_name(String(p_dict["name"]));
	Dictionary entries_dict = Dictionary(p_dict["entries"]);
	Array l_keys = entries_dict.keys();

	String key;
	Dictionary game_data_entry_dict;

	GameDataEntry *r_entry = nullptr;

	for (int i = 0; i < l_keys.size(); i++)
	{
		key = String(l_keys[i]);

		if (p_dict[key].get_type() == Variant::Type::STRING)
		{
			if (String(p_dict[key]).is_empty())
			{
				ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
				continue;
			}
		}
		else if (p_dict[key].get_type() == Variant::Type::DICTIONARY)
		{
			game_data_entry_dict = Dictionary(p_dict[key]);
			if (game_data_entry_dict.is_empty())
			{
				ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
				continue;
			}
		}
		else
		{
			ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
			continue;
		}

		r_entry = nullptr;

		if (!entries.has(key))
		{
			r_entry = entries.create_one(key);
		}
		else
		{
			r_entry = entries.get_one(key);

			if (!r_entry)
			{
				entries.delete_one(key);
				r_entry = entries.create_one(key);
			}
		}

		r_entry->set_uuid(UUID(key).get_uuid_bind());
		r_entry->set_parent(this);

		r_entry->set_path(String(p_dict[key]));
	}
}

Dictionary GameDataTable::to_dict()
{
	Dictionary r_dict;

	r_dict["name"] = table_specification->name;

	Dictionary entries_dict;

	HashMap<String, GameDataEntry *> l_entries = entries.get_const();

	for (KeyValue<String, GameDataEntry *> kv : l_entries)
	{
		if (!kv.value)
		{
			continue;
		}

		entries_dict[kv.key] = kv.value->get_path();
	}

	r_dict["entries"] = entries_dict;

	return r_dict;
}

GameDataCollection::GameDataCollection()
	: collection_type(cm_enums::CM_DataCollectionType::CM_DATA_COLLECTION_TYPE_NONE)
{
}

GameDataCollection::GameDataCollection(const GameDataCollection &p_game_data_collection)
	: tables(p_game_data_collection.tables)
	, collection_type(p_game_data_collection.collection_type)
{
}

GameDataCollection::~GameDataCollection()
{
	tables.~VectorHashMapPair();
}

String GameDataCollection::get_name() const
{
	return name;
}

void GameDataCollection::set_name(String p_name)
{
	if (p_name.is_empty())
	{
		return;
	}

	name = p_name;
}

String GameDataCollection::get_path() const
{
	return path;
}

void GameDataCollection::set_path(String p_path)
{
	path = p_path;
}

cm_enums::CM_DataCollectionType GameDataCollection::get_collection_type() const
{
	return collection_type;
}

void GameDataCollection::set_collection_type(cm_enums::CM_DataCollectionType p_collection_type)
{
	collection_type = p_collection_type;
}

bool GameDataCollection::has(String p_table_name) const
{
	return tables.has(p_table_name);
}

const GameDataTable *GameDataCollection::get_one_const(String p_table_name) const
{
	return tables.get_one_const(p_table_name);
}

GameDataTable *GameDataCollection::get_one(String p_table_name)
{
	return tables.get_one(p_table_name);
}

GameDataTable *GameDataCollection::create_one(String p_table_name)
{
	if (!has(p_table_name))
	{
		return tables.create_one(p_table_name);
	}

	return tables.get_one(p_table_name);
}

bool GameDataCollection::delete_one(String p_table_name)
{
	return tables.delete_one(p_table_name);
}

HashMap<String, GameDataTable *> GameDataCollection::get_const(Vector<String> p_table_names) const
{
	return tables.get_const(p_table_names);
}

HashMap<String, GameDataTable *> GameDataCollection::get(Vector<String> p_table_names)
{
	return tables.get(p_table_names);
}

bool GameDataCollection::can_merge(const GameDataCollection &p_game_data_collection) const
{
	return tables.can_merge(p_game_data_collection.tables);
}

bool GameDataCollection::merge(const GameDataCollection &p_game_data_collection)
{
	return tables.merge(p_game_data_collection.tables);
}

void GameDataCollection::merge_keep(const GameDataCollection &p_game_data_collection)
{
	tables.merge_keep(p_game_data_collection.tables);
	return;
}

void GameDataCollection::merge_override(const GameDataCollection &p_game_data_collection)
{
	tables.merge_override(p_game_data_collection.tables);
	return;
}

void GameDataCollection::replace(const GameDataCollection &p_game_data_collection)
{
	tables.replace(p_game_data_collection.tables);
	return;
}

void GameDataCollection::from_dict(Dictionary p_dict)
{
	set_uuid_from_string(String(p_dict["uuid"]));
	set_name(String(p_dict["name"]));

	Dictionary tables_dict = Dictionary(p_dict["tables"]);
	Array l_keys = tables_dict.keys();

	String key;
	Dictionary table_pair_dict, t_spec_dict, table_dict;
	TableSpecification t_spec;
	GameDataTable *r_table = nullptr;

	for (int i = 0; i < l_keys.size(); i++)
	{
		key = l_keys[i];

		table_pair_dict = Dictionary(tables_dict[key]);
		if (table_pair_dict.is_empty())
		{
			ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
			continue;
		}

		t_spec_dict.clear();
		t_spec = TableSpecification();
		DataInfo t_spec_info;
		t_spec_info.data_type = cm_enums::CM_DataType::CM_DATA_TYPE_TABLE_SPECIFICATION;
		t_spec_info.table = key;
		if (table_pair_dict["table_specification"].get_type() == Variant::Type::STRING)
		{
			if (String(table_pair_dict["table_specification"]).is_empty())
			{
				ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
				continue;
			}
			t_spec.path = String(table_pair_dict["table_specification"]);
			DataInfo data_info;
			data_info.data_type = cm_enums::CM_DataType::CM_DATA_TYPE_TABLE_SPECIFICATION;
			data_info.path = t_spec.path;
			data_info.table = key;
			GameDataManager::load(data_info);
		}
		else if (table_pair_dict["table_specification"].get_type() == Variant::Type::DICTIONARY)
		{
			t_spec_dict = Dictionary(table_pair_dict["table_specification"]);
			if (t_spec_dict.is_empty())
			{
				ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
				continue;
			}
			t_spec.from_dict(t_spec_dict);
			GameDataManager::set_data_table_specification(t_spec_info, t_spec);
		}
		else
		{
			continue;
		}

		table_dict.clear();

		if (table_pair_dict["table"].get_type() == Variant::Type::STRING)
		{
			if (String(table_pair_dict["table"]).is_empty())
			{
				ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
				continue;
			}
		}
		else if (table_pair_dict["table"].get_type() == Variant::Type::DICTIONARY)
		{
			table_dict = Dictionary(table_pair_dict["table"]);
			if (table_dict.is_empty())
			{
				ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
				continue;
			}
		}
		else
		{
			ERR_PRINT_ED("[ ERROR ] Something's wrong with serialized data");
			continue;
		}

		r_table = nullptr;

		if (!tables.has(key))
		{
			r_table = tables.create_one(key);
		}
		else
		{
			r_table = tables.get_one(key);

			if (!r_table)
			{
				tables.delete_one(key);
				r_table = tables.create_one(key);
			}
		}

		r_table->set_name(key);
		r_table->set_table_specification(GameDataManager::get_data_table_specification(t_spec_info));
		r_table->set_parent(this);

		r_table->set_path(String(table_pair_dict["table"]));
	}
}

Dictionary GameDataCollection::to_dict()
{
	Dictionary r_dict;

	r_dict["uuid"] = uuid.get_uuid_string_bind();
	r_dict["name"] = name;

	Dictionary tables_dict, t_dict;

	HashMap<String, GameDataTable *> l_tables = tables.get_const();

	for (KeyValue<String, GameDataTable *> kvt : l_tables)
	{
		if (!kvt.value)
		{
			continue;
		}

		t_dict.clear();

		if (TableSpecification *t_spec = kvt.value->get_table_specification())
		{
			t_dict["table_specification"] = t_spec->path;
		}
		else
		{
			ERR_PRINT_ED("Table does not have a specification at save time");
			t_dict["table_specification"] = "";
		}

		t_dict["table"] = kvt.value->get_path();


		tables_dict[kvt.key] = t_dict;
	}

	r_dict["tables"] = tables_dict;

	return r_dict;
}

