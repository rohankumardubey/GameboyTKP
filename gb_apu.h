#pragma once
#ifndef TKP_GB_APU_H
#define TKP_GB_APU_H
#include "gb_bus.h"
namespace TKPEmu::Gameboy::Devices {
    class APU {
    public:
        APU(Bus& bus);
        void Update();
    private:
        Bus& bus_;
        uint8_t &DIV;
    };
}
#endif