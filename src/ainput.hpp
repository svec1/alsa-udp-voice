#ifndef AINPUT_HPP
#define AINPUT_HPP

#include <aaudio.hpp>
#include <array>
#include <chrono>
#include <cstring>
#include <thread>

class input : private audio {
   public:
    input();

   public:
    std::array<byte, audio::buffer_size> get_samples();
};

input::input() : audio(SND_PCM_STREAM_CAPTURE) {}

std::array<byte, audio::buffer_size> input::get_samples() {
    std::array<byte, audio::buffer_size> bytes;
    int ret;

    while ((ret = snd_pcm_readi(pcm_handle, bytes.data(), period_size)) < 0)
        snd_call(snd_pcm_prepare, pcm_handle);

    return bytes;
}
#endif
