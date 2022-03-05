#include "gb_apu_ch.h"
#include <iostream>

namespace TKPEmu::Gameboy::Devices {
    void APUChannel::StepWaveGeneration(int cycles) {
        FrequencyTimer -= cycles;
        if (FrequencyTimer <= 0) {
            FrequencyTimer += (2048 - WaveFrequency) * 4;
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
        if (LengthTimer > 0 && LengthDecOne) {
            --LengthTimer;
        }
        if (LengthTimer == 0) {
            LengthCtrEnabled = false;
        }
    }
    void APUChannel::ClockVolEnv() {
        if (VolEnvEnabled) {
            if (EnvelopePeriod != 0) {
                if (PeriodTimer > 0) {
                    --PeriodTimer;
                }
                if (PeriodTimer == 0) {
                    PeriodTimer = EnvelopePeriod;
                    if (EnvelopeCurrentVolume > 0 && !EnvelopeIncrease) {
                        --EnvelopeCurrentVolume;
                    }
                    if (EnvelopeCurrentVolume < 0xF && EnvelopeIncrease) {
                        ++EnvelopeCurrentVolume;
                    }
                }
                DACInput = GetAmplitude() * EnvelopeCurrentVolume;
                DACOutput = (DACInput / 7.5f) - 1.0f; 
            }
        }
    }
    void APUChannel::ClockSweep() {
        if (SweepTimer > 0) {
            --SweepTimer; 
        }
        if (SweepTimer == 0) {
            SweepTimer = (SweepPeriod == 0) ? 8 : SweepPeriod;
            SweepEnabled = SweepPeriod != 0 || SweepShift != 0;
            if (SweepEnabled && SweepPeriod > 0) {
                CalculateSweepFreq();
                if (new_frequency <= 2047 && SweepShift > 0) {
                    Frequency = new_frequency;
                    ShadowFrequency = new_frequency;
                    CalculateSweepFreq();
                }
            }
        }
    }
    void APUChannel::CalculateSweepFreq() {
        new_frequency = ShadowFrequency >> SweepShift;
        if (!SweepIncrease) {
            new_frequency = ShadowFrequency - new_frequency;
        } else {
            new_frequency = ShadowFrequency + new_frequency;
        }
        if (new_frequency > 2047) {
            SweepEnabled = false;
            DisableChannelFlag = true;
        }
    }
}