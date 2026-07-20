#pragma once
#include "decryption.h"
#include "../memory/memory.h"
#include "math.h"
#include <string>
#include <vector>

struct PlayerData {
    uintptr_t address;
    Vector3 position;
    std::string name;
    bool isSleeping;
    bool isValid;
};

class Camera {
public:
    static uintptr_t GetMainCamera() {
        uintptr_t game_asm = memory::game_assembly_base;
        // MainCamera typeinfo = 0xfd0a5c0 (از دامپ)
        uintptr_t static_fields = memory::read<uintptr_t>(game_asm + 0xfd0a5c0 + 0xB8);
        uintptr_t cam_obj_handle = memory::read<uintptr_t>(static_fields + 0x28);
        uintptr_t cam_native = memory::read<uintptr_t>(cam_obj_handle + 0x10);
        return cam_native;
    }

    static Matrix GetViewMatrix(uintptr_t camera) {
        return memory::read<Matrix>(camera + 0x2FC);
    }
};

class BasePlayer {
public:
    uintptr_t address;
    BasePlayer(uintptr_t addr) : address(addr) {}

    uintptr_t GetPlayerModel() {
        return memory::read<uintptr_t>(address + 0x500);
    }

    Vector3 GetPosition() {
        uintptr_t model = GetPlayerModel();
        if (!model) return {};
        return memory::read<Vector3>(model + 0x2F8);
    }

    std::string GetName() {
        uintptr_t name_obj = memory::read<uintptr_t>(address + 0x6E0);
        if (!name_obj) return "";
        int length = memory::read<int>(name_obj + 0x10);
        if (length <= 0 || length > 64) return "";
        std::wstring wname(length, L'\0');
        memory::read_raw(name_obj + 0x14, (void*)wname.data(), length * sizeof(wchar_t));
        std::string name;
        for (wchar_t c : wname) name += (c < 128) ? (char)c : '?';
        return name;
    }

    bool IsSleeping() {
        uint32_t flags = memory::read<uint32_t>(address + 0x6B8);
        return (flags & 0x10) != 0;
    }

    uint64_t GetTeamID() {
        return memory::read<uint64_t>(address + 0x538);
    }
};
