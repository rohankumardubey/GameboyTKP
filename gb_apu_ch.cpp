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
    void APUChannel::StepFrameSequencer() {
        FrameSequencer = (FrameSequencer + 1) & 0b111;
        if (FrameSequencer % 2 == 0) {
            ClockLengthCtr();
            if (FrameSequencer == 2 || FrameSequencer == 6) {
                ClockSweep();
            }
        } else if (FrameSequencer == 7) {
            ClockVolEnv();
        }
    }
    void APUChannel::ClockLengthCtr() {
        if (LengthCtrEnabled) {

        }
    }
    void APUChannel::ClockVolEnv() {
        if (VolEnvEnabled) {

        }
    }
    void APUChannel::ClockSweep() {
        if (SweepEnabled) {
            
        }
    }
}