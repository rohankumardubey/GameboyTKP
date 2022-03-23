#include <iostream>
#include <atomic>
#include <chrono>
#include <syncstream>
#include <GL/glew.h>
#include "gameboy.h"
#include "../lib/md5.h"
#include "../include/console_colors.h"
namespace TKPEmu::Gameboy {
	Gameboy::Gameboy() : 
		apu_(),
		bus_(apu_, Instructions),
		ppu_(bus_, &DrawMutex),
		timer_(bus_, apu_),
		cpu_(bus_, ppu_, timer_),
		joypad_(bus_.GetReference(addr_joy)),
		interrupt_flag_(bus_.GetReference(addr_if))
	{
		EmulatorImage.width = 160;
		EmulatorImage.height = 144;
	}
	Gameboy::Gameboy(GameboyKeys dirkeys, GameboyKeys actionkeys) :
		Gameboy()
	{
		SetKeysLate(dirkeys, actionkeys);
		init_image();
	}
	Gameboy::~Gameboy() {
		Stopped.store(true);
		if (start_options != EmuStartOptions::Console)
			glDeleteTextures(1, &EmulatorImage.texture);
	}
	void Gameboy::SetKeysLate(GameboyKeys dirkeys, GameboyKeys actionkeys) {
		direction_keys_ = dirkeys;
		action_keys_ = actionkeys;
	}
	void Gameboy::SetLogTypes(std::unique_ptr<std::vector<LogType>> types_ptr) {
		log_types_ptr_ = std::move(types_ptr);
	}
	std::string Gameboy::GetScreenshotHash() {
		return md5(bus_.GetVramDump());
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
	bool Gameboy::IsReadyToDraw() {
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
	void Gameboy::start_console() {
		Paused = false;
		Stopped = false;
		Reset();
		while (!Stopped.load()) {
			if (!Paused.load()) {
				update();
				if (cpu_.TotalClocks == ScreenshotClocks - 1) {
					std::osyncstream scout(std::cout);
					if (ScreenshotHash == GetScreenshotHash()) {
						scout << "[" << color_success << CurrentFilename << color_reset "]: Passed" << std::endl;
						Result = TKPEmu::Testing::TestResult::Passed;
					} else {
						scout << "[" << color_error << CurrentFilename << color_reset "]: Failed" << std::endl;
						Result = TKPEmu::Testing::TestResult::Failed;
					}
					Stopped = true;
				}
				if (action_ptr_ && *action_ptr_ != 0) {
                    SDL_Keycode key = SDLK_UNKNOWN;
                    switch (*action_ptr_) {
                        case 1: {
                            key = SDLK_UP;
                            break;
                        }
                        case 2: {
                            key = SDLK_RIGHT;
                            break;
                        }
                        case 3: {
                            key = SDLK_DOWN;
                            break;
                        }
                        case 4: {
                            key = SDLK_LEFT;
                            break;
                        }
                        case 5: {
                            key = SDLK_z;
                            break;
                        }
                        case 6: {
                            key = SDLK_x;
                            break;
                        }
                        case 7: {
                            key = SDLK_RETURN;
                            break;
                        }
                    }
                    if (key != SDLK_UNKNOWN) {
                        HandleKeyDown(key);
                        std::thread th([this, &key]() {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                            HandleKeyUp(key);
                            std::this_thread::sleep_for(std::chrono::milliseconds(400));
                            Screenshot("image.bmp");
                        });
                        th.detach();
                    }
                    *action_ptr_ = 0;

                }
			}
		}
	}
	void Gameboy::start_debug() {
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
					bool broken = false;
					if (!first_instr) {
						for (const auto& bp : Breakpoints) {
							bool brk = bp.Check();
							if (brk) {
								InstructionBreak.store(cpu_.PC);
								Paused.store(true);
								broken = true;
							}
						}
					}
					first_instr = false;
					if (!broken)
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
		if ((cpu_.TClock / 2) < cpu_.MaxCycles || FastMode) {
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
			if (!cpu_.halt_)
				log_state();
		} else {
			auto end = std::chrono::system_clock::now();
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - frame_start).count();
			if (dur > 16.6f) {
				frame_start = std::chrono::system_clock::now();
				cpu_.TClock = 0;
			}
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
	void Gameboy::load_file(std::string path) {
		bus_.LoadCartridge(std::forward<std::string>(path));
	}
	DisInstr Gameboy::GetInstruction(uint16_t address) {
		uint8_t ins = bus_.Read(address);
		auto time = InstrTimes[ins];
		auto instr = DisInstr(address, ins, time);
		uint8_t p1 = 0, p2 = 0;
		if (time == 1) {
			p1 = bus_.Read(address + 1);
			instr.Params[0] = p1;
			return instr;
		}
		else if (time == 2) {
			p1 = bus_.Read(address + 1);
			p2 = bus_.Read(address + 2);
			instr.Params[0] = p1;
			instr.Params[1] = p2;
			return instr;
		}
		else {
			return instr;
		}
	}
	bool Gameboy::AddBreakpoint(GBBPArguments bp) {
		using RegCheckVector = std::vector<std::function<bool()>>;
		RegCheckVector register_checks;
		// TODO: Check if breakpoint already exists before adding it
		// We calculate which of these checks we need, and add them all to a vector to save execution time
		// Before being copied to the lambda, the values are decremented, as to keep them (1-256) -> (0-255)
		// because we used the value of 0 to check whether this is used or not.
		if (bp.A_using) { register_checks.push_back([this, gbbp = bp.A_value]() { return cpu_.A == gbbp; }); }
		if (bp.B_using) { register_checks.push_back([this, gbbp = bp.B_value]() { return cpu_.B == gbbp; }); }
		if (bp.C_using) { register_checks.push_back([this, gbbp = bp.C_value]() { return cpu_.C == gbbp; }); }
		if (bp.D_using) { register_checks.push_back([this, gbbp = bp.D_value]() { return cpu_.D == gbbp; }); }
		if (bp.E_using) { register_checks.push_back([this, gbbp = bp.E_value]() { return cpu_.E == gbbp; }); }
		if (bp.F_using) { register_checks.push_back([this, gbbp = bp.F_value]() { return cpu_.F == gbbp; }); }
		if (bp.H_using) { register_checks.push_back([this, gbbp = bp.H_value]() { return cpu_.H == gbbp; }); }
		if (bp.L_using) { register_checks.push_back([this, gbbp = bp.L_value]() { return cpu_.L == gbbp; }); }
		if (bp.PC_using) { register_checks.push_back([this, gbbp = bp.PC_value]() { return cpu_.PC == gbbp; }); }
		if (bp.SP_using) { register_checks.push_back([this, gbbp = bp.SP_value]() { return cpu_.SP == gbbp; }); }
		if (bp.SP_using) { register_checks.push_back([this, gbbp = bp.SP_value]() { return cpu_.SP == gbbp; }); }
		if (bp.Ins_using) { register_checks.push_back([this, gbbp = bp.Ins_value]() { return (bus_.Read(cpu_.PC)) == gbbp; }); }
		if (bp.Clocks_using) { register_checks.push_back([this, gbbp = bp.Clocks_value]() { return cpu_.TotalClocks == gbbp; }); }
		auto lamb = [rc = std::move(register_checks)]() {
			for (auto& check : rc) {
				if (!check()) {
					// If any of the checks fails, that means the breakpoint shouldn't trigger
					return false;
				}
			}
			// Every check is passed, trigger breakpoint
			return true;
		};
		GameboyBreakpoint gbp;
		gbp.Args = std::move(bp);
		gbp.SetChecks(std::move(lamb));
		std::cout << "Breakpoint added:\n" << gbp.GetName() << std::endl;
		bool ret = false;
		if (gbp.BPFromTable)
			ret = true;
		Breakpoints.push_back(std::move(gbp));
		return ret;
	}
	void Gameboy::RemoveBreakpoint(int index) {
		Breakpoints.erase(Breakpoints.begin() + index);
	}
	float* Gameboy::GetScreenData() {
		return ppu_.GetScreenData();
	}
	std::string Gameboy::GetEmulatorName() {
		return "GameboyTKP";
	}
	Devices::Cartridge& Gameboy::GetCartridge() {
		return bus_.GetCartridge();
	}
	const auto& Gameboy::GetOpcodeDescription(uint8_t opc) {
		return cpu_.Instructions[opc].name;
	}
	void Gameboy::init_image() {
		GLuint image_texture;
		glGenTextures(1, &image_texture);
		glBindTexture(GL_TEXTURE_2D, image_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, image_texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			160,
			144,
			0,
			GL_RGBA,
			GL_FLOAT,
			NULL
		);
		glBindTexture(GL_TEXTURE_2D, 0);
		EmulatorImage.texture = image_texture;
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
