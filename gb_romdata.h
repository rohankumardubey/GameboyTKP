#pragma once
#ifndef TKP_GB_ROMDATA_H
#define TKP_GB_ROMDATA_H
#include <string>
#include <chrono>
#include "../include/base_application.h"
#include "gameboy.h"
#include "gb_cartridge.h"
#include <GL/glew.h>
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
        void draw_palettes();
        void update_tilesets();
        bool texture_cached_ = false;
        GLuint texture_;
        std::vector<float> image_data_;
        std::chrono::high_resolution_clock::time_point clock_a = std::chrono::high_resolution_clock::now();
    };
}
#endif