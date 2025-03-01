/* cm_singleton_specifications. */

#include "cm_singleton_interfaces.h"
#include "core/object/class_db.h"

namespace cm_singleton_interfaces
{

void GameDataManager::_bind_methods()
{
	BIND_ENUM_CONSTANT(CM_DATA_FIELD_TYPE_NONE);
	BIND_ENUM_CONSTANT(CM_DATA_FIELD_TYPE_BASIC);
	BIND_ENUM_CONSTANT(CM_DATA_FIELD_TYPE_ENTRY);
	BIND_ENUM_CONSTANT(CM_DATA_FIELD_TYPE_RESOURCE);
}

} // namespace cm_singleton_interfaces
