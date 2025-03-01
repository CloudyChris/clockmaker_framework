/* register_types.cpp */

#include "register_types.h"

#include "core/config/engine.h"
#include "core/object/class_db.h"

#include "cm_singleton_interfaces.h"
#include "uuid.h"

static cm_singleton_interfaces::GameDataManager *_game_data_manager = nullptr;

void initialize_clockmakers_framework_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		return;
	}

	GameDataManager::initialize();

	/* TRACKING */
	GDREGISTER_CLASS(TrackedObject);

	_game_data_manager = memnew(cm_singleton_interfaces::GameDataManager);

	/* SINGLETONS CLASS REGISTRATION */
	GDREGISTER_CLASS(cm_singleton_interfaces::GameDataManager);

	/* SINGLETONS ENGINE REGISTRATION */
	Engine::get_singleton()->add_singleton(Engine::Singleton("GameDataManager", cm_singleton_interfaces::GameDataManager::get_singleton()));
}

void uninitialize_clockmakers_framework_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		return;
	}

	memdelete(_game_data_manager);

	GameDataManager::finalize();
}
