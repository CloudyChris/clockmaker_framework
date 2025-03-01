/* cm_enums.h */

#ifndef CM_ENUMS_H
#define CM_ENUMS_H

#include "core/string/ustring.h"

#undef COMPILE_ENUMS_TO_STRING
#undef ENUM_BEGIN
#undef ENUM
#undef ENUM_END

#ifndef COMPILE_ENUMS_TO_STRING
    #define ENUM_BEGIN(m_enum_name) typedef enum e##m_enum_name
    #define ENUM(m_enum_value) m_enum_value
    #define ENUM_END(m_enum_name) m_enum_name; \
		String m_enum_name##_str(enum e##m_enum_name index);
#else
    #define ENUM_BEGIN(m_enum_name) const char * const m_enum_name##_strings [] =
    #define ENUM(m_enum_value) #m_enum_value
    #define ENUM_END(m_enum_name) ; String m_enum_name##_str(m_enum_name index){ return m_enum_name##_strings[index]; };
#endif

namespace cm_enums
{
ENUM_BEGIN(CM_DataFieldType)
{
	ENUM(CM_DATA_FIELD_TYPE_NONE),
	ENUM(CM_DATA_FIELD_TYPE_BASIC),
	ENUM(CM_DATA_FIELD_TYPE_ENTRY),
	ENUM(CM_DATA_FIELD_TYPE_RESOURCE)
}
ENUM_END(CM_DataFieldType)

ENUM_BEGIN(CM_DataType)
{
	ENUM(CM_DATA_TYPE_NONE),
	ENUM(CM_DATA_TYPE_TABLE_SPECIFICATION),
	ENUM(CM_DATA_TYPE_COLLECTION),
	ENUM(CM_DATA_TYPE_TABLE),
	ENUM(CM_DATA_TYPE_ENTRY)
}
ENUM_END(CM_DataType)

ENUM_BEGIN(CM_DataCollectionType)
{
	ENUM(CM_DATA_COLLECTION_TYPE_NONE),
	ENUM(CM_DATA_COLLECTION_TYPE_CORE),
	ENUM(CM_DATA_COLLECTION_TYPE_GAME),
	ENUM(CM_DATA_COLLECTION_TYPE_MODS),
	ENUM(CM_DATA_COLLECTION_TYPE_TOOL)
}
ENUM_END(CM_DataCollectionType)
} // namespace cm_enums
#endif
