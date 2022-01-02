#pragma once
#ifndef TKP_GB_PPU_H
#define TKP_GB_PPU_H
#include "gb_bus.h"
#include "../include/TKPImage.h"
#include "gb_addresses.h"
#include <mutex>
#include <array>
#include <queue>
namespace TKPEmu::Gameboy::Devices {
	constexpr int FRAME_CYCLES = 70224;
	struct Pixel {
		uint8_t Color;
		uint8_t Palette;
		uint8_t SpritePriority;
		uint8_t BackgroundPriority;
	};
	class PPU {
	private:
		using TKPImage = TKPEmu::Tools::TKPImage;
	public:
		bool ReadyToDraw = false;
		PPU(Bus& bus, std::mutex* draw_mutex);
		void Update(uint8_t cycles);
		void Reset();
        int CalcCycles();
		float* GetScreenData();
		void FillTileset(float* pixels, size_t x_off = 0, size_t y_off = 0, uint16_t addr = 0x8000);
	private:
		Bus& bus_;
		std::mutex* draw_mutex_;
		std::array<float, 4 * 160 * 144> screen_color_data_{};
		// PPU memory mapped registers
		uint8_t& LCDC, &STAT, &LYC, &LY, &IF, &SCY, &SCX, &WY, &WX;
		std::vector<uint8_t> cur_scanline_sprites_;
		uint8_t window_internal_temp_ = 0;
		uint8_t window_internal_ = 0;
		int clock_ = 0;
		int clock_target_ = 0;
		int set_mode(int mode);
		int get_mode();
		int update_lyc();
		bool is_sprite_eligible(uint8_t sprite_y);
		void draw_scanline();
		inline void render_tiles();
		inline void render_sprites();
	};
}
#endif
