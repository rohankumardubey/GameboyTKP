#include "../gameboy.h"
#include <filesystem>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

namespace TKPEmu::Gameboy::QA {

    class TestMooneye : public CppUnit::TestFixture {
        void testMooneye(std::filesystem::path path);
        void testAllMooneye();
        CPPUNIT_TEST_SUITE(TestMooneye);
        CPPUNIT_TEST(testAllMooneye);
        CPPUNIT_TEST_SUITE_END();
    };
    void TestMooneye::testAllMooneye() {
        CPPUNIT_ASSERT_MESSAGE("Failed lol", false);
    }
    void TestMooneye::testMooneye(std::filesystem::path path) {
        TKPEmu::Gameboy::Gameboy gb_;
        gb_.LoadFromFile(path.string());
        for (unsigned i = 0; i < 4'000'000; i++) {
            gb_.update();
            if (gb_.cpu_.GetLastInstr() == 0x40) {
                CPPUNIT_ASSERT_EQUAL(uint8_t(3), gb_.cpu_.B);
                CPPUNIT_ASSERT_EQUAL(uint8_t(5), gb_.cpu_.C);
                CPPUNIT_ASSERT_EQUAL(uint8_t(8), gb_.cpu_.D);
                CPPUNIT_ASSERT_EQUAL(uint8_t(13), gb_.cpu_.E);
                CPPUNIT_ASSERT_EQUAL(uint8_t(21), gb_.cpu_.H);
                CPPUNIT_ASSERT_EQUAL(uint8_t(34), gb_.cpu_.L);
                return;
            }
        }
        CPPUNIT_ASSERT_MESSAGE("Exceeded 4 million instructions", false);
    }
    CPPUNIT_TEST_SUITE_REGISTRATION(TestMooneye);
}