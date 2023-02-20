#ifndef GB_WIIWRAPPER_H
#define GB_WIIWRAPPER_H
#include <GameboyTKP/gb_addresses.h>
#include <GameboyTKP/gb_cpu.h>
#include <GameboyTKP/gb_ppu.h>
#include <GameboyTKP/gb_bus.h>
#include <GameboyTKP/gb_timer.h>
#include <GameboyTKP/gb_apu.h>
#include <GameboyTKP/gb_apu_ch.h>

class GB_WiiWrapper {
public:
    GB_WiiWrapper();
    void Update();
    void* GetScreenData();
    void LoadCartridge(void* data);
    using CPU = TKPEmu::Gameboy::Devices::CPU;
    CPU cpu_;
private:
    using GameboyPalettes = std::array<std::array<float, 3>,4>;
    using GameboyKeys = std::array<uint32_t, 4>;
    using PPU = TKPEmu::Gameboy::Devices::PPU;
    using APU = TKPEmu::Gameboy::Devices::APU;
    using ChannelArrayPtr = TKPEmu::Gameboy::Devices::ChannelArrayPtr;
    using ChannelArray = TKPEmu::Gameboy::Devices::ChannelArray;
    using Bus = TKPEmu::Gameboy::Devices::Bus;
    using Timer = TKPEmu::Gameboy::Devices::Timer;
    using Cartridge = TKPEmu::Gameboy::Devices::Cartridge;
    ChannelArrayPtr channel_array_ptr_;
    Bus bus_;
    APU apu_;
    PPU ppu_;
    Timer timer_;
    uint8_t& joypad_, &interrupt_flag_;
};
#endif