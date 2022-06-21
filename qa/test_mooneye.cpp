#include "../gb_tkpwrapper.h"
#include <filesystem>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "../../lib/threadpool.hxx"

namespace {
    void testMooneye(std::string path) {
        TKPEmu::Gameboy::Gameboy gb_;
        CPPUNIT_ASSERT_MESSAGE("Could not load file: " + path, gb_.LoadFromFile(path));
        gb_.SkipBoot = true;
        gb_.Reset();
        gb_.FastMode = true;
        auto& cpu = gb_.GetCPU();
        #define caem(a, b) CPPUNIT_ASSERT_EQUAL_MESSAGE("Failed, " #b ": " + path, a, b)
        for (unsigned i = 0; i < 4'000'000; i++) {
            gb_.Update();
            if (cpu.GetLastInstr() == 0x40) {
                caem(uint8_t(0x03), cpu.B);
                caem(uint8_t(0x05), cpu.C);
                caem(uint8_t(0x08), cpu.D);
                caem(uint8_t(0x0D), cpu.E);
                caem(uint8_t(0x15), cpu.H);
                caem(uint8_t(0x22), cpu.L);
                return;
            }
        }
        #undef caem
        CPPUNIT_ASSERT_MESSAGE("Exceeded 4 million instructions", false);
    }
}

namespace TKPEmu::Gameboy::QA {
    class TestMooneye : public CppUnit::TestFixture {
        void testAllMooneye();
        CPPUNIT_TEST_SUITE(TestMooneye);
        CPPUNIT_TEST(testAllMooneye);
        CPPUNIT_TEST_SUITE_END();
        std::string gameboy_tests_path_ = std::filesystem::current_path().string() + "/../GameboyTKP/tests/";
    };
    void TestMooneye::testAllMooneye() {
        using rdi = std::filesystem::recursive_directory_iterator;
        std::vector<std::function<void()>> gb_jobs;
        for (const auto& entry : rdi(gameboy_tests_path_ + "mooneye/")) {
            if (entry.is_regular_file() && entry.path().extension() == ".gb") {
                std::function<void()> job = std::bind(testMooneye, entry.path().string());
                gb_jobs.push_back(job);
            }
        }
        TKPEmu::Tools::FixedTaskThreadPool fttp(gb_jobs);
        fttp.StartAllAndWait();
    }
    CPPUNIT_TEST_SUITE_REGISTRATION(TestMooneye);
}