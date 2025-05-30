#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <memory>
#include <string>

class Memory
{
public:
    std::uintptr_t processId = 0;
    void* processHandle = nullptr;

    bool Attach(const wchar_t* processName) noexcept
    {
        ::PROCESSENTRY32 entry = { };
        entry.dwSize = sizeof(::PROCESSENTRY32);
        const HANDLE snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapShot == INVALID_HANDLE_VALUE)
            return false;
        bool found = false;
        while (::Process32Next(snapShot, &entry))
        {
            if (!_wcsicmp(processName, entry.szExeFile))
            {
                processId = entry.th32ProcessID;
                processHandle = ::OpenProcess(PROCESS_VM_READ, FALSE, processId);
                found = (processHandle != nullptr);
                break;
            }
        }
        if (snapShot)
            ::CloseHandle(snapShot);
        return found;
    }

    void Detach() noexcept
    {
        if (processHandle)
        {
            ::CloseHandle(processHandle);
            processHandle = nullptr;
            processId = 0;
        }
    }

    std::uintptr_t GetModuleBase(const wchar_t* moduleName) const noexcept
    {
        ::MODULEENTRY32 entry = { };
        entry.dwSize = sizeof(::MODULEENTRY32);
        const HANDLE snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
        if (snapShot == INVALID_HANDLE_VALUE)
            return 0;
        std::uintptr_t result = 0;
        while (::Module32Next(snapShot, &entry))
        {
            if (!_wcsicmp(moduleName, entry.szModule))
            {
                result = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
                break;
            }
        }
        if (snapShot)
            ::CloseHandle(snapShot);
        return result;
    }

    template <typename T>
    T read(const std::uintptr_t address) const noexcept
    {
        T value = { };
        ::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), &value, sizeof(T), NULL);
        return value;
    }

    bool read(const std::uintptr_t address, const void* buffer, size_t size) const noexcept
    {
        SIZE_T bytesRead = 0;
        return ::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), (LPVOID)buffer, size, &bytesRead);
    }

    template <typename T>
    bool Write(const std::uintptr_t address, const T& value) const noexcept
    {
        SIZE_T bytesWritten = 0;
        return ::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), &value, sizeof(T), &bytesWritten);
    }

    bool WriteRaw(const std::uintptr_t address, const void* buffer, size_t size) const noexcept
    {
        SIZE_T bytesWritten = 0;
        return ::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), buffer, size, &bytesWritten);
    }
};

inline Memory m;