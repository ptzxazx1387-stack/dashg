#pragma once
#include "../memory/memory.h"

// در این نسخه نیازی به رمزگشایی نیست، چون مستقیماً از EntList RVA می‌خوانیم.
// اما اگر برای بخش‌های دیگر لازم شد، توابع خالی باقی موندن.
namespace decryption {
    inline uintptr_t base_networkable_0(uintptr_t wrapper) {
        // اینجا دیگه استفاده نمی‌شه
        return 0;
    }
}
