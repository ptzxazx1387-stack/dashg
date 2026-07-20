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

    while (true) {
        HWND gameHwnd = FindWindowA("UnityWndClass", "Rust");
        if (gameHwnd) Overlay::UpdatePosition(gameHwnd);
        else break;

        Overlay::PollMessages();

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // --- تست نهایی: اشاره‌گر مستقیم EntList از دامپر ---
        uintptr_t entListPtr = memory::read<uintptr_t>(memory::game_assembly_base + 0x3c830a0);

        char buf[256];
        sprintf_s(buf, "Base: 0x%llX", memory::game_assembly_base);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 30), IM_COL32(255,255,255,255), buf);

        sprintf_s(buf, "EntListPtr: 0x%llX", entListPtr);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 50), IM_COL32(0,255,0,255), buf);

        sprintf_s(buf, "Test OK");
        ImGui::GetForegroundDrawList()->AddText(ImVec2(10, 70), IM_COL32(255,0,0,255), buf);
        // --- پایان تست ---

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

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    return 0;
}
