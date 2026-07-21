#include "memory.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <string.h>

bool memory::attach() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32 pe = { sizeof(pe) };
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, "RustClient.exe") == 0) {
                process_id = pe.th32ProcessID;
                CloseHandle(snap);
                process_handle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, process_id);
                return process_handle != nullptr;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return false;
}

void memory::detach() {
    if (process_handle) {
        CloseHandle(process_handle);
        process_handle = nullptr;
    }
    process_id = 0;
}

uintptr_t memory::get_module_base(const wchar_t* module_name) {
    if (!process_id) return 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32 me = { sizeof(me) };
    if (Module32First(snap, &me)) {
        do {
            if (_stricmp(me.szModule, "GameAssembly.dll") == 0) {
                CloseHandle(snap);
                return (uintptr_t)me.modBaseAddr;
            }
        } while (Module32Next(snap, &me));
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
        gc_table = game_assembly_base + 0x100C22A0;
    }
    uint32_t index = handle >> 3;
    uintptr_t slot = read<uintptr_t>(gc_table + index * 8);
    if (slot & 1)
        return slot & ~1;
    return slot;
}
