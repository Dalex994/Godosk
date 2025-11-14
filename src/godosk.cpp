#include "../include/godosk.hpp"
#include "godot_cpp/core/class_db.hpp"




#include <cstring>

using namespace godot;

Godosk::Godosk() {
	model = nullptr;
	recognizer = nullptr;
}

Godosk::~Godosk() {
	stop_listening();
	// Free recognizer/model if still present
	if (recognizer) {
		vosk_recognizer_free(recognizer);
		recognizer = nullptr;
	}
	if (model) {
		vosk_model_free(model);
		model = nullptr;
	}
}

void Godosk::_ready() {
	print_line("[Godosk] ready");
	set_process(true);
}

void Godosk::_process(double delta)
{
	// Emit queued partials/finals/audio buffers on main thread
    std::vector<String> partials;
    std::vector<String> finals;
    std::vector<PackedByteArray> audio_buffers;

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (!pending_partials.empty()) {
            partials.swap(pending_partials);
        }
        if (!pending_finals.empty()) {
            finals.swap(pending_finals);
        }
        if (!pending_audio_buffers.empty()) {
            audio_buffers.swap(pending_audio_buffers);
        }
    }

    for (const String &p : partials) {
        // partial_result(String text)
        emit_signal("partial_result", p);
    }

    for (const String &f : finals) {
        // final_result(String text)
        emit_signal("final_result", f);
    }

    for (const PackedByteArray &b : audio_buffers) {
        emit_signal("audio_buffer", b);
    }
}

void Godosk::load_model(const String &model_path)
{
	// Free previous
    if (recognizer) {
        vosk_recognizer_free(recognizer);
        recognizer = nullptr;
    }
    if (model) {
        vosk_model_free(model);
        model = nullptr;
    }

    const char *mp = model_path.utf8().get_data();
    model = vosk_model_new(mp);
    if (!model) {
        print_line("[Godosk] Error: could not load model at: ", model_path);
        emit_signal("error", String("Could not load model: ") + model_path);
        return;
    }

    recognizer = vosk_recognizer_new(model, (float)sample_rate);
    if (!recognizer) {
        print_line("[Godosk] Error: could not create recognizer.");
        emit_signal("error", String("Could not create recognizer"));
        return;
    }

    vosk_recognizer_set_max_alternatives(recognizer, 0);
    vosk_recognizer_set_words(recognizer, true);
    use_grammar = false;

    print_line("[Godosk] Model loaded:", model_path);
}

void Godosk::load_model_with_grammar(const String &model_path, const String &grammar_json)
{
	// Free previous
    if (recognizer) {
        vosk_recognizer_free(recognizer);
        recognizer = nullptr;
    }
    if (model) {
        vosk_model_free(model);
        model = nullptr;
    }

    const char *mp = model_path.utf8().get_data();
    model = vosk_model_new(mp);
    if (!model) {
        UtilityFunctions::print("[Godosk] Error: could not load model at: ", model_path);
        emit_signal("error", String("Could not load model: ") + model_path);
        return;
	}

	vosk_recognizer_set_max_alternatives(recognizer, 0);
    vosk_recognizer_set_words(recognizer, true);
    use_grammar = true;

    print_line("[Godosk] Model+grammar loaded:", model_path);
}

void Godosk::start_listening()
{
	if (listening) {
        UtilityFunctions::print("[Godosk] Already listening.");
        return;
    }

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        UtilityFunctions::print("[Godosk] PortAudio init failed:", Pa_GetErrorText(err));
        emit_signal("error", String("PortAudio init failed: ") + Pa_GetErrorText(err));
        return;
    }

    PaStreamParameters input_params;
    input_params.device = Pa_GetDefaultInputDevice();
    if (input_params.device == paNoDevice) {
        print_line("[Godosk] No default input device.");
        emit_signal("error", String("No default input device"));
        Pa_Terminate();
        return;
    }
    const PaDeviceInfo* devinfo = Pa_GetDeviceInfo(input_params.device);

    input_params.channelCount = 1;
    input_params.sampleFormat = paFloat32;
    input_params.suggestedLatency = devinfo->defaultLowInputLatency;
    input_params.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&stream,
                        &input_params,
                        nullptr,                // no output
                        sample_rate,
                        frames_per_buffer,
                        paClipOff,
                        &Godosk::audio_callback,
                        this);
    if (err != paNoError) {
        UtilityFunctions::print("[Godosk] PortAudio open stream failed:", Pa_GetErrorText(err));
        emit_signal("error", String("PortAudio open stream failed: ") + Pa_GetErrorText(err));
        Pa_Terminate();
        return;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        print_line("[Godosk] PortAudio start stream failed:", Pa_GetErrorText(err));
        emit_signal("error", String("PortAudio start stream failed: ") + Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        stream = nullptr;
        return;
    }

    listening = true;
    print_line("[Godosk] Listening started.");
}

void Godosk::stop_listening()
{
	if (!listening) return;

    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        stream = nullptr;
    }
    Pa_Terminate();
    listening = false;
    print_line("[Godosk] Listening stopped.");
}



// Thread-safe push helpers (called from audio callback)
void Godosk::push_partial(const String &s)
{
	std::lock_guard<std::mutex> lock(queue_mutex);
    pending_partials.push_back(s);
}

void Godosk::push_final(const String &s)
{
	std::lock_guard<std::mutex> lock(queue_mutex);
    pending_finals.push_back(s);
}

void Godosk::push_audio_buffer(const void *data, size_t byte_count)
{
	 PackedByteArray arr;
    arr.resize(byte_count);
    memcpy(arr.ptrw(), data, byte_count);
    std::lock_guard<std::mutex> lock(queue_mutex);
    pending_audio_buffers.push_back(arr);
}

int Godosk::audio_callback(const void *inputBuffer, 
							     void *outputBuffer,
							  unsigned long framesPerBuffer,
							  const PaStreamCallbackTimeInfo* timeInfo,
							  PaStreamCallbackFlags statusFlags,
							  void *userData)
{
	Godosk *self = static_cast<Godosk*>(userData);
	if (!self) return paContinue;

    const float *in = reinterpret_cast<const float*>(inputBuffer);
    if (!in) {
        // silence
        return paContinue;
    }

    // Send raw audio buffer to main thread too (optional, for visualization)
    self->push_audio_buffer(in, framesPerBuffer * sizeof(float));

    // If no recognizer loaded, nothing to do
    if (!self->recognizer) return paContinue;

    // Feed waveform to Vosk (float)
    // Vosk provides vosk_recognizer_accept_waveform_f to accept floats
    int accepted = vosk_recognizer_accept_waveform_f(self->recognizer, in, (int)framesPerBuffer);
    // partial result (always available)
    const char *partial = vosk_recognizer_partial_result(self->recognizer);
    if (partial && partial[0] != '\0') {
        // push partial as String (JSON string as Vosk returns)
        self->push_partial(String(partial));
    }
    // If final result available, push it
    // In Vosk C API, vosk_recognizer_final_result returns JSON string only when end-of-utterance detected and recognizer reset.
    // However many implementations use vosk_recognizer_final_result() after accept or on VAD
    const char *final_res = nullptr;
    // Vosk C API: vosk_recognizer_result returns final result JSON if available (but it may return empty)
    final_res = vosk_recognizer_result(self->recognizer);
    if (final_res && final_res[0] != '\0') {
        self->push_final(String(final_res));
        // Note: many Vosk apps call vosk_recognizer_reset(recognizer) after final; C API doesn't provide reset directly.
        // If needed, user can recreate recognizer; here we keep the recognizer as-is.
    }

    (void)accepted; // silence unused var warning in some builds
    return paContinue;
}

void Godosk::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("load_model", "model_path"),  &Godosk::load_model);
	ClassDB::bind_method(D_METHOD("load_model_with_grammar", "model_path", "grammar_json"), &Godosk::load_model_with_grammar);
	ClassDB::bind_method(D_METHOD("start_listening"), &Godosk::start_listening);
    ClassDB::bind_method(D_METHOD("stop_listening"), &Godosk::stop_listening);
    // Expose is_listening if wanted
    ClassDB::bind_method(D_METHOD("is_listening"), &Godosk::is_listening);

    // Signals
    ADD_SIGNAL(MethodInfo("partial_result", PropertyInfo(Variant::STRING, "text")));
    ADD_SIGNAL(MethodInfo("final_result", PropertyInfo(Variant::STRING, "text")));
    ADD_SIGNAL(MethodInfo("audio_buffer", PropertyInfo(Variant::PACKED_BYTE_ARRAY, "buffer")));
    ADD_SIGNAL(MethodInfo("error", PropertyInfo(Variant::STRING, "message")));
}

