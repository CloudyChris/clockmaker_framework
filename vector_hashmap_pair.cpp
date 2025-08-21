/* vector_hashmap_pair.cpp */

#include "vector_hashmap_pair.h"

#include "core/error/error_macros.h"

template <typename TKey, typename TValue>
VectorHashMapPair<TKey, TValue>::VectorHashMapPair()
{
	CRASH_COND_MSG(!std::is_default_constructible_v<TValue>, "TValue type is not default constructible");
	CRASH_COND_MSG(!std::is_trivially_destructible_v<TValue>, "TValue type is not trivially destructible");
}

template <typename TKey, typename TValue>
VectorHashMapPair<TKey, TValue>::VectorHashMapPair(const VectorHashMapPair<TKey, TValue> &p_vhm_pair)
	: values(p_vhm_pair.values)
	, values_cache(p_vhm_pair.values_cache)
{
}

template <typename TKey, typename TValue>
VectorHashMapPair<TKey, TValue>::~VectorHashMapPair()
{
	values.clear();
	values_cache.clear();
}

template <typename TKey, typename TValue>
bool VectorHashMapPair<TKey, TValue>::is_empty() const
{
	return values.is_empty();
}

template <typename TKey, typename TValue>
bool VectorHashMapPair<TKey, TValue>::has(TKey p_key) const
{
	return values_cache.has(p_key);
}

template <typename TKey, typename TValue>
const TValue *VectorHashMapPair<TKey, TValue>::get_one_const(TKey p_key) const
{
	if (!values_cache.has(p_key))
	{
		ERR_PRINT_ED("Key does not exist");
		return nullptr;
	}

	uint64_t values_index = values_cache.get(p_key);

	if (values_index >= values.size())
	{
		ERR_PRINT_ED("Values cache index error");
		return nullptr;
	}

	return &values[values_index];
}

template <typename TKey, typename TValue>
TValue *VectorHashMapPair<TKey, TValue>::get_one(TKey p_key)
{
	if (!values_cache.has(p_key))
	{
		ERR_PRINT_ED("Key does not exist");
		return nullptr;
	}

	uint64_t values_index = values_cache.get(p_key);

	if (values_index >= values.size())
	{
		ERR_PRINT_ED("Values cache index error");
		return nullptr;
	}

	return &values[values_index];
}

template <typename TKey, typename TValue>
TValue *VectorHashMapPair<TKey, TValue>::create_one(TKey p_key)
{
	uint64_t new_index = values.size();

	if (holes.head)
	{
		new_index = holes.pop_head();
	}

	values.push_back(TValue());
	values_cache.insert(p_key, new_index);

	return &values[new_index];
}

template <typename TKey, typename TValue>
bool VectorHashMapPair<TKey, TValue>::delete_one(TKey p_key)
{
	if (!has(p_key))
	{
		ERR_PRINT_ED("Key does not exist");
		return false;
	}

	uint8_t values_index = values_cache.get(p_key);
	values_cache.erase(p_key);
	holes.prepend(values_index);

	return true;
}

template <typename TKey, typename TValue>
HashMap<TKey, TValue *> VectorHashMapPair<TKey, TValue>::get_const(Vector<TKey> p_keys) const
{
	HashMap<TKey, TValue> r_pairs;
	if (!p_keys.is_empty())
	{
		for (TKey key : p_keys)
		{
			r_pairs.insert(key, get_one_const(key));
		}
	}
	else
	{
		for (KeyValue<TKey, uint64_t> kv : values_cache)
		{
			r_pairs.insert(kv.key, get_one_const(kv.key));
		}
	}
}

template <typename TKey, typename TValue>
HashMap<TKey, TValue *> VectorHashMapPair<TKey, TValue>::get(Vector<TKey> p_keys)
{
	HashMap<TKey, TValue> r_pairs;
	if (!p_keys.is_empty())
	{
		for (TKey key : p_keys)
		{
			r_pairs.insert(key, get_one(key));
		}
	}
	else
	{
		for (KeyValue<TKey, uint64_t> kv : values_cache)
		{
			r_pairs.insert(kv.key, get_one(kv.key));
		}
	}
}
template <typename TKey, typename TValue>
bool VectorHashMapPair<TKey, TValue>::can_merge(const VectorHashMapPair<TKey, TValue> &p_vhm_pair) const
{
	HashMap<TKey, TValue> l_values_cache = values_cache;

	for (KeyValue<TKey, TValue> kv : p_vhm_pair.values_cache)
	{
		if (l_values_cache.has(kv.key))
		{
			return false;
		}
	}

	return true;
}

template <typename TKey, typename TValue>
bool VectorHashMapPair<TKey, TValue>::merge(const VectorHashMapPair<TKey, TValue> &p_vhm_pair)
{
	if (!can_merge(&p_vhm_pair))
	{
		return false;
	}

	TValue *new_entry = nullptr;

	for (KeyValue<TKey, TValue> kv : p_vhm_pair.values_cache)
	{
		new_entry = create_one(kv.key);
		*new_entry = p_vhm_pair.values[kv.value];
	}

	return true;
}

template <typename TKey, typename TValue>
void VectorHashMapPair<TKey, TValue>::merge_keep(const VectorHashMapPair<TKey, TValue> &p_vhm_pair)
{
	TValue *new_entry = nullptr;

	for (KeyValue<TKey, TValue> kv : p_vhm_pair.values_cache)
	{
		if (values_cache.has(kv.key))
		{
			continue;
		}

		*new_entry = p_vhm_pair.values[kv.value];
	}

	return;
}

template <typename TKey, typename TValue>
void VectorHashMapPair<TKey, TValue>::merge_override(const VectorHashMapPair<TKey, TValue> &p_vhm_pair)
{
	TValue *new_entry = nullptr;

	for (KeyValue<TKey, TValue> kv : p_vhm_pair.values_cache)
	{
		if (values_cache.has(kv.key))
		{
			values[values_cache[kv.key]] = p_vhm_pair.values[kv.value];
		}
		else
		{
			new_entry = create_one(kv.key);
			*new_entry = p_vhm_pair.values[kv.value];
		}
	}
	return;
}

template <typename TKey, typename TValue>
void VectorHashMapPair<TKey, TValue>::replace(const VectorHashMapPair<TKey, TValue> &p_vhm_pair)
{
	values.clear();
	values_cache.clear();

	// DEBUG:  If this somehow malfunctions, just switch to the for loop method
	values = p_vhm_pair.values;
	values_cache = p_vhm_pair.values_cache;

	return;
}
