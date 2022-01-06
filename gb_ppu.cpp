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
		static int  mode3_extend = 0; // unused for now
		uint8_t current_mode = STAT & STATFlag::MODE;
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
					ReadyToDraw = true;
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
				window_internal_ = 0;
				window_internal_temp_ = 0;
			}
		}
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
		} else {
			STAT &= 0b1111'1011;
		}
		return 0;
	}

	void PPU::draw_scanline() {
		bool enabled = LCDC & LCDCFlag::LCD_ENABLE;
		if (enabled) {
			if (LCDC & LCDCFlag::BG_ENABLE) {
				render_tiles();
			}
			if (LCDC & LCDCFlag::OBJ_ENABLE) {
				render_sprites();
			}
		}
	}

	inline void PPU::render_tiles() {
		uint16_t tileData = (LCDC & LCDCFlag::BG_TILES) ? 0x8000 : 0x8800;
		bool unsig = true;
		if (tileData == 0x8800) {
			unsig = false;
		}
		bool windowEnabled = (LCDC & LCDCFlag::WND_ENABLE && WY <= LY);
		if (WX >= 160 || WX == 0) {
			windowEnabled = false;
		}
		uint16_t identifierLocationW = (LCDC & LCDCFlag::WND_TILEMAP) ? 0x9C00 : 0x9800;
		uint16_t identifierLocationB = (LCDC & LCDCFlag::BG_TILEMAP) ? 0x9C00 : 0x9800;
		uint8_t positionY = LY + SCY;
		if (windowEnabled) {
			++window_internal_temp_;
		} else if (window_internal_temp_) {
			window_internal_ = window_internal_temp_ - 2;
		}
		uint16_t identifierLoc = identifierLocationB;
		uint16_t tileRow = (((uint8_t)(positionY / 8)) * 32);
		for (int pixel = 0; pixel < 160; pixel++) {
			uint8_t positionX = pixel + SCX;
			if (windowEnabled && positionX >= (WX - 7)) {
				identifierLoc = identifierLocationW;
				positionX = pixel - (WX - 7);
				positionY = LY - WY - (window_internal_ * 4);
				tileRow = (((uint8_t)(positionY / 8)) * 32);
			}
			uint16_t tileCol = (positionX / 8);
			int16_t tileNumber;
			uint16_t tileAddress = identifierLoc + tileRow + tileCol;
			uint16_t tileLocation = tileData;
			if (unsig) {
				tileNumber = bus_.Read(tileAddress);
				tileLocation += tileNumber * 16;
			} else {
				tileNumber = static_cast<int8_t>(bus_.Read(tileAddress));
				tileLocation += (tileNumber + 128) * 16;
			}
			uint8_t line = (positionY % 8) * 2;
			uint8_t data1 = bus_.Read(tileLocation + line);
			uint8_t data2 = bus_.Read(tileLocation + line + 1);
			int colorBit = -((positionX % 8) - 7);
			int colorNum = (((data2 >> colorBit) & 0b1) << 1) | ((data1 >> colorBit) & 0b1);
			int idx = (pixel * 4) + (LY * 4 * 160);
			screen_color_data_[idx++] = bus_.Palette[bus_.BGPalette[colorNum]][0];
			screen_color_data_[idx++] = bus_.Palette[bus_.BGPalette[colorNum]][1];
			screen_color_data_[idx++] = bus_.Palette[bus_.BGPalette[colorNum]][2];
			screen_color_data_[idx] = 255.0f;
		}
	}
	void PPU::render_sprites() {
		bool use8x16 = LCDC & LCDCFlag::OBJ_SIZE;
		// Sort depending on X and reverse iterate to correctly select sprite priority
		std::sort(cur_scanline_sprites_.begin(), cur_scanline_sprites_.end(), [this](const auto& lhs, const auto& rhs) {
			return bus_.oam_[lhs + 1] < bus_.oam_[rhs + 1];
		});
		for (auto i = cur_scanline_sprites_.rbegin(); i != cur_scanline_sprites_.rend(); ++i) {
			auto sprite = *i;
			uint8_t positionY = bus_.oam_[sprite] - 16;
			uint8_t positionX = bus_.oam_[sprite + 1] - 8;
			uint8_t tileLoc = bus_.oam_[sprite + 2];
			uint8_t attributes = bus_.oam_[sprite + 3];
			if (use8x16) {
				// dmg-acid2: Bit 0 of tile index for 8x16 objects should be ignored
				tileLoc &= 0b1111'1110;
			}
			bool yFlip = attributes & 0b1000000;
			bool xFlip = attributes & 0b100000;
			int height = use8x16 ? 16 : 8;
			int line = LY - positionY;
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
				int colorNum = ((data2 >> colorbit) & 0b1) << 1;
				colorNum |= (data1 >> colorbit) & 0b1;
				bool obp1 = (attributes & 0b10000);
				uint8_t color = 1;
				if (obp1) {
					color = bus_.OBJ1Palette[colorNum];
				} else {
					color = bus_.OBJ0Palette[colorNum];
				}
				int pixel = positionX - tilePixel + 7;
				if ((LY > 143) || (pixel < 0) || (pixel > 159) || (colorNum == 0)) {
					continue;
				}
				int idx = (pixel * 4) + (LY * 4 * 160);
				if (attributes & 0b1000'0000) {
					if (!(screen_color_data_[idx] == bus_.Palette[0][0] &&
					  	screen_color_data_[idx + 1] == bus_.Palette[0][1] && 
						screen_color_data_[idx + 2] == bus_.Palette[0][2])) {
						continue;
					}
				}
				auto red = bus_.Palette[color][0];
				red += (255.0f - red) * SpriteDebugColor;
				screen_color_data_[idx++] = red;
				screen_color_data_[idx++] = bus_.Palette[color][1];
				screen_color_data_[idx++] = bus_.Palette[color][2];
				screen_color_data_[idx] = 255;
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
