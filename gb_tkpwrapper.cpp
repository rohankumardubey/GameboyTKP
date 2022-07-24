#include <iostream>
#include <atomic>
#include <chrono>
#include <syncstream>
#include <filesystem>
// #include <valgrind/callgrind.h>
#include <GameboyTKP/gb_tkpwrapper.h>
#include <lib/md5.h>
#include <include/console_colors.h>
#include <include/emulator_user_data.hxx>
#include <include/emulator_factory.h>
#ifndef CALLGRIND_START_INSTRUMENTATION
#define CALLGRIND_START_INSTRUMENTATION
#endif
#ifndef CALLGRIND_STOP_INSTRUMENTATION
#define CALLGRIND_STOP_INSTRUMENTATION
#endif
namespace TKPEmu::Gameboy {
	Gameboy_TKPWrapper::Gameboy_TKPWrapper() : 
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
		const EmulatorUserData& user_data = EmulatorFactory::GetEmulatorUserData()[static_cast<int>(EmuType::Gameboy)];
		const KeyMappings& mappings = EmulatorFactory::GetEmulatorData()[static_cast<int>(EmuType::Gameboy)].Mappings;
		if (!mappings.KeyValues.empty()) {
			for (int i = 0; i < 4; i++) {
				direction_keys_[i] = mappings.KeyValues[i];
				action_keys_[i] = mappings.KeyValues[4 + i];
				auto color = std::stoi(user_data.Get(std::string("dmg_c") + std::to_string(i)));
				bus_.Palette[i][0] = color & 0xFF;
				bus_.Palette[i][1] = (color >> 8) & 0xFF;
				bus_.Palette[i][2] = color >> 16;
			}
		}
		if (!user_data.IsEmpty()) {
			if (user_data.Get("skip_bios") == "false") {
				auto dmg_path = user_data.Get("dmg_path");
				auto cgb_path = user_data.Get("cgb_path");
				if (std::filesystem::exists(dmg_path)) {
					std::ifstream ifs(dmg_path, std::ios::binary);
					if (ifs.is_open()) {
						ifs.read(reinterpret_cast<char*>(&bus_.dmg_bios_[0]), sizeof(bus_.dmg_bios_));
						ifs.close();
						bus_.dmg_bios_loaded_ = true;
						bus_.BiosEnabled = true;
					}
				}
				if (std::filesystem::exists(cgb_path)) {
					std::ifstream ifs(cgb_path, std::ios::binary);
					if (ifs.is_open()) {
						ifs.read(reinterpret_cast<char*>(&bus_.cgb_bios_[0]), sizeof(bus_.cgb_bios_));
						ifs.close();
						bus_.cgb_bios_loaded_ = true;
						bus_.BiosEnabled = true;
					}
				}
				SkipBoot = !bus_.BiosEnabled;
			}
		} else {
			SkipBoot = true;
		}
	}
	Gameboy_TKPWrapper::~Gameboy_TKPWrapper() {
		Stopped.store(true);
	}
	bool& Gameboy_TKPWrapper::IsReadyToDraw() {
		return ppu_.ReadyToDraw;
	}
	void Gameboy_TKPWrapper::start() {
		std::lock_guard<std::mutex> lg(ThreadStartedMutex);
		apu_.UseSound = true;
		apu_.InitSound();
		Loaded = true;
		Loaded.notify_all();
		Paused = false;
		Stopped = false;
		Step = false;
		Reset();
		// Emulation doesn't break on first instruction
		bool first_instr = true;
		while (!Stopped.load()) {
			if (!Paused.load()) {
				update();
			} else {
				Step.wait(false);
				Step.store(false);
				update();
			}
		}
	}
	void Gameboy_TKPWrapper::reset() {
		bus_.SoftReset();
		cpu_.Reset(SkipBoot);
		timer_.Reset();
		ppu_.Reset();
	}
	void Gameboy_TKPWrapper::update() {
		while (MessageQueue->PollRequests()) [[unlikely]] {
			auto request = MessageQueue->PopRequest();
			poll_request(request);
		}
		update_audio_sync();
		v_log();
	}
	void Gameboy_TKPWrapper::update_audio_sync() {
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
			CALLGRIND_STOP_INSTRUMENTATION;
		} else {
			// std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	void Gameboy_TKPWrapper::HandleKeyDown(uint32_t key) {
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
	void Gameboy_TKPWrapper::HandleKeyUp(uint32_t key) {
		if (auto it_dir = std::find(direction_keys_.begin(), direction_keys_.end(), key); it_dir != direction_keys_.end()) {
			int index = it_dir - direction_keys_.begin();
			bus_.DirectionKeys |= (1UL << index);
		}
		if (auto it_dir = std::find(action_keys_.begin(), action_keys_.end(), key); it_dir != action_keys_.end()) {
			int index = it_dir - action_keys_.begin();
			bus_.ActionKeys |= (1UL << index);
		}
	}
	bool Gameboy_TKPWrapper::load_file(std::string path) {
		auto loaded = bus_.LoadCartridge(std::forward<std::string>(path));
		ppu_.UseCGB = bus_.UseCGB;
		return loaded;
	}
	void* Gameboy_TKPWrapper::GetScreenData() {
		return ppu_.GetScreenData();
	}
	bool Gameboy_TKPWrapper::poll_uncommon_request(const Request& request) {
		return false;
	}
	void Gameboy_TKPWrapper::v_log() {
		if (logging_) {
			*log_file_ptr_ << std::hex << (int)bus_.Read(cpu_.PC) << " " << (int)bus_.Read(cpu_. PC + 1);
			if (log_flags_.test(0)) {
				*log_file_ptr_ << "PC: " << std::hex << cpu_.PC << " A:" << (uint16_t)cpu_.A << " F:" << (uint16_t)cpu_.F << " B:" << (uint16_t)cpu_.B
					<< " C:" << (uint16_t)cpu_.C << " D:" << (uint16_t)cpu_.D << " H:" << (uint16_t)cpu_.H << " L:" << (uint16_t)cpu_.L << " ";
			}

			*log_file_ptr_ << "\n";
		}
	}
}
