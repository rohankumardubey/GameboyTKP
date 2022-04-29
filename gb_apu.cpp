#include "gb_apu.h"
#include "gb_addresses.h"
#include <iostream>
constexpr int SAMPLE_RATE = 48000;
constexpr int AMPLITUDE = 8000;
constexpr int RESAMPLED_RATE = (4194304 / SAMPLE_RATE);

namespace TKPEmu::Gameboy::Devices {
    APU::APU(ChannelArrayPtr channel_array_ptr, uint8_t& NR52) 
            : channel_array_ptr_(channel_array_ptr), NR52_(NR52) {}
    APU::~APU() {
        if (UseSound) {
            SDL_CloseAudioDevice(device_id_);
        }
    }
    void APU::InitSound() {
        if (UseSound) {
            SDL_AudioSpec want;
            SDL_zero(want);
            want.freq = SAMPLE_RATE;
            want.format = AUDIO_S16SYS;
            want.channels = 1;
            want.samples = 512;
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
        }
    }
    void APU::Update(int clk) {
        if (UseSound && (NR52_ & 0b1000'0000)) {
            static int inner_clk = 0;
            static unsigned yep_clock = 0;
            auto& chan1 = (*channel_array_ptr_)[0];
            auto& chan2 = (*channel_array_ptr_)[1];
            auto& chan4 = (*channel_array_ptr_)[3];
            inner_clk += clk;
            chan1.StepWaveGeneration(clk);
            chan2.StepWaveGeneration(clk);
            chan4.StepWaveGenerationCh4(clk);
            double chan1out = (chan1.GetAmplitude() == 0.0 ? 1.0 : -1.0) * chan1.DACOutput * chan1.GlobalVolume();
            double chan2out = (chan2.GetAmplitude() == 0.0 ? 1.0 : -1.0) * chan2.DACOutput * chan2.GlobalVolume();
            double chan4out = (~chan4.LFSR & 0x01) * chan4.DACOutput * chan4.GlobalVolume();
            if (inner_clk >= RESAMPLED_RATE) {
                auto sample = (chan1out + chan2out) / 3;
                samples_[sample_index_++] = sample * AMPLITUDE;
                yep_clock++;
                // in case it's bigger
                inner_clk = inner_clk - RESAMPLED_RATE;
            }
            if (sample_index_ == samples_.size()) {
                sample_index_ = 0;
                QueueSamples();
            }
        }
    }
}