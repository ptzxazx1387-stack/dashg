#pragma once
#include <Windows.h>
#include <cstdint>
#include <vector>
#include <string>

namespace memory {
    inline HANDLE process_handle = nullptr;
    inline DWORD process_id = 0;
    inline uintptr_t game_assembly_base = 0;

    bool attach(const wchar_t* process_name);
    void detach();
    uintptr_t get_module_base(const wchar_t* module_name);

    template<typename T>
    T read(uintptr_t address) {
        T buffer{};
        ReadProcessMemory(process_handle, (LPCVOID)address, &buffer, sizeof(T), nullptr);
        return buffer;
    }

    template<typename T>
    void write(uintptr_t address, const T& value) {
        WriteProcessMemory(process_handle, (LPVOID)address, &value, sizeof(T), nullptr);
    }

    bool read_raw(uintptr_t address, void* buffer, size_t size);
    bool write_raw(uintptr_t address, const void* buffer, size_t size);

    template<typename T>
    T read_chain(uintptr_t base, const std::vector<uintptr_t>& offsets) {
        uintptr_t cur = base;
        for (size_t i = 0; i < offsets.size(); ++i) {
            if (i == offsets.size() - 1)
                return read<T>(cur + offsets[i]);
            cur = read<uintptr_t>(cur + offsets[i]);
            if (!cur || cur == UINTPTR_MAX) return T{};
        }
        return T{};
    }

    uintptr_t resolve_gchandle(uint32_t handle);
}
