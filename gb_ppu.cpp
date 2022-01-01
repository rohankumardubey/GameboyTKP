#include "gb_ppu.h"
#include "../glad/glad/glad.h"
#include <iostream>

namespace TKPEmu::Gameboy::Devices {
	enum STATMode {
		MODE_OAM_SCAN = 2,
		MODE_DRAW_PIXELS = 3,
		MODE_HBLANK = 0,
		MODE_VBLANK = 1,
	};
	PPU::PPU(Bus& bus, std::mutex* draw_mutex) : bus_(bus), draw_mutex_(draw_mutex),
		LCDC(bus.GetReference(0xFF40)),
		STAT(bus.GetReference(0xFF41)),
		LYC(bus.GetReference(0xFF45)),
		LY(bus.GetReference(0xFF44)),
		IF(bus.GetReference(0xFF0F)),
		SCY(bus.GetReference(0xFF42)),
		SCX(bus.GetReference(0xFF43)),
		WY(bus.GetReference(0xFF4A)),
		WX(bus.GetReference(0xFF4B))
	{}
	void PPU::Update(uint8_t cycles) {
		static constexpr int clock_max = 456 * 144 + 456 * 10;
		static bool oam_scanned = false;
		static int  mode3_extend = 0; // unused for now
		bool enabled = LCDC & LCDCFlag::LCD_ENABLE;
		uint8_t current_mode = STAT & STATFlag::MODE;
		if (enabled) {
			clock_ += cycles;
			clock_ %= clock_max;
			auto true_ly = clock_ / 456;
			if (LY != true_ly) {
				LY = true_ly;
				IF |= update_lyc();
			}
			if (LY <= 143) {
				// Normal scanline
				auto cur_scanline_clocks = clock_ % 456;
				// Every scanline takes 456 tclocks
				if (cur_scanline_clocks < 80) {
					// OAM scan
					if (get_mode() != MODE_OAM_SCAN) {
						// Load the 10 sprites for this line
						cur_scanline_sprites_.clear();
						for (size_t i = 0; i < (bus_.oam_.size() - 4); i += 4) {
							if (cur_scanline_sprites_.size() < 10 && is_sprite_eligible(bus_.oam_[i])) {
								cur_scanline_sprites_.push_back(i);
							}
						}
						// Sprites for this scanline are now scanned
						IF |= set_mode(MODE_OAM_SCAN);
					}
				} else if (cur_scanline_clocks < (80 + 172 + mode3_extend)) {
					if (get_mode() != MODE_DRAW_PIXELS) {
						IF |= set_mode(MODE_DRAW_PIXELS);
					}
				} else {
					if (get_mode() != MODE_HBLANK) {
						IF |= set_mode(MODE_HBLANK);
						std::lock_guard<std::mutex> lg(*draw_mutex_);
						draw_scanline();
					}
				}
			} else {
				// VBlank scanline
				if (get_mode() != MODE_VBLANK) {
					// VBlank interrupt triggers when we first enter vblank
					// and never again during vblank
					IF |= IFInterrupt::VBLANK;
					set_mode(MODE_VBLANK);
				}
			}
		}

		// if (LCDC & LCDCFlag::LCD_ENABLE) {
		// 	clock_ += cycles;
		// 	if (clock_ >= clock_target_) {
		// 		if (LY == 153) {
		// 			next_stat_mode = 2;
		// 			LY = -1;
		// 			clock_ = FRAME_CYCLES;
		// 			clock_target_ = FRAME_CYCLES;
		// 		}
		// 		IF |= set_mode(next_stat_mode);
		// 		int mode = get_mode(); 
		// 		if (mode == 2) {
		// 			clock_target_ += 80;
		// 			next_stat_mode = 3;
		// 			LY += 1;
		// 			// possible bug
		// 			IF |= update_lyc();
		// 		}
		// 		else if (mode == 3) {
		// 			clock_target_ += 168;
		// 			next_stat_mode = 0;
		// 		}
		// 		else if (mode == 0) {
		// 			clock_target_ += 208;
		// 			if (LY <= 143) {
		// 				next_stat_mode = 2;
		// 				std::lock_guard<std::mutex> lg(*draw_mutex_);
		// 				draw_scanline();
		// 			}
		// 			else {
		// 				next_stat_mode = 1;
		// 			}
		// 		}
		// 		else if (mode == 1) {
		// 			clock_target_ += 456;
		// 			next_stat_mode = 1;
		// 			if (LY == 144) {
		// 				IF |= IFInterrupt::VBLANK;
		// 			}
		// 			LY += 1;
		// 			IF |= update_lyc();
		// 		}
		// 	}
		// }
		// else {
		// 	LY = 0;
		// 	clock_ += cycles;
		// 	if (clock_ >= FRAME_CYCLES) {
		// 		clock_ %= FRAME_CYCLES;
		// 	}
		// }
	}
	bool PPU::is_sprite_eligible(uint8_t sprite_y) {
		bool use8x16 = LCDC & LCDCFlag::OBJ_SIZE;
		int y_pos_end = use8x16 ? sprite_y : sprite_y - 8;
		int y_pos_start = sprite_y - 16;
		// In order for this sprite to be one of the 10 drawn in this scanline
		// one of its 8 (or 16) lines need to be equal to LY
		if (LY >= y_pos_start && LY < y_pos_end) {
			return true;
		}
		return false;
 	}
	void PPU::fifo_fetch(uint8_t dots) {

	}
	void PPU::Reset() {
		for (int i = 0; i < (screen_color_data_.size() - 4); i += 4) {
			screen_color_data_[i + 0] = bus_.Palette[0][0];
			screen_color_data_[i + 1] = bus_.Palette[0][1];
			screen_color_data_[i + 2] = bus_.Palette[0][2];
			screen_color_data_[i + 3] = 255.0f;
		}
		LY = 0x0;
		LCDC = 0b1001'0001;
		STAT = 0b1000'0000;
		next_stat_mode = 3;
		clock_ = 0;
		clock_target_ = 0;
	}
    int PPU::CalcCycles() {
		return clock_target_ - clock_;
	}
	float* PPU::GetScreenData() {
		return &screen_color_data_[0];
	}
	int PPU::set_mode(int mode) {
		mode &= 0b11;
		STAT &= 0b1111'1100;
		STAT |= mode;
		if (mode != 3 && (STAT & (1 << (mode + 3)))) {
			return IFInterrupt::LCDSTAT;
		}
		return 0;
	}
	int PPU::get_mode() {
		return STAT & STATFlag::MODE;
	}

	int PPU::update_lyc() {
		if (LYC == LY) {
			STAT |= STATFlag::COINCIDENCE;
			if (STAT & STATFlag::COINC_INTER)
				return IFInterrupt::LCDSTAT;
		}
		else {
			STAT &= 0b1111'1011;
		}
		return 0;
	}

	void PPU::draw_scanline() {
		if (LCDC & 0b1) {
			renderTiles();
		}
		if (LCDC & 0b10) {
			renderSprites();
		}
	}

	inline void PPU::renderTiles() {
		uint16_t tileData = (LCDC & 0b10000) ? 0x8000 : 0x8800;

		uint8_t scrollY = bus_.Read(0xFF42);
		uint8_t scrollX = bus_.Read(0xFF43);
		uint8_t windowY = bus_.Read(0xFF4A);
		uint8_t windowX = bus_.Read(0xFF4B) - 7;
		bool unsig = true;
		if (tileData == 0x8800) {
			unsig = false;
		}
		bool windowEnabled = false;

		// is the window enabled?f
		if (LCDC & 0b100000) {
			// there is no point in drawing the window if its located under the current scanline
			if (windowY <= bus_.Read(0xFF44)) {
				windowEnabled = true;
			}

		}
		uint16_t identifierLocation;
		uint8_t positionY = 0;
		// if window is enabled we use the window background memory map per Pan doc gb docs
		if (windowEnabled) {
			identifierLocation = (LCDC & 0b1000000) ? 0x9C00 : 0x9800;
			// if the window is enabled we can get the y-position of the place we need to draw via
			// window y since window y tells us distance from the area of the first place we need to draw
			positionY = bus_.Read(0xFF44) - windowY;
		}
		else {
			identifierLocation = (LCDC & 0b1000) == 1 ? 0x9C00 : 0x9800;
			// if windows is not enabled we get the edge y-position of the area we need to draw
			// by adding current scanline and scroll-y
			positionY = bus_.Read(0xFF44) + scrollY;
		}

		uint16_t tileRow = (((uint8_t)(positionY / 8)) * 32);

		// draw pixels horizontally
		for (int pixel = 0; pixel < 160; pixel++) {
			uint8_t positionX = pixel + scrollX;
			if (windowEnabled) {
				if (pixel >= windowX) {
					positionX = pixel - windowX;
				}
			}

			uint16_t tileCol = (positionX / 8);

			int16_t tileNumber;

			uint16_t tileAddress = identifierLocation + tileRow + tileCol;

			if (unsig) {
				tileNumber = bus_.Read(tileAddress);
			}
			else {
				tileNumber = static_cast<int8_t>(bus_.Read(tileAddress));
			}

			uint16_t tileLocation = tileData;

			if (unsig) {
				tileLocation += tileNumber * 16;
			}
			else {
				tileLocation += (tileNumber + 128) * 16;
			}

			// which col of tile
			uint8_t line = (positionY % 8);
			line *= 2;

			uint8_t data1 = bus_.Read(tileLocation + line);
			uint8_t data2 = bus_.Read(tileLocation + line + 1);

			int colorBit = positionX % 8;
			// pixel 0 is bit 7, so on
			// 0 - 7 is -7, multiplied by -1 is 7 so we can get the 7th bit of the data
			colorBit -= 7;
			colorBit *= -1;

			int colorNum = (data2 >> colorBit) & 0x1;
			colorNum <<= 1;
			colorNum |= (data1 >> colorBit) & 0x1;
			int finaly = bus_.Read(0xFF44);

			// safety check to make sure what im about 
			// to set is int the 160x144 bounds
			int idx = (pixel * 4) + (finaly * 4 * 160);
			if (LCDC & 0b01) {
				screen_color_data_[idx++] = bus_.Palette[bus_.BGPalette[colorNum]][0];
				screen_color_data_[idx++] = bus_.Palette[bus_.BGPalette[colorNum]][1];
				screen_color_data_[idx++] = bus_.Palette[bus_.BGPalette[colorNum]][2];
				screen_color_data_[idx] = 255.0f;
			}
		}
	}

	void PPU::renderSprites() {
		bool use8x16 = false;
		if (LCDC & 0b100)
			use8x16 = true;

		for (int sprite = 0; sprite < 40; sprite++) {
			uint8_t positionY = bus_.oam_[sprite * 4] - 16;
			uint8_t positionX = bus_.oam_[sprite * 4 + 1] - 8;
			uint8_t tileLoc = bus_.oam_[sprite * 4 + 2];
			uint8_t attributes = bus_.oam_[sprite * 4 + 3];

			bool yFlip = attributes & 0b1000000;
			bool xFlip = attributes & 0b100000;

			int scanLine = bus_.Read(0xFF44);
			int height = 8;
			if (use8x16) {
				height = 16;
			}

			if ((scanLine >= positionY) && (scanLine < (positionY + height))) {
				int line = scanLine - positionY;

				// if a sprite is flipped we read data from opposite side of table 
				if (yFlip) {
					line -= height - 1;
					line *= -1;
				}

				line *= 2;

				uint16_t address = (0x8000 + (tileLoc * 16) + line);
				uint8_t data1 = bus_.Read(address);
				uint8_t data2 = bus_.Read(address + 1);


				for (int tilePixel = 7; tilePixel >= 0; tilePixel--) {
					int colorbit = tilePixel;
					if (xFlip) {
						colorbit -= 7;
						colorbit *= -1;
					}

					int colorNum = (data2 >> colorbit) & 0x1;
					colorNum <<= 1;
					colorNum |= (data1 >> colorbit) & 0x1;

					bool obp1 = (attributes & 0b10000);
					uint8_t color = 1;
					if (obp1) {
						color = bus_.OBJ1Palette[colorNum];
					}
					else {
						color = bus_.OBJ0Palette[colorNum];
					}

					if (color == 0) {
						continue;
					}


					int xPix = 0 - tilePixel;
					xPix += 7;

					int pixel = positionX + xPix;


					if ((scanLine < 0) || (scanLine > 143) || (pixel < 0) || (pixel > 159))
					{
						continue;
					}

					int idx = (pixel * 4) + (scanLine * 4 * 160);
					if (LCDC & 0b10) {
						screen_color_data_[idx++] = bus_.Palette[color][0];
						screen_color_data_[idx++] = bus_.Palette[color][1];
						screen_color_data_[idx++] = bus_.Palette[color][2];
						screen_color_data_[idx] = 255;
					}
				}
			}
		}
	}
	void PPU::FillTileset(float* pixels, size_t x_off, size_t y_off, uint16_t addr) {
		for (int y_ = 0; y_ < 16; ++y_) {
			for (int x_ = 0; x_ < 16; ++x_) {
				for (size_t i = 0; i < 16; i += 2) {
					uint16_t curr_addr = addr + i + x_ * 16 + y_ * 256;
					uint8_t data1 = bus_.Read(curr_addr);
					uint8_t data2 = bus_.Read(curr_addr + 1);
					int x = ((i / 16) + x_) * 8;
					int y = i / 2 + y_ * 8;
					size_t start_idx = y * 256 * 4 + x * 4 + x_off * 4 + y_off * 256 * 4;
					for (size_t j = 0; j < 8; j++) {
						int color = (((data1 >> (7 - j)) & 0b1) << 1) + ((data2 >> (7 - j)) & 0b1);
						pixels[start_idx + (j * 4) + 0] = bus_.Palette[color][0];
						pixels[start_idx + (j * 4) + 1] = bus_.Palette[color][1];
						pixels[start_idx + (j * 4) + 2] = bus_.Palette[color][2];
						pixels[start_idx + (j * 4) + 3] = 255.0f;
					}
				}
			}
		}
	}
}
