#pragma once
#ifndef TKPEMU_GB_ROMDATA_H
#define TKPEMU_GB_ROMDATA_H
#include <string>
#include "../include/base_application.h"
#include "gameboy.h"
#include "gb_cartridge.h"
#include "../glad/glad/glad.h"
namespace TKPEmu::Applications {
    using Gameboy = TKPEmu::Gameboy::Gameboy;
    class GameboyRomData : public IMApplication {
    public:
        GameboyRomData(std::string menu_title, std::string window_title);
        ~GameboyRomData();
    private:
        void v_draw() override;
        void draw_info();
        void draw_tilesets();
        void update_tilesets();
        GLuint texture_;
        std::vector<float> image_data_;
    };
}
#endif