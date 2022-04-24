#include "gb_apu.h"
#include "gb_addresses.h"
#include <iostream>
constexpr int SAMPLE_RATE = 48000;
constexpr int AMPLITUDE = 18000;

namespace TKPEmu::Gameboy::Devices {
    APU::APU(ChannelArrayPtr channel_array_ptr) 
            : channel_array_ptr_(channel_array_ptr) {
        SDL_AudioSpec want;
        SDL_zero(want);
        want.freq = SAMPLE_RATE;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 2048;
        SDL_AudioSpec have;
        if (SDL_OpenAudio(&want, &have) != 0) {
            SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
        }
        if (want.format != have.format) {
            SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");
        }
        device_id_ = SDL_OpenAudioDevice(0, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
        float i = 0;
        std::fill(samples_.begin(), samples_.end(), 0);
        SDL_PauseAudioDevice(device_id_, 0);
        // SDL_Delay(1000);
    }
    void APU::Update(int clk) {
        (*channel_array_ptr_)[0].StepWaveGeneration(clk);
        float ch1 = (*channel_array_ptr_)[0].GetAmplitude();
        float ch2 = (*channel_array_ptr_)[1].GetAmplitude();
        float ch3 = (*channel_array_ptr_)[2].GetAmplitude();
        float ch4 = (*channel_array_ptr_)[3].GetAmplitude();
        auto samp = (ch1 + ch2 + ch3 + ch4) / 4.0f;
        auto freq = 440;
        double res = sin((sample_index_) / (SAMPLE_RATE / 440.0) * 2.0 * M_PI)>=0.0 ? 1.0:-1.0;//copysign(1.0, sin(((float)sample_index_/SAMPLE_RATE) * 441.0f));
        samples_[sample_index_++] = res * AMPLITUDE * (*channel_array_ptr_)[0].DACOutput;
        if (sample_index_ == samples_.size()) {
            sample_index_ = 0;
            SDL_QueueAudio(device_id_, &samples_[0], sizeof(samples_));
        }
    }
}