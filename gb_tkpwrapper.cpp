#include <iostream>
#include <atomic>
#include <chrono>
#include <syncstream>
#include <GL/glew.h>
#include <filesystem>
// #include <valgrind/callgrind.h>
#include <GameboyTKP/gb_tkpwrapper.h>
#include <lib/md5.h>
#include <include/console_colors.h>
#ifndef CALLGRIND_START_INSTRUMENTATION
#define CALLGRIND_START_INSTRUMENTATION
#endif
#ifndef CALLGRIND_STOP_INSTRUMENTATION
#define CALLGRIND_STOP_INSTRUMENTATION
#endif
namespace TKPEmu::Gameboy {
	Gameboy::Gameboy() : 
		channel_array_ptr_(std::make_shared<ChannelArray>()),
		bus_(channel_array_ptr_),
		apu_(channel_array_ptr_, bus_.GetReference(addr_NR52)),
		ppu_(bus_, &DrawMutex),
		timer_(channel_array_ptr_, bus_),
		cpu_(bus_, ppu_, apu_, timer_),
		joypad_(bus_.GetReference(addr_joy)),
		interrupt_flag_(bus_.GetReference(addr_if))
	{
		(*channel_array_ptr_.get())[0].HasSweep = true;
	}
	Gameboy::Gameboy(std::any args) :
		Gameboy()
	{
		if (args.has_value()) {
			auto keys = std::any_cast<std::pair<GameboyKeys, GameboyKeys>>(args);
			SetKeysLate(keys.first, keys.second);
		}
	}
	Gameboy::~Gameboy() {
		Stopped.store(true);
	}
	void Gameboy::SetKeysLate(GameboyKeys dirkeys, GameboyKeys actionkeys) {
		direction_keys_ = dirkeys;
		action_keys_ = actionkeys;
	}
	void Gameboy::SetLogTypes(std::unique_ptr<std::vector<LogType>> types_ptr) {
		log_types_ptr_ = std::move(types_ptr);
	}
	void Gameboy::v_log_state() {
		*ofstream_ptr_ << std::setfill('0');
		int inst = bus_.Read(cpu_.PC);
		for (const auto& t : *log_types_ptr_) {
			switch (t) {
				case LogType::InstrName: {
					*ofstream_ptr_ << std::setfill(' ') << std::setw(8) << cpu_.Instructions[inst].name << std::setfill('0');
					break;
				}
				case LogType::InstrNum: {
					*ofstream_ptr_ << std::setw(2) << std::hex << inst;
					break;
				}
				case LogType::PC: {
					*ofstream_ptr_ << std::setw(4) << std::hex << cpu_.PC << ":";
					break;
				}
				case LogType::SP: {
					*ofstream_ptr_ << "SP:" << std::setw(4) << std::hex <<  cpu_.SP;
					break;
				}
				case LogType::A: {
					*ofstream_ptr_ << "A:" << std::setw(2) << std::hex << static_cast<int>(cpu_.A);
					break;
				}
				case LogType::B: {
					*ofstream_ptr_ << "B:" << std::setw(2) << std::hex << static_cast<int>(cpu_.B);
					break;
				}
				case LogType::C: {
					*ofstream_ptr_ << "C:" << std::setw(2) <<  std::hex << static_cast<int>(cpu_.C);
					break;
				}
				case LogType::D: {
					*ofstream_ptr_ << "D:" << std::setw(2) << std::hex << static_cast<int>(cpu_.D);
					break;
				}
				case LogType::E: {
					*ofstream_ptr_ << "E:" << std::setw(2) << std::hex << static_cast<int>(cpu_.A);
					break;
				}
				case LogType::F: {
					*ofstream_ptr_ << "F:" <<  std::setw(2) << std::hex << static_cast<int>(cpu_.F);
					break;
				}
				case LogType::H: {
					*ofstream_ptr_ << "H:" << std::setw(2) << std::hex << static_cast<int>(cpu_.H);
					break;
				}
				case LogType::L: {
					*ofstream_ptr_ << "L:" << std::setw(2) << std::hex << static_cast<int>(cpu_.L);
					break;
				}
				case LogType::LY: {
					*ofstream_ptr_ << "LY:" << std::setw(2) << std::hex << static_cast<int>(cpu_.LY);
					break;
				}
				case LogType::IF: {
					*ofstream_ptr_ << "IF:" << std::setw(2) << std::hex << static_cast<int>(cpu_.IF);
					break;
				}
				case LogType::IE: {
					*ofstream_ptr_ << "IE:" << std::setw(2) << std::hex << static_cast<int>(cpu_.IE);
					break;
				}
				case LogType::IME: {
					*ofstream_ptr_ << "IME:" << static_cast<int>(cpu_.ime_);
					break;
				}
				case LogType::HALT: {
					*ofstream_ptr_ << "HALT:" << static_cast<int>(cpu_.halt_);
					break;
				}
			}
			*ofstream_ptr_ << " ";
		}
		*ofstream_ptr_ << "\n";
	}
	bool& Gameboy::IsReadyToDraw() {
		return ppu_.ReadyToDraw;
	}
	bool* Gameboy::DebugSpriteTint() {
		return &ppu_.SpriteDebugColor;
	}
	void Gameboy::save_state(std::ofstream& ofstream) {
		// TODO: After finishing apu, add all apu settings to save_state and load
		if (!Paused) {
			Paused = true;
		}
		std::lock_guard<std::mutex> lg(DebugUpdateMutex);
		bus_.TransferDMA(160); // Finish dma transfer just in case
		ofstream << bus_.selected_ram_bank_;
		ofstream << bus_.selected_rom_bank_;
		ofstream << cpu_.A << cpu_.F << cpu_.B << cpu_.C << cpu_.D << cpu_.E << cpu_.H << cpu_.L << (uint8_t)(cpu_.PC >> 8) 
  	    << (uint8_t)(cpu_.PC & 0xFF) << (uint8_t)(cpu_.SP >> 8) << (uint8_t)(cpu_.SP & 0xFF) << (uint8_t)cpu_.ime_ << (uint8_t)cpu_.halt_;
		for (uint16_t i = 0x8000; i < 0xA000; ++i) {
			ofstream << bus_.Read(i);
		}
		auto& rambanks = bus_.GetRamBanks();
		for (int i = 0; i < bus_.GetCartridge().GetRamSize(); ++i) {
			auto& cur_bank = rambanks[i];
			for (uint8_t data : cur_bank) {
				ofstream << data;
			}
		}
		for (int i = 0xC000; i < 0xF000; ++i) {
			ofstream << bus_.Read(i);
		}
		for (int i = 0xFE00; i <= 0xFFFF; ++i) {
			ofstream << bus_.Read(i);
		}
		ofstream.close();
		Paused.store(false);
		Step.store(true);
		Step.notify_all();
	}
	void Gameboy::load_state(std::ifstream& ifstream) {
		// TODO: God, fix this ugly shit
		if (!Paused) {
			Paused = true;
		}
		std::lock_guard<std::mutex> lg(DebugUpdateMutex);
		bus_.TransferDMA(160); // Finish dma transfer just in case
		uint8_t pc_high, pc_low, sp_high, sp_low;
		ifstream.read(reinterpret_cast<char*>(&bus_.selected_ram_bank_), 1);
		ifstream.read(reinterpret_cast<char*>(&bus_.selected_rom_bank_), 1);
		ifstream.read(reinterpret_cast<char*>(&cpu_.A), 1);
		ifstream.read(reinterpret_cast<char*>(&cpu_.F), 1);
		ifstream.read(reinterpret_cast<char*>(&cpu_.B), 1);
		ifstream.read(reinterpret_cast<char*>(&cpu_.C), 1);
		ifstream.read(reinterpret_cast<char*>(&cpu_.D), 1);
		ifstream.read(reinterpret_cast<char*>(&cpu_.E), 1);
		ifstream.read(reinterpret_cast<char*>(&cpu_.H), 1);
		ifstream.read(reinterpret_cast<char*>(&cpu_.L), 1);
		ifstream.read(reinterpret_cast<char*>(&pc_high), 1);
		ifstream.read(reinterpret_cast<char*>(&pc_low), 1);
		ifstream.read(reinterpret_cast<char*>(&sp_high), 1);
		ifstream.read(reinterpret_cast<char*>(&sp_low), 1);
		cpu_.PC = (pc_high << 8) | pc_low;
		cpu_.SP = (sp_high << 8) | sp_low;
		ifstream.read(reinterpret_cast<char*>(&cpu_.ime_), 1);
		ifstream.read(reinterpret_cast<char*>(&cpu_.halt_), 1);
		for (uint16_t i = 0x8000; i < 0xA000; ++i) {
			uint8_t data; ifstream.read(reinterpret_cast<char*>(&data), 1);
			bus_.Write(i, data);
		}
		auto& rambanks = bus_.GetRamBanks();
		for (int i = 0; i < bus_.GetCartridge().GetRamSize(); ++i) {
			auto& cur_bank = rambanks[i];
			for (uint8_t& data : cur_bank) {
				ifstream.read(reinterpret_cast<char*>(&data), 1);
			}
		}
		for (uint16_t i = 0xC000; i < 0xF000; ++i) {
			uint8_t data; ifstream.read(reinterpret_cast<char*>(&data), 1);
			bus_.Write(i, data);
		}
		for (int i = 0xFE00; i <= 0xFFFF; ++i) {
			uint8_t data; ifstream.read(reinterpret_cast<char*>(&data), 1);
			bus_.Write(i, data);
		}
		bus_.BiosEnabled = false;
		ifstream.close();
		Paused.store(false);
		Step.store(true);
		Step.notify_all();
	}
	void Gameboy::start_normal() {
		apu_.UseSound = true;
		apu_.InitSound();
		Reset();
		auto func = [this]() {
			std::lock_guard<std::mutex> lguard(ThreadStartedMutex);
			while (!Stopped.load()) {
				if (!Paused.load()) {
					std::lock_guard<std::mutex> lg(DebugUpdateMutex);
					update();
				}
			}
			std::terminate();
		};
		UpdateThread = std::thread(func);
		UpdateThread.detach();
	}
	void Gameboy::start_debug() {
		apu_.UseSound = true;
		apu_.InitSound();
		auto func = [this]() {
			std::lock_guard<std::mutex> lguard(ThreadStartedMutex);
			Loaded = true;
			Loaded.notify_all();
			Paused = true;
			Stopped = false;
			Step = false;
			Reset();
			// Emulation doesn't break on first instruction
			bool first_instr = true;
			while (!Stopped.load()) {
				if (!Paused.load()) {
					std::lock_guard<std::mutex> lg(DebugUpdateMutex);
					update();
				} else {
					Step.wait(false);
					std::lock_guard<std::mutex> lg(DebugUpdateMutex);
					Step.store(false);
					update();
					InstructionBreak.store(cpu_.PC);
				}
			}
		};
		UpdateThread = std::thread(func);
		UpdateThread.detach();
	}
	std::string Gameboy::GetScreenshotHash() {
		// return md5(bus_.GetVramDump());
		return ""; // TODO: wtf bug?
	}
	std::vector<std::string> Gameboy::Disassemble(std::string instr) {
		auto get_hex = [](char c) {
			switch (c) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				return uint8_t(c - '0');
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
				return uint8_t(c - 'A' + 10);
			}
			return uint8_t(0);
		};
		std::vector<std::string> ret;
		uint8_t ins = (get_hex(instr[0]) << 4) | get_hex(instr[1]);
		ret.push_back(cpu_.Instructions[ins].name);
		if (instr.size() == 4)
			ret.push_back(instr.substr(2, 2));
		else if (instr.size() == 6)
			ret.push_back(instr.substr(2, 4));
		return ret;
	}
	void Gameboy::reset_normal() {
		bus_.SoftReset();
		cpu_.Reset(false);
		timer_.Reset();
		ppu_.Reset();
	}
	void Gameboy::reset_skip() {
		bus_.SoftReset();
		cpu_.Reset(true);
		timer_.Reset();
		ppu_.Reset();
	}
	void Gameboy::update() {
		update_audio_sync();
	}
	void Gameboy::update_spinloop() {
		if ((cpu_.TClock < cpu_.MaxCycles) || FastMode) {
			CALLGRIND_START_INSTRUMENTATION;
			if (cpu_.PC == 0x100) {
				bus_.BiosEnabled = false;
			}
			uint8_t old_if = interrupt_flag_;
			int clk = 0;
			if (!cpu_.skip_next_)
				clk = cpu_.Update();
			cpu_.skip_next_ = false;
			if (timer_.Update(clk, old_if)) {
				if (cpu_.halt_) {
					cpu_.halt_ = false;
					cpu_.skip_next_ = true;
				}
			}
			ppu_.Update(clk);
			apu_.Update(clk);
			if (!cpu_.halt_)
				log_state();
			CALLGRIND_STOP_INSTRUMENTATION;
		} else {
			auto end = std::chrono::system_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - frame_start).count();
			if (apu_.IsQueueEmpty()) {
				// Audio starved! Queue last sample :(
				apu_.QueueSamples();
			}
			if (dur > 16.6f) {
				frame_start = std::chrono::system_clock::now();
				cpu_.TClock = 0;
			}
		}
	}
	void Gameboy::update_audio_sync() {
		if ((apu_.IsQueueEmpty()) || FastMode) {
			CALLGRIND_START_INSTRUMENTATION;
			uint8_t old_if = interrupt_flag_;
			int clk = 0;
			if (!cpu_.skip_next_)
				clk = cpu_.Update();
			cpu_.skip_next_ = false;
			if (timer_.Update(clk, old_if)) {
				if (cpu_.halt_) {
					cpu_.halt_ = false;
					cpu_.skip_next_ = true;
				}
			}
			ppu_.Update(clk);
			apu_.Update(clk);
			if (!cpu_.halt_)
				log_state();
			CALLGRIND_STOP_INSTRUMENTATION;
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	std::string Gameboy::print() const { 
		return "GameboyTKP for TKPEmu\n"
		       "Read more: https://github.com/OFFTKP/GameboyTKP/";
	}
	void Gameboy::HandleKeyDown(SDL_Keycode key) {
		if (auto it_dir = std::find(direction_keys_.begin(), direction_keys_.end(), key); it_dir != direction_keys_.end()) {
			int index = it_dir - direction_keys_.begin();
			bus_.DirectionKeys &= (~(1UL << index));
			interrupt_flag_ |= IFInterrupt::JOYPAD;
		}
		if (auto it_dir = std::find(action_keys_.begin(), action_keys_.end(), key); it_dir != action_keys_.end()) {
			int index = it_dir - action_keys_.begin();
			bus_.ActionKeys &= (~(1UL << index));
			interrupt_flag_ |= IFInterrupt::JOYPAD;
		}
	}
	void Gameboy::HandleKeyUp(SDL_Keycode key) {
		if (auto it_dir = std::find(direction_keys_.begin(), direction_keys_.end(), key); it_dir != direction_keys_.end()) {
			int index = it_dir - direction_keys_.begin();
			bus_.DirectionKeys |= (1UL << index);
		}
		if (auto it_dir = std::find(action_keys_.begin(), action_keys_.end(), key); it_dir != action_keys_.end()) {
			int index = it_dir - action_keys_.begin();
			bus_.ActionKeys |= (1UL << index);
		}
	}
	bool Gameboy::load_file(std::string path) {
		auto loaded = bus_.LoadCartridge(std::forward<std::string>(path));;
		ppu_.UseCGB = bus_.UseCGB;
		return loaded;
	}
	void* Gameboy::GetScreenData() {
		return ppu_.GetScreenData();
	}
	Devices::Cartridge& Gameboy::GetCartridge() {
		return bus_.GetCartridge();
	}
	const auto& Gameboy::GetOpcodeDescription(uint8_t opc) {
		return cpu_.Instructions[opc].name;
	}
	Gameboy::GameboyPalettes& Gameboy::GetPalette() {
		return bus_.Palette;
	}
	bool* Gameboy::GetDrawBackground() {
		return &ppu_.DrawBackground;
	}
	bool* Gameboy::GetDrawSprites() {
		return &ppu_.DrawSprites;
	}
	bool* Gameboy::GetDrawWindow() {
		return &ppu_.DrawWindow;
	}
}
