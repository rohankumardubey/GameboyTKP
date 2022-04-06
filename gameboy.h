#pragma once
#ifndef TKP_GB_GAMEBOY_H
#define TKP_GB_GAMEBOY_H
// TODO: twitch plays plugin
#include <array>
#include "../include/emulator.h"
#include "../include/disassembly_instr.h"
#include "gb_breakpoint.h"
#include "gb_addresses.h"
#include "gb_cpu.h"
#include "gb_ppu.h"
#include "gb_bus.h"
#include "gb_timer.h"
#include "gb_apu.h"
namespace TKPEmu::Gameboy {
	class Gameboy : public Emulator {
	private:
		using GameboyPalettes = std::array<std::array<float, 3>,4>;
		using GameboyKeys = std::array<SDL_Keycode, 4>;
		using CPU = TKPEmu::Gameboy::Devices::CPU;
		using PPU = TKPEmu::Gameboy::Devices::PPU;
		using APU = TKPEmu::Gameboy::Devices::APU;
		using Bus = TKPEmu::Gameboy::Devices::Bus;
		using Timer = TKPEmu::Gameboy::Devices::Timer;
		using Cartridge = TKPEmu::Gameboy::Devices::Cartridge;
		using DisInstr = TKPEmu::Tools::DisInstr;
		using GameboyBreakpoint = TKPEmu::Gameboy::Utils::GameboyBreakpoint;
	public:
		Gameboy();
		Gameboy(GameboyKeys dirkeys, GameboyKeys actionkeys);
		~Gameboy();
		void HandleKeyDown(SDL_Keycode key) override;
		void HandleKeyUp(SDL_Keycode key) override;
		float* GetScreenData() override;
		std::string GetEmulatorName() override;
		std::string GetScreenshotHash() override;
		bool IsReadyToDraw() override;
		void SetLogTypes(std::unique_ptr<std::vector<LogType>> types_ptr);
        DisInstr GetInstruction(uint16_t address);
		bool AddBreakpoint(GBBPArguments bp);
		bool* DebugSpriteTint();
		bool* GetDrawSprites();
		bool* GetDrawWindow();
		bool* GetDrawBackground();
		void RemoveBreakpoint(int index);
		void SetKeysLate(GameboyKeys dirkeys, GameboyKeys actionkeys);
		const auto& GetOpcodeDescription(uint8_t opc);
		GameboyPalettes& GetPalette();
		Cartridge& GetCartridge();
		CPU& GetCPU() { return cpu_; }
		PPU& GetPPU() { return ppu_; }
		std::vector<GameboyBreakpoint> Breakpoints{};
		std::vector<DisInstr> Instructions{};
	private:
		APU apu_;
		Bus bus_;
		PPU ppu_;
		Timer timer_;
		CPU cpu_;
		GameboyKeys direction_keys_;
		GameboyKeys action_keys_;
		uint8_t& joypad_, &interrupt_flag_;
		std::chrono::system_clock::time_point frame_start = std::chrono::system_clock::now();
		std::chrono::system_clock::time_point second_start = std::chrono::system_clock::now();
		int frames = 0;
		int frame_counter = 0;
		std::unique_ptr<std::vector<LogType>> log_types_ptr_;
		void v_log_state() override;
		void save_state(std::ofstream& ofstream) override;
		void load_state(std::ifstream& ifstream) override;
		void start_normal() override;
		void start_debug() override;
		void start_console() override;
		void reset_normal() override;
		void reset_skip() override;
		bool load_file(std::string path) override;
		void update() override;
		void init_image();
		std::string print() const override;
	};
}
#endif
