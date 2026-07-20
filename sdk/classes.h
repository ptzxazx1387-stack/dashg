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

namespace EntityList {
    inline uintptr_t get_client_entities() {
        return memory::read<uintptr_t>(memory::game_assembly_base + 0x3c830a0);
    }
    
    inline uintptr_t get_list_dict(uintptr_t client_entities) {
        return client_entities; // اگه مستقیم اشاره‌گر به لیست بود، نه دیکشنری
    }
    
    inline uintptr_t get_entity_array(uintptr_t list_ptr) {
        uintptr_t buffer_list = memory::read<uintptr_t>(list_ptr + 0x10);
        return memory::read<uintptr_t>(buffer_list + 0x10);
    }
    
    inline int get_entity_count(uintptr_t list_ptr) {
        uintptr_t buffer_list = memory::read<uintptr_t>(list_ptr + 0x10);
        return memory::read<int>(buffer_list + 0x18);
    }

    inline uintptr_t get_entity(uintptr_t array, int i) {
        return memory::read<uintptr_t>(array + 0x20 + (i * 8));
    }
}

class Camera {
public:
    static uintptr_t GetMainCamera() {
        uintptr_t game_asm = memory::game_assembly_base;
        // MainCamera typeinfo = 0xfd0a5c0 (از دامپ شما)
        uintptr_t static_fields = memory::read<uintptr_t>(game_asm + 0xfd0a5c0 + 0xB8);
        uintptr_t cam_obj_handle = memory::read<uintptr_t>(static_fields + 0x28); // instance = 0x28
        uintptr_t cam_native = memory::read<uintptr_t>(cam_obj_handle + 0x10);   // buffer = 0x10
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
        // playerModel = 0x500 طبق دامپ جدید
        return memory::read<uintptr_t>(address + 0x500);
    }

    Vector3 GetPosition() {
        uintptr_t model = GetPlayerModel();
        if (!model) return {};
        // PlayerModel::position = 0x2f8 (طبق دامپ جدید)
        return memory::read<Vector3>(model + 0x2F8);
    }

    std::string GetName() {
        // displayName = 0x6e0 (طبق دامپ جدید)
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
        // playerFlags = 0x6B8 (تغییر نکرده)
        uint32_t flags = memory::read<uint32_t>(address + 0x6B8);
        return (flags & 0x10) != 0;
    }

    uint64_t GetTeamID() {
        // currentTeam = 0x538 (تغییر نکرده)
        return memory::read<uint64_t>(address + 0x538);
    }

    uintptr_t GetBoneTransforms() {
        uintptr_t model = GetPlayerModel();
        if (!model) return 0;
        return memory::read<uintptr_t>(model + 0x98); // boneTransforms (در صورت نیاز)
    }

    Vector3 GetBonePosition(int bone_id) {
        uintptr_t transforms = GetBoneTransforms();
        if (!transforms) return {};
        uintptr_t entry = memory::read<uintptr_t>(transforms + 0x20 + (bone_id * 0x8));
        if (!entry) return {};
        return memory::read<Vector3>(entry + 0x18);
    }
};

namespace LocalPlayer {
    inline uintptr_t Get() {
        uintptr_t game_asm = memory::game_assembly_base;
        // این آفست باید بر اساس دامپ جدید باشد، فعلاً از مقدار قدیمی استفاده می‌کنیم
        // اگر local_player typeinfo موجود باشد می‌توانیم جایگزین کنیم
        uintptr_t static_fields = memory::read<uintptr_t>(game_asm + 0xFBD6028 + 0xB8);
        uintptr_t entity_handle = memory::read<uintptr_t>(static_fields + 0x20);
        return memory::read<uintptr_t>(entity_handle + 0x10);
    }
}
