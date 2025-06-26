#include "godosk.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

Godosk::Godosk() {
    UtilityFunctions::print("Godosk plugin initialized.");
}

Godosk::~Godosk() {
    if (thread.is_alive()) {
        thread.wait_to_finish();
    }
}

void Godosk::_ready() {
    UtilityFunctions::print("Godosk ready!");
}

void Godosk::run_on_thread() {
    Callable cb = Callable(this, "_thread_task");
    if (thread.is_alive()) {
        UtilityFunctions::print("Thread already running.");
        return;
    }
    thread.start(cb);
}

void Godosk::_thread_task() {
    UtilityFunctions::print("Thread task executing...");
    // Place ton code Vosk ici
}

void Godosk::_bind_methods() {
    ClassDB::bind_method(D_METHOD("run_on_thread"), &Godosk::run_on_thread);
    ClassDB::bind_method(D_METHOD("_thread_task"), &Godosk::_thread_task);
}
