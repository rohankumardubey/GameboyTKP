#pragma once
#ifndef TKP_GB_APU_H
#define TKP_GB_APU_H
#include <SDL2/SDL.h>
#include <queue>
#include "gb_apu_ch.h"
#define AUDIO_FREQUENCY 48000
#define AUDIO_BUFFER_SIZE 512
namespace TKPEmu::Gameboy::Devices {
    struct Sample {
        double freq;
        int samplesLeft;
    };
    // This class is solely for sound output and is not needed to pass sound
    // emulation tests.
    // All computation for this class happens in gb_bus and gb_apu_ch
    class APU {
    public:
        APU(ChannelArrayPtr channel_array_ptr);
        void Update();
    private:
        std::queue<Sample> samples_;
        ChannelArrayPtr channel_array_ptr_;
    };
}
#endif