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
        bool LengthCtrEnabled = false;
        bool VolEnvEnabled = false;
        bool SweepEnabled = false;
        int EnvelopeCurrentVolume = 0;
        bool EnvelopeIncrease = false;
        int EnvelopeNumSweep = 0;
        int PeriodTimer = 0;
        int DACInput = 0;
        float DACOutput = 0;

        void StepWaveGeneration(int cycles);
        void StepFrameSequencer();
        bool GetAmplitude();
        void ClockLengthCtr();
        void ClockVolEnv();
        void ClockSweep();
    };
}
#endif
