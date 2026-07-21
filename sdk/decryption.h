// ========================================================================
// UPDATED sdk/decryption.h - با دیکریپت‌های کامل از کاربر ۴ (AFTAB)
// ========================================================================
#pragma once
#include "../memory/memory.h"

namespace decryption {
    // base_networkable_0 (client_entities) - 3 ops
    inline uintptr_t base_networkable_0(uintptr_t wrapper) {
        // First read the encrypted value from wrapper+0x18
        uint64_t encrypted = memory::read<uint64_t>(wrapper + 0x18);
        // Apply decryption in-place on the 64-bit value (two 32-bit halves)
        uint32_t* parts = (uint32_t*)&encrypted;
        for (int i = 0; i < 2; ++i) {
            uint32_t v = parts[i];
            v = (v << 16) | (v >> 16);          // rol16
            v ^= 0xFE89EFE3u;
            v -= 0x7C71A258u;
            parts[i] = v;
        }
        // Resolve GCHandle
        return memory::resolve_gchandle((uint32_t)encrypted);
    }

    // base_networkable_1 (entity_list dictionary) - 4 ops
    inline uintptr_t base_networkable_1(uintptr_t parent) {
        uint64_t encrypted = memory::read<uint64_t>(parent + 0x18);
        uint32_t* parts = (uint32_t*)&encrypted;
        for (int i = 0; i < 2; ++i) {
            uint32_t v = parts[i];
            v += 0xEBB43A5Au;
            v = (v << 23) | (v >> 9);           // rol23
            v += 0x4A9A11E7u;
            v = (v << 28) | (v >> 4);           // rol28
            parts[i] = v;
        }
        return memory::resolve_gchandle((uint32_t)encrypted);
    }

    // (Optional) local_player decryption if needed – not used in this simple version.
    // But we keep it for completeness.
    inline uintptr_t local_player(uintptr_t encrypted) {
        uint64_t val = encrypted;
        uint32_t* parts = (uint32_t*)&val;
        for (int i = 0; i < 2; ++i) {
            uint32_t v = parts[i];
            v ^= 0x2B64236Cu;
            v += 0x049DE2A2u;
            v = (v << 19) | (v >> 13);          // rol19
            v ^= 0x721A9186u;
            parts[i] = v;
        }
        return val;
    }
}
