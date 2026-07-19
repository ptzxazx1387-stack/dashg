#pragma once
#include "../memory/memory.h"

namespace decryption {
    inline uintptr_t base_networkable_0(uintptr_t wrapper) {
        uint64_t rax = memory::read<uint64_t>(wrapper + 0x18);
        uint32_t* d = (uint32_t*)&rax;
        for (int i = 0; i < 2; ++i) {
            uint32_t v = d[i];
            v = (v << 16) | (v >> 16);
            v ^= 0xFE89EFE3u;
            v -= 0x7C71A258u;
            d[i] = v;
        }
        return memory::resolve_gchandle((uint32_t)rax);
    }

    inline uintptr_t base_networkable_1(uintptr_t parent) {
        uint64_t rax = memory::read<uint64_t>(parent + 0x18);
        uint32_t* d = (uint32_t*)&rax;
        for (int i = 0; i < 2; ++i) {
            uint32_t v = d[i];
            v += 0xEBB43A5Au;
            v = (v << 23) | (v >> 9);
            v += 0x4A9A11E7u;
            v = (v << 28) | (v >> 4);
            d[i] = v;
        }
        return memory::resolve_gchandle((uint32_t)rax);
    }
}
