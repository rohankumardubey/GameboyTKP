#pragma once
#ifndef TKP_GB_GAMEBOY_H
#define TKP_GB_GAMEBOY_H
#include <array>
#include <include/emulator.h>
#include <GameboyTKP/gb_breakpoint.h>
#include <GameboyTKP/gb_addresses.h>
#include <GameboyTKP/gb_cpu.h>
#include <GameboyTKP/gb_ppu.h>
#include <GameboyTKP/gb_bus.h>
#include <GameboyTKP/gb_timer.h>
#include <GameboyTKP/gb_apu.h>
#include <GameboyTKP/gb_apu_ch.h>

namespace TKPEmu {
	namespace Applications {
		class GameboyRomData;
	}
	namespace Gameboy::QA {
		struct TestMooneye;
	}
}
namespace TKPEmu::Gameboy {
	class Gameboy_TKPWrapper : public Emulator {
		TKP_EMULATOR(Gameboy_TKPWrapper);
	private:
		using GameboyPalettes = std::array<std::array<float, 3>,4>;
		using GameboyKeys = std::array<uint32_t, 4>;
		using CPU = TKPEmu::Gameboy::Devices::CPU;
		using PPU = TKPEmu::Gameboy::Devices::PPU;
		using APU = TKPEmu::Gameboy::Devices::APU;
		using ChannelArrayPtr = TKPEmu::Gameboy::Devices::ChannelArrayPtr;
		using ChannelArray = TKPEmu::Gameboy::Devices::ChannelArray;
		using Bus = TKPEmu::Gameboy::Devices::Bus;
		using Timer = TKPEmu::Gameboy::Devices::Timer;
		using Cartridge = TKPEmu::Gameboy::Devices::Cartridge;
		using GameboyBreakpoint = TKPEmu::Gameboy::Utils::GameboyBreakpoint;
	public:
		// Used by automated tests
		void Update() { update(); }
	private:
		ChannelArrayPtr channel_array_ptr_;
		Bus bus_;
		APU apu_;
		PPU ppu_;
		Timer timer_;
		CPU cpu_;
		GameboyKeys direction_keys_;
		GameboyKeys action_keys_;
		uint8_t& joypad_, &interrupt_flag_;
		void update();
		// this is the old update function that was replaced by update_audio_sync
		// keeping it anyway
		__always_inline void update_audio_sync();
		__always_inline void v_log() override;
	};
}
#endif
