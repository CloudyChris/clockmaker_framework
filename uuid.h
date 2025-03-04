/* uuid.h */

#ifndef UUID_H
#define UUID_H

#pragma region godot_includes
#include "../../core/object/class_db.h"
#include "../../core/variant/variant.h"
#include "../../core/object/object.h"
#pragma endregion godot_includes

class UUID
{
private:
	static const size_t uuid_size = 16;
	static const size_t uuid_string_size = 36;
	uint8_t *uuid;
	char *uuid_string;

	void _generate_uuid();
	void _update_uuid();
	void _update_uuid_string();

public:
	static UUID empty();

	bool is_empty() const;

	char *get_uuid_string();
	char *get_uuid_string_const() const;
	void set_uuid_string(char *p_uuid_string);

	String get_uuid_string_bind() const;
	void set_uuid_string_bind(String p_uuid_string);

	uint8_t *get_uuid();
	uint8_t *get_uuid_const() const;
	void set_uuid(uint8_t *p_uuid);

	PackedByteArray get_uuid_bind() const;
	void set_uuid_bind(PackedByteArray p_uuid);

	void operator=(const UUID &p_uuid);

	UUID();
	UUID(const UUID &p_uuid);
	UUID(uint8_t *p_uuid);
	UUID(PackedByteArray p_uuid);
	UUID(char *p_uuid_string);
	UUID(String p_uuid_string);
	~UUID();
};

class Tracked
{
protected:
	UUID uuid;

public:
	static Tracked empty();

	const UUID &get_uuid() const;
	UUID &get_uuid_m();
	PackedByteArray get_uuid_bind() const;
	String get_uuid_string() const;
	void set_uuid(const UUID &p_uuid);
	void set_uuid_bind(PackedByteArray p_uuid);
	void set_uuid_from_string(String p_uuid_string);

	Tracked();
	Tracked(const Tracked &p_tracked);
	~Tracked();
};

class TrackedObject : public Object
{
	GDCLASS(TrackedObject, Object);

protected:
	UUID uuid;

protected:
	static void _bind_methods();

public:
	static TrackedObject empty();

	const UUID &get_uuid() const;
	UUID &get_uuid_m();
	PackedByteArray get_uuid_bind() const;
	String get_uuid_string() const;
	void set_uuid(const UUID &p_uuid);
	void set_uuid_bind(PackedByteArray p_uuid);
	void set_uuid_from_string(String p_uuid_string);

	TrackedObject();
	TrackedObject(const TrackedObject &p_tracked_object);
	~TrackedObject();
};

#endif
