#include "imgui_impl_dx9_custom.h"
#include "imgui/imgui.h"
#include <d3d9.h>

// حذف: static LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
// از extern تعریف‌شده در هدر استفاده می‌کنیم.

static LPDIRECT3DVERTEXBUFFER9   g_pVB = NULL;
static LPDIRECT3DINDEXBUFFER9    g_pIB = NULL;
static LPDIRECT3DTEXTURE9        g_FontTexture = NULL;
static int                       g_VertexBufferSize = 5000, g_IndexBufferSize = 10000;

struct CUSTOMVERTEX {
    float    pos[3];
    D3DCOLOR col;
    float    uv[2];
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

static void ImGui_ImplDX9_SetupRenderState(ImDrawData* draw_data) {
    g_pd3dDevice->SetPixelShader(NULL);
    g_pd3dDevice->SetVertexShader(NULL);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    D3DVIEWPORT9 vp;
    vp.X = vp.Y = 0;
    vp.Width = (DWORD)draw_data->DisplaySize.x;
    vp.Height = (DWORD)draw_data->DisplaySize.y;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    g_pd3dDevice->SetViewport(&vp);

    float L = draw_data->DisplayPos.x + 0.5f;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x + 0.5f;
    float T = draw_data->DisplayPos.y + 0.5f;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y + 0.5f;
    D3DMATRIX mat_identity = { { { 1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f } } };
    D3DMATRIX mat_projection = { { { 2.0f/(R-L),   0.0f,         0.0f,  0.0f,  0.0f,  2.0f/(T-B),   0.0f,  0.0f,  0.0f,  0.0f,         0.5f,  0.0f,  (L+R)/(L-R),  (T+B)/(B-T),  0.5f,  1.0f } } };
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &mat_identity);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &mat_identity);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &mat_projection);
}

bool ImGui_ImplDX9_CreateFontsTexture() {
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height, bytes_per_pixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

    if (g_FontTexture)
        g_FontTexture->Release();
    g_FontTexture = NULL;

    if (g_pd3dDevice->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_FontTexture, NULL) != D3D_OK)
        return false;

    D3DLOCKED_RECT rect;
    if (g_FontTexture->LockRect(0, &rect, NULL, 0) == D3D_OK) {
        memcpy(rect.pBits, pixels, width * height * 4);
        g_FontTexture->UnlockRect(0);
    }
    io.Fonts->TexID = (ImTextureID)g_FontTexture;
    return true;
}

bool ImGui_ImplDX9_CreateDeviceObjects() {
    if (!g_pd3dDevice) return false;
    if (!ImGui_ImplDX9_CreateFontsTexture()) return false;
    return true;
}

void ImGui_ImplDX9_InvalidateDeviceObjects() {
    if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }
    if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
    if (g_FontTexture) { g_FontTexture->Release(); g_FontTexture = NULL; }
}

bool ImGui_ImplDX9_Init(IDirect3DDevice9* device) {
    g_pd3dDevice = device;  // اینجا دستگاه اصلی را می‌گیرد
    if (!g_pd3dDevice) return false;
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_dx9_custom";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    return true;
}

void ImGui_ImplDX9_Shutdown() {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    g_pd3dDevice = NULL;
}

void ImGui_ImplDX9_NewFrame() {
    if (!g_FontTexture)
        ImGui_ImplDX9_CreateDeviceObjects();
}

void ImGui_ImplDX9_RenderDrawData(ImDrawData* draw_data) {
    if (!draw_data || draw_data->CmdListsCount == 0) return;

    IDirect3DDevice9* ctx = g_pd3dDevice;

    if (g_pVB == NULL || g_VertexBufferSize < draw_data->TotalVtxCount) {
        if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }
        g_VertexBufferSize = draw_data->TotalVtxCount + 5000;
        if (ctx->CreateVertexBuffer(g_VertexBufferSize * sizeof(CUSTOMVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &g_pVB, NULL) != D3D_OK)
            return;
    }
    if (g_pIB == NULL || g_IndexBufferSize < draw_data->TotalIdxCount) {
        if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
        g_IndexBufferSize = draw_data->TotalIdxCount + 10000;
        if (ctx->CreateIndexBuffer(g_IndexBufferSize * sizeof(ImDrawIdx), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, D3DPOOL_DEFAULT, &g_pIB, NULL) != D3D_OK)
            return;
    }

    CUSTOMVERTEX* vtx_dst;
    ImDrawIdx* idx_dst;
    if (g_pVB->Lock(0, (UINT)(draw_data->TotalVtxCount * sizeof(CUSTOMVERTEX)), (void**)&vtx_dst, D3DLOCK_DISCARD) != D3D_OK) return;
    if (g_pIB->Lock(0, (UINT)(draw_data->TotalIdxCount * sizeof(ImDrawIdx)), (void**)&idx_dst, D3DLOCK_DISCARD) != D3D_OK) { g_pVB->Unlock(); return; }

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
        for (int i = 0; i < cmd_list->VtxBuffer.Size; i++) {
            vtx_dst->pos[0] = vtx_src->pos.x;
            vtx_dst->pos[1] = vtx_src->pos.y;
            vtx_dst->pos[2] = 0.0f;
            vtx_dst->col = (vtx_src->col & 0xFF00FF00) | ((vtx_src->col & 0xFF0000) >> 16) | ((vtx_src->col & 0xFF) << 16);
            vtx_dst->uv[0] = vtx_src->uv.x;
            vtx_dst->uv[1] = vtx_src->uv.y;
            vtx_dst++;
            vtx_src++;
        }
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    g_pVB->Unlock();
    g_pIB->Unlock();

    ctx->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
    ctx->SetIndices(g_pIB);
    ctx->SetFVF(D3DFVF_CUSTOMVERTEX);

    ImGui_ImplDX9_SetupRenderState(draw_data);

    int vtx_offset = 0;
    int idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
                ctx->SetScissorRect(&r);
                ctx->SetTexture(0, (IDirect3DTexture9*)pcmd->TextureId);
                ctx->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, vtx_offset + pcmd->VtxOffset, (UINT)cmd_list->VtxBuffer.Size, idx_offset + pcmd->IdxOffset, pcmd->ElemCount / 3);
            }
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
        idx_offset += cmd_list->IdxBuffer.Size;
    }
}
