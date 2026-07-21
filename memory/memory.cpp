// ========================================================================
// memory.cpp (بهبود get_module_base برای دقت بیشتر)
// ========================================================================
#include "memory.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <string.h>

bool memory::attach() {
    const wchar_t* names[] = { L"RustClient.exe", L"Rust.exe", L"RustClient" };
    for (auto name : names) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) continue;
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, name) == 0) {
                    process_id = pe.th32ProcessID;
                    CloseHandle(snap);
                    process_handle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, process_id);
                    if (process_handle) {
                        // Try to get GameAssembly base immediately
                        game_assembly_base = get_module_base(L"GameAssembly.dll");
                        return true;
                    }
                    return false;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
    return false;
}

void memory::detach() {
    if (process_handle) {
        CloseHandle(process_handle);
        process_handle = nullptr;
    }
    process_id = 0;
    game_assembly_base = 0;
}

uintptr_t memory::get_module_base(const wchar_t* module_name) {
    if (!process_id) return 0;

    // Try using EnumProcessModules for reliability
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(process_handle, hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            wchar_t szModName[MAX_PATH];
            if (GetModuleBaseNameW(process_handle, hMods[i], szModName, sizeof(szModName) / sizeof(wchar_t))) {
                if (_wcsicmp(szModName, module_name) == 0) {
                    return (uintptr_t)hMods[i];
                }
            }
        }
    }

    // Fallback to toolhelp snapshot
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W me = { sizeof(me) };
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, module_name) == 0) {
                CloseHandle(snap);
                return (uintptr_t)me.modBaseAddr;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return 0;
}

bool memory::read_raw(uintptr_t address, void* buffer, size_t size) {
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(process_handle, (LPCVOID)address, buffer, size, &bytesRead) && bytesRead == size;
}

bool memory::write_raw(uintptr_t address, const void* buffer, size_t size) {
    SIZE_T bytesWritten = 0;
    return WriteProcessMemory(process_handle, (LPVOID)address, buffer, size, &bytesWritten) && bytesWritten == size;
}

uintptr_t memory::resolve_gchandle(uint32_t handle) {
    static uintptr_t gc_table = 0;
    if (!gc_table) {
        gc_table = game_assembly_base + 0x100C22A0; // gc_handles = 0x100C22A0
    }
    uint32_t index = handle >> 3;
    uintptr_t slot = read<uintptr_t>(gc_table + index * 8);
    if (slot & 1)
        return slot & ~1;
    return slot;
}
