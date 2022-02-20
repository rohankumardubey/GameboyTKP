#include "gb_apu_ch.h"

namespace TKPEmu::Gameboy::Devices {
    void APUChannel::StepWaveGeneration(int cycles) {
        FrequencyTimer -= cycles;
        if (FrequencyTimer <= 0) {
            FrequencyTimer += (2048 - Frequency) * 4;
            // WaveDutyPosition stays in range 0-7
            WaveDutyPosition = (WaveDutyPosition + 1) & 0b111;
        }
    }
    bool APUChannel::GetAmplitude() {
        return (Waveforms[WaveDutyPattern] >> WaveDutyPosition) & 0b1;
    }
}