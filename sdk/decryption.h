#pragma once
#include "../memory/memory.h"

namespace decryption {
    inline uintptr_t base_networkable_0(uintptr_t wrapper) {
        uint64_t rax = memory::read<uint64_t>(wrapper + 0x18);
        return memory::resolve_gchandle((uint32_t)rax);
    }

    inline uintptr_t base_networkable_1(uintptr_t parent) {
        uint64_t rax = memory::read<uint64_t>(parent + 0x18);
        return memory::resolve_gchandle((uint32_t)rax);
    }
}
