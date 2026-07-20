#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <d3d9.h>
#include <dwmapi.h>
#include "memory/memory.h"
#include "sdk/classes.h"
#include "overlay.h"
#include "esp.h"
#include "imgui/imgui.h"
#include "imgui_impl_dx9_custom.h"
#include "imgui_impl_win32.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dwmapi.lib")

namespace Cache {
    std::vector<PlayerData> players;
    Matrix viewMatrix;
    float fps = 0;
    std::mutex mtx;
    bool running = true;

    // Debug
    inline uintptr_t debug_entListPtr = 0;
    inline uintptr_t debug_bufferList = 0;
    inline uintptr_t debug_array = 0;
    inline int debug_count = 0;
}

static LPDIRECT3D9              g_pD3D = NULL;
IDirect3DDevice9* g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D(HWND hWnd) {
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) return false;
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) return false;
    return true;
}

void slow_thread() {
    while (Cache::running) {
        if (!memory::game_assembly_base) {
            memory::game_assembly_base = memory::get_module_base(L"GameAssembly.dll");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        uintptr_t entListPtr = memory::read<uintptr_t>(memory::game_assembly_base + 0x3c830a0);
        uintptr_t bufferList = 0, entityArray = 0;
        int count = 0;

        if (entListPtr) {
            bufferList = memory::read<uintptr_t>(entListPtr + 0x10);
            if (bufferList) {
                entityArray = memory::read<uintptr_t>(bufferList + 0x10);
                count = memory::read<int>(bufferList + 0x18);
            }
        }

        {
            std::lock_guard<std::mutex> lock(Cache::mtx);
            Cache::debug_entListPtr = entListPtr;
            Cache::debug_bufferList = bufferList;
            Cache::debug_array = entityArray;
            Cache::debug_count = count;
        }

        // اگر همه چی اوکی بود، entityها رو بخون
        if (entityArray && count > 0) {
            std::vector<PlayerData> tempPlayers;
            for (int i = 0; i < count; i++) {
                uintptr_t entity = memory::read<uintptr_t>(entityArray + 0x20 + (i * 8));
                if (!entity) continue;

                BasePlayer bp(entity);
                PlayerData data;
                data.address = entity;
                data.name = bp.GetName();
                data.isSleeping = bp.IsSleeping();
                data.isValid = true;
                tempPlayers.push_back(data);
            }
            std::lock_guard<std::mutex> lock(Cache::mtx);
            Cache::players = std::move(tempPlayers);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void fast_thread() {
    while (Cache::running) {
        uintptr_t camera = Camera::GetMainCamera();
        Matrix vMatrix;
        for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) vMatrix.m[r][c] = 0.0f;
        if (camera) vMatrix = Camera::GetViewMatrix(camera);

        {
            std::lock_guard<std::mutex> lock(Cache::mtx);
            Cache::viewMatrix = vMatrix;
            for (auto& player : Cache::players) {
                BasePlayer bp(player.address);
                player.position = bp.GetPosition();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

int main() {
    if (!memory::attach()) {
        MessageBoxA(0, "Game not found", "Error", MB_OK);
        return 1;
    }
    memory::game_assembly_base = memory::get_module_base(L"GameAssembly.dll");

    if (!Overlay::Init()) {
        MessageBoxA(0, "failed to create the overlay", "pidor", MB_OKCANCEL);
    }

    if (!CreateDeviceD3D(Overlay::hwnd)) return 1;

    ImGui_ImplDX9_Init(g_pd3dDevice);
    ImGui_ImplWin32_Init(Overlay::hwnd);

    std::thread(slow_thread).detach();
    std::thread(fast_thread).detach();

    while (true) {
        HWND gameHwnd = FindWindowA("UnityWndClass", "Rust");
        if (gameHwnd) Overlay::UpdatePosition(gameHwnd);
        else break;

        Overlay::PollMessages();

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Debug info
        {
            char buf[256];
            sprintf_s(buf, "entListPtr: 0x%llX", Cache::debug_entListPtr);
            ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 110), IM_COL32(255,255,0,255), buf);
            sprintf_s(buf, "bufferList: 0x%llX", Cache::debug_bufferList);
            ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 130), IM_COL32(255,255,0,255), buf);
            sprintf_s(buf, "entityArray: 0x%llX, count: %d", Cache::debug_array, Cache::debug_count);
            ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 150), IM_COL32(255,255,0,255), buf);
        }

        {
            std::vector<PlayerData> localPlayers;
            Matrix localMatrix;
            float localFps;
            {
                std::lock_guard<std::mutex> lock(Cache::mtx);
                localPlayers = Cache::players;
                localMatrix = Cache::viewMatrix;
                localFps = ImGui::GetIO().Framerate;
            }

            RECT rect;
            GetClientRect(Overlay::hwnd, &rect);
            ESP::Render(localPlayers, localMatrix, rect.right - rect.left, rect.bottom - rect.top, localFps);
        }

        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

        if (g_pd3dDevice->BeginScene() >= 0) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        if (GetAsyncKeyState(VK_END) & 1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    Cache::running = false;
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    return 0;
}
