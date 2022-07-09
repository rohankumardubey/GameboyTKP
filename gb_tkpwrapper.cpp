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
#include <include/emulator_user_data.hxx>
#include <include/emulator_factory.h>
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
		const EmulatorUserData& user_data = EmulatorFactory::GetEmulatorUserData()[static_cast<int>(EmuType::Gameboy)];
		const KeyMappings& mappings = EmulatorFactory::GetEmulatorData()[static_cast<int>(EmuType::Gameboy)].Mappings;
		for (int i = 0; i < 4; i++) {
			direction_keys_[i] = mappings.KeyValues[i];
			action_keys_[i] = mappings.KeyValues[4 + i];
			auto color = std::stoi(user_data.Get(std::string("dmg_c") + std::to_string(i)));
			bus_.Palette[i][0] = color & 0xFF;
			bus_.Palette[i][1] = (color >> 8) & 0xFF;
			bus_.Palette[i][2] = color >> 16;
		}
	}
	Gameboy::~Gameboy() {
		Stopped.store(true);
	}
	bool& Gameboy::IsReadyToDraw() {
		return ppu_.ReadyToDraw;
	}
	void Gameboy::start() {
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
				while (MessageQueue->PollRequests()) [[unlikely]] {
					auto request = MessageQueue->PopRequest();
					poll_request(request);
				}
				update();
			} else {
				Step.wait(false);
				Step.store(false);
				while (MessageQueue->PollRequests()) [[unlikely]] {
					auto request = MessageQueue->PopRequest();
					poll_request(request);
				}
				update();
			}
		}
	}
	void Gameboy::reset() {
		bus_.SoftReset();
		cpu_.Reset(SkipBoot);
		timer_.Reset();
		ppu_.Reset();
	}
	void Gameboy::update() {
		update_audio_sync();
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
			CALLGRIND_STOP_INSTRUMENTATION;
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	void Gameboy::HandleKeyDown(uint32_t key) {
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
	void Gameboy::HandleKeyUp(uint32_t key) {
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
	bool Gameboy::poll_uncommon_request(const Request& request) {
		return false;
	}
}
