#include "../gb_tkpwrapper.h"
#include <filesystem>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "../../lib/threadpool.hxx"

using TestResult = std::pair<bool, std::string>; // passed, path
namespace {
    void testMooneye(std::string path, TestResult* result) {
        TKPEmu::Gameboy::Gameboy gb_;
        CPPUNIT_ASSERT_MESSAGE("Could not load file: " + path, gb_.LoadFromFile(path));
        gb_.SkipBoot = true;
        gb_.Reset();
        gb_.FastMode = true;
        auto& cpu = gb_.GetCPU();
        #define must(a, b) if (a != b) { result->first = false; return; }
        for (unsigned i = 0; i < 4'000'000; i++) {
            gb_.Update();
            if (cpu.GetLastInstr() == 0x40) {
                must(uint8_t(0x03), cpu.B);
                must(uint8_t(0x05), cpu.C);
                must(uint8_t(0x08), cpu.D);
                must(uint8_t(0x0D), cpu.E);
                must(uint8_t(0x15), cpu.H);
                must(uint8_t(0x22), cpu.L);
                result->first = true;
                return;
            }
        }
        #undef must
        result->first = false;
        CPPUNIT_ASSERT_MESSAGE("4 million instructions exceeded", false);
    }
    void testBlargg(std::string path, TestResult* result) {
        TKPEmu::Gameboy::Gameboy gb_;
        CPPUNIT_ASSERT_MESSAGE("Could not load file: " + path, gb_.LoadFromFile(path));
        gb_.SkipBoot = true;
        gb_.Reset();
        gb_.FastMode = true;
        auto& cpu = gb_.GetCPU();


        result->first = false;
        CPPUNIT_ASSERT_MESSAGE("10 million instructions exceeded", false);
    }
}

namespace TKPEmu::Gameboy::QA {
    class TestGameboy : public CppUnit::TestFixture {
        void testAllMooneye();
        CPPUNIT_TEST_SUITE(TestGameboy);
        CPPUNIT_TEST(testAllMooneye);
        CPPUNIT_TEST_SUITE_END();
        std::string gameboy_tests_path_ = std::filesystem::current_path().string() + "/../GameboyTKP/tests/";
        std::vector<TestResult> mooneye_results_;
    };
    void TestGameboy::testAllMooneye() {
        using rdi = std::filesystem::recursive_directory_iterator;
        mooneye_results_.clear();
        std::vector<std::function<void()>> gb_jobs;
        unsigned count = 0;
        for (const auto& entry : rdi(gameboy_tests_path_ + "mooneye/"))
            if (entry.is_regular_file() && entry.path().extension() == ".gb")
                ++count;
        mooneye_results_.resize(count);
        unsigned i = 0;
        for (const auto& entry : rdi(gameboy_tests_path_ + "mooneye/")) {
            if (entry.is_regular_file() && entry.path().extension() == ".gb") {
                std::string name = entry.path().parent_path().filename().string() + "/" + entry.path().filename().string();
                mooneye_results_[i] = {false, name};
                std::function<void()> job = std::bind(testMooneye, entry.path().string(), &mooneye_results_[i]);
                gb_jobs.push_back(job);
                ++i;
            }
        }
        TKPEmu::Tools::FixedTaskThreadPool fttp(gb_jobs);
        fttp.StartAllAndWait();
        std::sort(mooneye_results_.begin(), mooneye_results_.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.second < rhs.second;
        });
        int fail_count = 0;
        std::ofstream ofs("result.md");
        ofs << "## [Gekkio](https://github.com/Gekkio)'s tests:\n\n";
        ofs << "| Test | GameboyTKP |\n";
        ofs << "| -- | -- |\n";
        for (auto& r : mooneye_results_) {
            fail_count += !r.first;
            ofs << "| " << r.second << " | " << (r.first ? ":+1:" : ":x:") << " |\n";
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Not all tests passed", count, count - fail_count);
    }
    CPPUNIT_TEST_SUITE_REGISTRATION(TestGameboy);
}