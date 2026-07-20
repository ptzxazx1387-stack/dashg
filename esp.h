#pragma once
#include "sdk/classes.h"
#include "sdk/math.h"
#include "imgui/imgui.h"
#include <vector>

namespace ESP {
    inline void Render(const std::vector<PlayerData>& players, Matrix viewMatrix, int width, int height, float fps) {
        char buf[128];

        sprintf_s(buf, "Players: %d", (int)players.size());
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 30), IM_COL32(0, 255, 0, 255), buf);

        sprintf_s(buf, "by .presidental - FPS: %.0f", fps);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 50), IM_COL32(255, 255, 255, 255), buf);

        for (const auto& player : players) {
            if (!player.isValid || player.position.empty()) continue;

            ImU32 col = player.isSleeping ? IM_COL32(150, 150, 150, 255) : IM_COL32(255, 255, 255, 255);

            Vector2 screenPos;
            if (WorldToScreen(player.position, screenPos, viewMatrix, width, height)) {
                std::string label = player.name;
                if (player.isSleeping) label += " [sleeping]";

                ImGui::GetForegroundDrawList()->AddText(
                    ImVec2(screenPos.x, screenPos.y - 15.0f),
                    col,
                    label.c_str()
                );
            }
        }
    }
}
