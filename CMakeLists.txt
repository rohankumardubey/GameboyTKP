cmake_minimum_required(VERSION 3.19)
project(GameboyTKP)
set(CORE_FILES gb_tkpwrapper.cpp gb_apu_ch.cpp gb_apu.cpp
    gb_bus.cpp gb_cartridge.cpp gb_cpu.cpp gb_ppu.cpp gb_timer.cpp)
add_library(GameboyTKP ${CORE_FILES})
target_include_directories(GameboyTKP PUBLIC ../)
