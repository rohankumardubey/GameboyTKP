#pragma once
#ifndef TKP_GAMEBOY_TEST_FUNCS_H
#define TKP_GAMEBOY_TEST_FUNCS_H
#include <string>
#include <filesystem>
#include "gameboy.h"

namespace TKPEmu::Gameboy {
    struct QA {
        static std::string TestError;

        static bool TestMooneye(std::filesystem::path path);
    };
}
#endif
