#ifndef GODOSK_GODOSK_HPP
#define GODOSK_GODOSK_HPP

#pragma once

// It's bad, it's horrible, but it works for all toolchains !
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include "portaudio.h"
#include "vosk_api.h"

namespace godot {
class String;
}

using namespace godot;

class Godosk : public Node {
	GDCLASS(Godosk, Node);

	private:
		// Audio / Portaudio
		PaStream *stream = nullptr;
		std::atomic<bool> listening = false;
		int sample_rate = 16000;
	    unsigned long frames_per_buffer = 512;

		// Vosk
		VoskModel *model = nullptr;
		VoskRecognizer *recognizer = nullptr;
		bool use_grammar = false;

	// Thread-safe queues for results (to emit on main thread)
	std::mutex queue_mutex;
	std::vector<String> pending_partials;
	std::vector<String> pending_finals;
	std::vector<PackedByteArray> pending_audio_buffers;

	// Helper to push into queues from audio thread
	void push_partial(const String &s);
	void push_final(const String &s);
	void push_audio_buffer(const void *data, size_t byte_count);

	// PortAudio callback (static)
	static int audio_callback(const void *inputBuffer, void *outputBuffer,
							  unsigned long framesPerBuffer,
							  const PaStreamCallbackTimeInfo* timeInfo,
							  PaStreamCallbackFlags statusFlags,
							  void *userData);

	protected:
		static void _bind_methods();

	public:
		Godosk();
	    ~Godosk();

		// Godot lifecycle
	void _ready() override;
	void _process(double delta) override;

	// Public API
	void load_model(const String &model_path);
	void load_model_with_grammar(const String &model_path, const String &grammar_json);
	void start_listening();
	void stop_listening();

	// Utility (exposed if wanted)
	bool is_listening() const { return listening; }
};



#endif //GODOSK_GODOSK_HPP