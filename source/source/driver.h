#pragma once

#include "../api/driver/driver_um_lib.hpp"

#ifdef log
#undef log
#endif

#ifdef log_new_line
#undef log_new_line
#endif

#include <Windows.h>
#include <TlHelp32.h>
#include <winternl.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

namespace user_driver
{
    inline std::string narrow(const wchar_t* value)
    {
        if (!value)
            return {};

        const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
        if (size <= 1)
            return {};

        std::string result(static_cast<size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
        result.resize(static_cast<size_t>(size - 1));
        return result;
    }

    inline std::string lowercase(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    inline bool names_match(const char* lhs, const std::string& rhs)
    {
        if (!lhs || rhs.empty())
            return false;

        return lowercase(lhs) == lowercase(rhs);
    }

    inline uint64_t get_module_base_from_snapshot(uint64_t pid, const std::string& module_name)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, static_cast<DWORD>(pid));
        if (snapshot == INVALID_HANDLE_VALUE)
            return 0;

        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        uint64_t base = 0;
        if (Module32FirstW(snapshot, &entry)) {
            do {
                const std::string entry_name = narrow(entry.szModule);
                if (names_match(entry_name.c_str(), module_name)) {
                    base = reinterpret_cast<uint64_t>(entry.modBaseAddr);
                    break;
                }
            } while (Module32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return base;
    }
}

namespace n_driver
{
    class c_driver
    {
    public:
        uint64_t m_process_id = 0;
        uint64_t m_base_address = 0;
        uint64_t m_process_cr3 = 0;
        uint64_t m_local_process_id = 0;
        uint64_t m_local_cr3 = 0;

        bool init_driver()
        {
            if (!physmem::init_driver())
                return false;

            m_local_process_id = GetCurrentProcessId();
            m_local_cr3 = physmem::get_cr3(m_local_process_id);

            return m_local_cr3 != 0;
        }

        bool setup()
        {
            return init_driver();
        }

        bool attach(const wchar_t* process_name)
        {
            m_process_id = 0;
            m_process_cr3 = 0;
            m_base_address = 0;

            if (!init_driver())
                return false;

            const std::string process_name_a = user_driver::narrow(process_name);
            if (process_name_a.empty())
                return false;

            const uint64_t process_id = physmem::get_pid_by_name(process_name_a.c_str());
            if (!process_id)
                return false;

            const uint64_t process_cr3 = physmem::get_cr3(process_id);
            if (!process_cr3)
                return false;

            const uint64_t base_address = get_module_base(process_id, process_name_a);
            if (!base_address)
                return false;

            m_process_id = process_id;
            m_process_cr3 = process_cr3;
            m_base_address = base_address;
            return true;
        }

        bool attach(const std::wstring& process_name)
        {
            return attach(process_name.c_str());
        }

        bool read_memory(uint64_t address, void* buffer, uint64_t size)
        {
            if (!buffer || !address || !size || !m_process_cr3 || !m_local_cr3)
                return false;

            return physmem::copy_virtual_memory(
                m_process_cr3,
                m_local_cr3,
                reinterpret_cast<void*>(address),
                buffer,
                size
            );
        }

        bool write_memory(uint64_t address, const void* buffer, uint64_t size)
        {
            if (!buffer || !address || !size || !m_process_cr3 || !m_local_cr3)
                return false;

            return physmem::copy_virtual_memory(
                m_local_cr3,
                m_process_cr3,
                const_cast<void*>(buffer),
                reinterpret_cast<void*>(address),
                size
            );
        }

        template <typename T>
        T read(uint64_t address)
        {
            T value{};
            read_memory(address, &value, sizeof(T));
            return value;
        }

        template <typename T>
        bool write(uint64_t address, const T& value)
        {
            return write_memory(address, &value, sizeof(T));
        }

        std::string read_w_string(uint64_t address, uint32_t max_chars = 128)
        {
            uint64_t string_address = read<uint64_t>(address);
            int length = read<int>(address + sizeof(uint64_t));

            if (!string_address || length <= 0 || length > static_cast<int>(max_chars)) {
                string_address = address;
                length = static_cast<int>(max_chars);
            }

            std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
            if (!read_memory(string_address, buffer.data(), static_cast<uint64_t>(length) * sizeof(wchar_t)))
                return {};

            std::wstring wide(buffer.data());
            return user_driver::narrow(wide.c_str());
        }

    private:
        uint64_t get_module_base(uint64_t pid, const std::string& module_name)
        {
            const uint64_t count = physmem::get_ldr_data_table_entry_count(pid);
            if (count > 0 && count < 4096) {
                std::vector<module_info_t> modules(static_cast<size_t>(count));

                if (physmem::get_data_table_entry_info(pid, modules.data())) {
                    for (const auto& module : modules) {
                        if (module.base && user_driver::names_match(module.name, module_name))
                            return module.base;
                    }

                    for (const auto& module : modules) {
                        if (module.base)
                            return module.base;
                    }
                }
            }

            return user_driver::get_module_base_from_snapshot(pid, module_name);
        }
    };
}

inline n_driver::c_driver* Kernel = new n_driver::c_driver();
