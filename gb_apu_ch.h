#pragma once
#ifndef TKP_GB_APU_CH_H
#define TKP_GB_APU_CH_H
namespace TKPEmu::Gameboy::Devices {
    constexpr int Waveforms[4] = { 0b00000001, 0b00000011, 0b00001111, 0b11111100 };
    struct APUChannel {
        // TODO: some of these variables dont need to be public
        unsigned WaveFrequency = 0;
        int FrequencyTimer = 0;
        int WaveDutyPattern = 0;
        int WaveDutyPosition = 0;
        int FrameSequencer = 0; // TODO: unimplemented
        bool LengthCtrEnabled = false;
        bool VolEnvEnabled = false;
        int EnvelopeCurrentVolume = 0;
        bool EnvelopeIncrease = false;
        int EnvelopePeriod = 0;
        int SweepPeriod = 0;
        bool SweepIncrease = false;
        int SweepShift = 0;
        bool SweepEnabled = false;
        int SweepTimer = 0;
        int ShadowFrequency = 0;
        int Frequency = 0;
        int LengthTimer = 0;
        int LengthData = 0;
        bool LengthDecOne = false;
        int LengthInit = 64;
        int PeriodTimer = 0;
        int DACInput = 0;
        float DACOutput = 0;
        bool DACEnabled = true;

        void StepWaveGeneration(int cycles);
        void StepFrameSequencer();
        bool GetAmplitude();
        void ClockLengthCtr();
        void ClockVolEnv();
        void ClockSweep();
    private:
        int new_frequency = 0;
        void calculate_frequency();
    };
}
#endif
