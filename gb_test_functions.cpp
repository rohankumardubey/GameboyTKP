#include "gb_test_functions.h"

namespace TKPEmu::Gameboy {
    std::string QA::TestError = "";
    bool QA::TestMooneye(std::filesystem::path path) {
        Gameboy::Gameboy gb_;
        gb_.LoadFromFile(path.string());
        TestError = "exceeded 4,000,000 instructions";
        for (unsigned i = 0; i < 4'000'000; i++) {
            gb_.update();
            if (gb_.cpu_.last_instr_ == 0x40) {
                // Mooneye tests stop when a 0x40 (LD B,B) is run
                if (
                    gb_.cpu_.B == 3  &&
                    gb_.cpu_.C == 5  &&
                    gb_.cpu_.D == 8  &&
                    gb_.cpu_.E == 13 &&
                    gb_.cpu_.H == 21 &&
                    gb_.cpu_.L == 34
                )
                {
                    // A successful test writes these fibonacci numbers to the registers
                    return true;
                }
                TestError = "registers are not equal to the magic numbers";
                break;
            }
        }
        TestError = "Failed Mooneye test: " + path.stem().string() + " - " + TestError;
        return false;
    }
}