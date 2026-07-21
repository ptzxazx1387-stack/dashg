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

        uintptr_t client_entities = EntityList::get_client_entities();
        if (!client_entities) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        uintptr_t list_dict = EntityList::get_list_dict(client_entities);
        if (!list_dict) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        uintptr_t entity_array = EntityList::get_entity_array(list_dict);
        int count = EntityList::get_entity_count(list_dict);

        std::vector<PlayerData> tempPlayers;
        for (int i = 0; i < count; i++) {
            uintptr_t entity = EntityList::get_entity(entity_array, i);
            if (!entity) continue;

            BasePlayer bp(entity);
            PlayerData data;
            data.address = entity;
            data.name = bp.GetName();
            data.isSleeping = bp.IsSleeping();
            data.isValid = true;
            tempPlayers.push_back(data);
        }

        {
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
        for(int r=0; r<4; r++) for(int c=0; c<4; c++) vMatrix.m[r][c] = 0.0f;
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
