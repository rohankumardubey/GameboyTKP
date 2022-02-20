#pragma once
#ifndef TKP_GB_APU_H
#define TKP_GB_APU_H
#include <SDL2/SDL.h>
#define AUDIO_FREQUENCY 48000
#define AUDIO_BUFFER_SIZE 512
namespace TKPEmu::Gameboy::Devices {
    // This class is solely for sound output and is not needed to pass sound emulation tests.
    // All computation for this class happens in gb_bus
    class APU {
    public:
        APU();
        void Update();
    };
}
#endif