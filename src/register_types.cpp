#include "register_types.h"
#include "godosk.h"
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_vosk_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        ClassDB::register_class<Godosk>();
    }
}

void uninitialize_vosk_module(ModuleInitializationLevel p_level) {}

extern "C" {
GDExtensionBool GDE_EXPORT entry_point(const GDExtensionInterface *p_interface,
                                       GDExtensionClassLibraryPtr p_library,
                                       GDExtensionInitialization *r_initialization) {
    GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);
    init_obj.register_initializer(initialize_vosk_module);
    init_obj.register_terminator(uninitialize_vosk_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
}
