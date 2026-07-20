#include "memory.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <string.h>
#include <fstream>
#include <vector>

// اسکنر الگو روی فایل
static uintptr_t FindPatternInFile(const std::wstring& filePath, const char* pattern) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;
    size_t fileSize = (size_t)file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    // تبدیل pattern رشته‌ای به بایت و ماسک
    std::vector<uint8_t> bytes;
    std::vector<bool> mask;
    const char* p = pattern;
    while (*p) {
        if (*p == ' ') { ++p; continue; }
        if (*p == '?') {
            bytes.push_back(0);
            mask.push_back(true);
            if (p[1] == '?') p++;
            p++;
            continue;
        }
        char hex[3] = { p[0], p[1], 0 };
        bytes.push_back((uint8_t)strtoul(hex, nullptr, 16));
        mask.push_back(false);
        p += 2;
    }

    // جستجو
    for (size_t i = 0; i < fileSize - bytes.size(); i++) {
        bool found = true;
        for (size_t j = 0; j < bytes.size(); j++) {
            if (!mask[j] && buffer[i + j] != bytes[j]) {
                found = false;
                break;
            }
        }
        if (found) return i; // آفست توی فایل
    }
    return 0;
}

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

std::wstring memory::get_module_path(const wchar_t* module_name) {
    if (!process_id) return L"";
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
    if (snap == INVALID_HANDLE_VALUE) return L"";
    MODULEENTRY32 me = { sizeof(me) };
    if (Module32First(snap, &me)) {
        do {
            if (_stricmp(me.szModule, "GameAssembly.dll") == 0) {
                CloseHandle(snap);
                return me.szExePath; // مسیر کامل
            }
        } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);
    return L"";
}

bool memory::read_raw(uintptr_t address, void* buffer, size_t size) {
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(process_handle, (LPCVOID)address, buffer, size, &bytesRead) && bytesRead == size;
}

bool memory::write_raw(uintptr_t address, const void* buffer, size_t size) {
    SIZE_T bytesWritten = 0;
    return WriteProcessMemory(process_handle, (LPVOID)address, buffer, size, &bytesWritten) && bytesWritten == size;
}

// اسکن و ذخیره آدرس جدول GCHandle (یک بار انجام می‌شود)
static uintptr_t g_gchandle_table = 0;
static bool g_gchandle_init = false;

uintptr_t memory::resolve_gchandle(uint32_t handle) {
    if (!g_gchandle_init) {
        // پیدا کردن مسیر فایل GameAssembly.dll
        std::wstring dllPath = get_module_path(L"GameAssembly.dll");
        if (dllPath.empty()) return 0;

        // Signature مخصوص: lea rcx, [rip+disp32]; call ...
        // این signature در تابع il2cpp_gchandle_get_target وجود دارد.
        const char* sig = "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B ? ? ? ? ?";
        uintptr_t fileOffset = FindPatternInFile(dllPath, sig);
        if (fileOffset == 0) return 0;

        // بازخوانی 4 بایت جابجایی (Disp32) از محل signature + 3
        std::ifstream file(dllPath, std::ios::binary);
        if (!file) return 0;
        file.seekg(fileOffset + 3);
        int32_t disp32 = 0;
        file.read(reinterpret_cast<char*>(&disp32), sizeof(disp32));
        file.close();

        // محاسبه آدرس مجازی جدول GCHandle
        // RVA (آفست داخل فایل) = fileOffset + 7 + disp32
        // اما چون ما از فایل می‌خوانیم، RVA = fileOffset + 7 + disp32
        uintptr_t rva = fileOffset + 7 + disp32;
        // آدرس نهایی = بیس GameAssembly.dll + rva
        g_gchandle_table = game_assembly_base + rva;
        g_gchandle_init = true;
    }

    if (g_gchandle_table == 0 || handle <= 0) return 0;

    // استاندارد il2cpp GCHandle decryption
    uint32_t index = handle >> 3;
    uintptr_t slot = read<uintptr_t>(g_gchandle_table + index * 8);
    if (slot & 1)
        return slot & ~1;
    return slot;
}
