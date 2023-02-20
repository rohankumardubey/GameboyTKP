#include <GameboyTKP/gb_wiiwrapper.h>

GB_WiiWrapper::GB_WiiWrapper() :
    channel_array_ptr_(nullptr),
    bus_(channel_array_ptr_),
    apu_(channel_array_ptr_, bus_.GetReference(addr_NR52)),
    ppu_(bus_),
    timer_(channel_array_ptr_, bus_),
    cpu_(bus_, ppu_, apu_, timer_),
    joypad_(bus_.GetReference(addr_joy)),
    interrupt_flag_(bus_.GetReference(addr_if))
{}

void GB_WiiWrapper::Update() {
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
}

void GB_WiiWrapper::LoadCartridge(void* data) {
    bus_.LoadCartridge((uint8_t*)data);
}

void* GB_WiiWrapper::GetScreenData() {
    return ppu_.GetScreenData();
}