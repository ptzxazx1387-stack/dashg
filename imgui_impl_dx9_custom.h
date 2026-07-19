#pragma once
#include <d3d9.h>

extern IDirect3DDevice9* g_pd3dDevice;

bool ImGui_ImplDX9_Init(IDirect3DDevice9* device);
void ImGui_ImplDX9_Shutdown();
void ImGui_ImplDX9_NewFrame();
void ImGui_ImplDX9_RenderDrawData(ImDrawData* draw_data);
bool ImGui_ImplDX9_CreateDeviceObjects();
void ImGui_ImplDX9_InvalidateDeviceObjects();
