#include "gb_apu.h"
#include "gb_addresses.h"
constexpr int SAMPLE_RATE = 44100;
constexpr int AMPLITUDE = 28000;

namespace TKPEmu::Gameboy::Devices {
    void audio_callback(void *user_data, Uint8 *raw_buffer, int bytes)
    {
        Sint16 *buffer = (Sint16*)raw_buffer;
        int length = bytes / 2; // 2 bytes per sample for AUDIO_S16SYS
        int &sample_nr(*(int*)user_data);

        for(int i = 0; i < length; i++, sample_nr++)
        {
            double time = (double)sample_nr / (double)SAMPLE_RATE;
            buffer[i] = (Sint16)(AMPLITUDE * sin(2.0f * M_PI * 441.0f * time)); // render 441 HZ sine wave
        }
    }
    APU::APU(ChannelArrayPtr channel_array_ptr) 
            : channel_array_ptr_(channel_array_ptr) {
        SDL_AudioSpec want;
        int sample_nr = 0;
        SDL_zero(want);
        want.freq = SAMPLE_RATE; // number of samples per second
        want.format = AUDIO_S16SYS; // sample type (here: signed short i.e. 16 bit)
        want.channels = 1; // only one channel
        want.samples = 2048; // buffer-size
        want.callback = audio_callback; // function SDL calls periodically to refill the buffer
        want.userdata = &sample_nr; // counter, keeping track of current sample number
         SDL_AudioSpec have;
        if (SDL_OpenAudio(&want, &have) != 0) {
            SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
        }
        if (want.format != have.format) {
            SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");
        }
    }
    void APU::Update() {
        
    }
}