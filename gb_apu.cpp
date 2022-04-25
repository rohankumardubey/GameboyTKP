#include "gb_apu.h"
#include "gb_addresses.h"
#include <iostream>
constexpr int SAMPLE_RATE = 48000;
constexpr int AMPLITUDE = 8000;

namespace TKPEmu::Gameboy::Devices {
    APU::APU(ChannelArrayPtr channel_array_ptr) 
            : channel_array_ptr_(channel_array_ptr) {
        SDL_AudioSpec want;
        SDL_zero(want);
        want.freq = SAMPLE_RATE;
        want.format = AUDIO_S16SYS;
        want.channels = 1;
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
        for (int i = 0; i < clk; i++) {
            (*channel_array_ptr_)[0].StepWaveGeneration(1);
            (*channel_array_ptr_)[1].StepWaveGeneration(1);
            double freq = (*channel_array_ptr_)[1].Frequency;
            double freq2 = (*channel_array_ptr_)[0].Frequency;
            freq = 131072/(2048-freq);
            freq2 = 131072/(2048-freq2);
            double ch1 = (sin((sample_index_) / (SAMPLE_RATE / freq2) * 2.0 * M_PI)>=0.0 ? 1.0:-1.0) * (*channel_array_ptr_)[0].DACOutput;
            double ch2 = (sin((sample_index_) / (SAMPLE_RATE / freq) * 2.0 * M_PI)>=0.0 ? 1.0:-1.0) * (*channel_array_ptr_)[1].DACOutput;
            if (sample_index_ == samples_.size() ) {
                if (SDL_GetQueuedAudioSize(device_id_) == 0) {
                    sample_index_ = 0;
                    SDL_QueueAudio(device_id_, &samples_[0], sizeof(samples_));
                }
            } else {
                auto sample = (ch1 + ch2) / 2;
                samples_[sample_index_++] = sample * AMPLITUDE;
            }
        }
    }
}