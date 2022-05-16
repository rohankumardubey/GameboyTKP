cmake_minimum_required(VERSION 3.19)
project(GameboyTKP)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/expected_results.csv ~/.config/tkpemu/expected_results.csv COPYONLY)
set(FILES gameboy.cpp gb_apu_ch.cpp gb_apu.cpp
    gb_bus.cpp gb_cartridge.cpp gb_cpu.cpp
    gb_disassembler.cpp gb_ppu.cpp gb_romdata.cpp
    gb_timer.cpp gb_tracelogger.cpp)
add_library(GameboyTKP ${FILES})