#pragma once
#ifndef TKP_GB_APU_CH_H
#define TKP_GB_APU_CH_H
namespace TKPEmu::Gameboy::Devices {
    constexpr int Waveforms[4] = { 0b00000001, 0b00000011, 0b00001111, 0b11111100 };
    struct APUChannel {
        int Frequency = 0;
        int FrequencyTimer = 0;
        int WaveDutyPattern = 0;
        int WaveDutyPosition = 0;
        int FrameSequencer = 0; // TODO: unimplemented

        void StepWaveGeneration(int cycles);
        bool GetAmplitude();
    }
}
#endif