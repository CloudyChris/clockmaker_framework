/* vector_hashmap_pair.h */

// FORGET THIS WE'LL USE RID_Owner instead of a vector I'M SO SMART

#ifndef VECTOR_HASHMAP_PAIR_H
#define VECTOR_HASHMAP_PAIR_H

#include "core/templates/hash_map.h"
#include "core/templates/vector.h"

// DEBUG:  Crashes if TValue is not default constructible and trivially destructible
template <typename TKey, typename TValue>
class VectorHashMapPair
{
private:
	Vector<TValue> values;
	HashMap<TKey, uint32_t> values_cache;

public:
	bool is_empty() const;

	bool has(TKey p_key) const;

	const TValue *get_one_const(TKey p_key) const;
	TValue *get_one(TKey p_key);
	TValue *create_one(TKey p_key);
	bool delete_one(TKey p_key);

	HashMap<TKey, TValue *> get_const(Vector<TKey> p_keys = Vector<TKey>()) const;
	HashMap<TKey, TValue *> get(Vector<TKey> p_keys = Vector<TKey>());

	bool can_merge(const VectorHashMapPair<TKey, TValue> &p_vhm_pair) const;
	bool merge(const VectorHashMapPair<TKey, TValue> &p_vhm_pair);
	void merge_keep(const VectorHashMapPair<TKey, TValue> &p_vhm_pair);
	void merge_override(const VectorHashMapPair<TKey, TValue> &p_vhm_pair);

	void replace(const VectorHashMapPair<TKey, TValue> &p_vhm_pair);

	VectorHashMapPair();
	VectorHashMapPair(const VectorHashMapPair<TKey, TValue> &p_vhm_pair);
	~VectorHashMapPair();
};

#endif // VECTOR_HASHMAP_PAIR_H
