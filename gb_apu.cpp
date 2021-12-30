#include "gb_apu.h"
#include "gb_addresses.h"
namespace TKPEmu::Gameboy::Devices {
    APU::APU(Bus& bus) : bus_(bus), DIV(bus_.GetReference(addr_div)) {}
    void APU::Update() {
        
    }
}