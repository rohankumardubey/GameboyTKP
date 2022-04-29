#pragma once
#ifndef TKP_GB_APU_H
#define TKP_GB_APU_H
#include <SDL2/SDL.h>
#include <queue>
#include "gb_apu_ch.h"
namespace TKPEmu::Gameboy::Devices {
    // This class is solely for sound output and is not needed to pass sound
    // emulation tests.
    // All computation for this class happens in gb_bus and gb_apu_ch
    class APU {
    public:
        APU(ChannelArrayPtr channel_array_ptr, uint8_t& NR52);
        ~APU();
        void InitSound();
        void Update(int clk);
        inline void QueueSamples() {
            SDL_QueueAudio(device_id_, &samples_[0], sizeof(samples_));
        }
        inline bool IsQueueEmpty() {
            return SDL_GetQueuedAudioSize(device_id_) == 0;
        }
        bool UseSound = false;
    private:
        SDL_AudioDeviceID device_id_;
        std::array<int16_t, 512> samples_;
        size_t sample_index_ = 0;
        uint8_t& NR52_;
        ChannelArrayPtr channel_array_ptr_;
    };
}
#endif