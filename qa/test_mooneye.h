#pragma once
#ifndef TKP_GAMEBOY_QA_MOONEYE_H
#define TKP_GAMEBOY_QA_MOONEYE_H
#include <string>
#include <filesystem>
#include "../../cppunit/TestCase.h"
#include "../../cppunit/TestSuite.h"
#include "../../cppunit/TestCaller.h"
#include "../../cppunit/TestRunner.h"
#include "../gameboy.h"

namespace TKPEmu::Gameboy::QA {
    class MooneyeTestCase : public CppUnit::TestCase {
        MooneyeTestCase(std::string name) : TestCase(name) {}
        void testMooneye(std::filesystem::path path);
    };
}
#endif