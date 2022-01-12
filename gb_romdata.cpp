#include "gb_romdata.h"
namespace TKPEmu::Applications {
    GameboyRomData::GameboyRomData(std::string menu_title, std::string window_title) 
        : IMApplication(menu_title, window_title) 
    {
        min_size = ImVec2(400, 400);
        max_size = ImVec2(500, 400);
        image_data_.resize(256 * 128 * 4);
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, texture_);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			256,
		    128,
			0,
			GL_RGBA,
			GL_FLOAT,
			NULL
		);
		glBindTexture(GL_TEXTURE_2D, 0);
    }
    GameboyRomData::~GameboyRomData() {
        glDeleteTextures(1, &texture_);
    }
    void GameboyRomData::v_draw() {
		//std::lock_guard<std::mutex> lg(emulator_->DebugUpdateMutex);
        if (ImGui::BeginTabBar("RomDataTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Info")) {
                draw_info();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Tilesets")) {
                draw_tilesets();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("OAM")) {

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Draw options")) {
                Gameboy* gb_ptr = static_cast<Gameboy*>(emulator_);
                ImGui::Checkbox("Red tint for sprites", gb_ptr->DebugSpriteTint());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    void GameboyRomData::draw_info() {
        Gameboy* gb_ptr = static_cast<Gameboy*>(emulator_);
        ImGui::Text("Filename: "); ImGui::SameLine(); ImGui::Text(gb_ptr->CurrentFilename.c_str());
        ImGui::Text(gb_ptr->GetCartridge()->GetHeaderText());
        ImGui::InputText("Rom hash", gb_ptr->RomHash.data(), gb_ptr->RomHash.length(), ImGuiInputTextFlags_ReadOnly);
        static bool hashed = false;
        static std::string hash = "?";
        static std::string result = "?";
        if (gb_ptr->Paused) {
            hashed = true;
            hash = gb_ptr->GetScreenshotHash();
            result = "{ \"" + gb_ptr->RomHash + "\", { " + std::to_string(gb_ptr->GetCPU().TotalClocks) + ", \"" + hash + "\" } },";
        } else if (hashed && !gb_ptr->Paused) {
            hashed = false;
            hash = "Pause to get current screenshot hash";
        }
        ImGui::InputText("VRAM hash", hash.data(), hash.length(), ImGuiInputTextFlags_ReadOnly);
        if (hashed) {
            ImGui::InputText("emulator_results.h hash", result.data(), result.length(), ImGuiInputTextFlags_ReadOnly);
        }
    }
    void GameboyRomData::draw_tilesets() {
        ImGui::Text("0x8000:             0x8800:");
        ImGui::Image((void*)(intptr_t)texture_, ImVec2(256, 128));
        if (!texture_cached_) {
            update_tilesets();
            texture_cached_ = true;
            clock_a = std::chrono::high_resolution_clock::now();
        } else {
            if (!emulator_->Paused) {
                std::chrono::high_resolution_clock::time_point clock_b = std::chrono::high_resolution_clock::now();
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(clock_b - clock_a).count();
                if (seconds >= 1) {
                    texture_cached_ = false;
                }
            }
        }
    }
    void GameboyRomData::update_tilesets() {
        Gameboy* gb = static_cast<Gameboy*>(emulator_);
        gb->GetPPU().FillTileset(image_data_.data());
        gb->GetPPU().FillTileset(image_data_.data(), 128, 0, 0x8800);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            256,
            128,
            GL_RGBA,
            GL_FLOAT,
            image_data_.data()
        );
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}