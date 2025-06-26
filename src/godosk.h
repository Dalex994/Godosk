#ifndef GODOSK_H
#define GODOSK_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/callable.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include "vosk_api.h"
#include "portaudio.h"

namespace godot {

    class Godosk : public RefCounted {
        GDCLASS(Godot, RefCounted);

    private:
        VoskModel *model = nullptr;
        VoskRecognizer *recognizer = nullptr;
        Thread *thread = nullptr;
        bool running = false;

        static int pa_callback(const void *input, void *output,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData);

        void capture_loop();

    protected:
        static void _bind_methods();

    public:
        Godosk();
        ~Godosk();

        void init(String model_path, int sample_rate = 16000);
        void stop();

        signals:
                void text_recognized(String result);
                void partial_recognized(String partial);
    };
}

#endif //GODOSK_H
