#ifndef GODOSK_H
#define GODOSK_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/binder_common.hpp>

using namespace godot;

class Godosk : public RefCounted {
    GDCLASS(Godosk, RefCounted);

private:
    Thread thread;
    Callable callback;

public:
    Godosk();
    ~Godosk();

    void _ready();

    void run_on_thread();  // démarre le thread
    void _thread_task();   // fonction du thread

protected:
    static void _bind_methods();
};

#endif // GODOSK_H
