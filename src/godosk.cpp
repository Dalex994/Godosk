#include "godosk.h"
#include "vosk_api.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <thread>

using namespace godot;

void Godosk::_bind_methods() {
    ClassDB::bind_method(D_METHOD("init", "model_path", "sample_rate"), &Godosk::init);
    ClassDB::bind_method(D_METHOD("stop"), &Godosk::stop);

    ADD_SIGNAL(MethodInfo("text_recognized", PropertyInfo(Variant::STRING, "result")));
    ADD_SIGNAL(MethodInfo("partial_recognized", PropertyInfo(Variant::STRING, "partial")));
}

Godosk::Godosk() {

}

Godosk::~Godosk() {
    stop();
    if (recognizer) {
        vosk_recognizer_free(recognizer);
    }
    if (model) {
        vosk_model_free(model);
    }
}

void Godosk::init(String model_path, int sample_rate) {
    if (running) return;

    vosk_set_log_level(2)
    model = vosk_model_new(model_path.utf8().get_data());
    recognizer = vosk_recognizer_new(model, sample_rate);

    Pa_Initialize();

    running = true;
    thread = memnew(Thread);
    thread->start([this]() { capture_loop(); });
}

void Godosk::stop() {
    if (!running) return;
    running = false;
    Pa_Terminate();
    if (thread) {
        thread->wait_to_finish();
        memdelete(thread);
        thread = nullptr;
    }
}

int Godosk::pa_callback(const void *input, void *, unsigned long frameCount,
                            const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags,
                            void *userData) {
    Recognizer *self = (Recognizer *)userData;
    if (!input || !self->recognizer) return paContinue;

    int accepted = vosk_recognizer_accept_waveform(self->recognizer, input, frameCount * 2);
    if (accepted) {
        String text = String(vosk_recognizer_result(self->recognizer));
        self->emit_signal("text_recognized", text);
    } else {
        String partial = String(vosk_recognizer_partial_result(self->recognizer));
        self->emit_signal("partial_recognized", partial);
    }

    return paContinue;
}

void Godosk::capture_loop() {
    PaStream *stream;
    PaStreamParameters input_params;

    input_params.device = Pa_GetDefaultInputDevice();
    input_params.channelCount = 1;
    input_params.sampleFormat = paInt16;
    input_params.suggestedLatency = Pa_GetDeviceInfo(input_params.device)->defaultLowInputLatency;
    input_params.hostApiSpecificStreamInfo = nullptr;

    Pa_OpenStream(&stream, &input_params, nullptr, 16000, 512, paClipOff, pa_callback, this);
    Pa_StartStream(stream);

    while (running && Pa_IsStreamActive(stream)) {
        Pa_Sleep(50);
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
}