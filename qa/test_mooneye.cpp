#include "../gameboy.h"
#include <filesystem>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

namespace TKPEmu::Gameboy::QA {

    class TestMooneye : public CppUnit::TestFixture {
        void testMooneye(std::string path);
        void testAllMooneye();
        CPPUNIT_TEST_SUITE(TestMooneye);
        CPPUNIT_TEST(testAllMooneye);
        CPPUNIT_TEST_SUITE_END();
        std::string gameboy_tests_path_ = std::filesystem::current_path().string() + "/../GameboyTKP/tests/";
    };
    void TestMooneye::testAllMooneye() {
        // try {
            testMooneye(gameboy_tests_path_ + "mooneye/acceptance/add_sp_e_timing.gb");
        // } catch (std::exception& e) {
        //     CPPUNIT_ASSERT_MESSAGE(e.what(), false);
        // }
    }
    void TestMooneye::testMooneye(std::string path) {
        TKPEmu::Gameboy::Gameboy gb_;
        // CPPUNIT_ASSERT_MESSAGE(path, false);
        gb_.LoadFromFile(path);
        gb_.FastMode = true;
        gb_.apu_.UseSound = false;
        gb_.Reset();
        for (unsigned i = 0; i < 4'000'000; i++) {
            gb_.update();
            if (gb_.cpu_.GetLastInstr() == 0x40) {
                CPPUNIT_ASSERT_EQUAL(uint8_t(0x03), gb_.cpu_.B);
                CPPUNIT_ASSERT_EQUAL(uint8_t(0x05), gb_.cpu_.C);
                CPPUNIT_ASSERT_EQUAL(uint8_t(0x08), gb_.cpu_.D);
                CPPUNIT_ASSERT_EQUAL(uint8_t(0x0D), gb_.cpu_.E);
                CPPUNIT_ASSERT_EQUAL(uint8_t(0x15), gb_.cpu_.H);
                CPPUNIT_ASSERT_EQUAL(uint8_t(0x22), gb_.cpu_.L);
                return;
            }
        }
        CPPUNIT_ASSERT_EQUAL(uint8_t(0x03), gb_.cpu_.B);
        CPPUNIT_ASSERT_EQUAL(uint8_t(0x05), gb_.cpu_.C);
        CPPUNIT_ASSERT_EQUAL(uint8_t(0x08), gb_.cpu_.D);
        CPPUNIT_ASSERT_EQUAL(uint8_t(0x0D), gb_.cpu_.E);
        CPPUNIT_ASSERT_EQUAL(uint8_t(0x15), gb_.cpu_.H);
        CPPUNIT_ASSERT_EQUAL(uint8_t(0x22), gb_.cpu_.L);
        CPPUNIT_ASSERT_MESSAGE("Exceeded 4 million instructions", false);
    }
    CPPUNIT_TEST_SUITE_REGISTRATION(TestMooneye);
}