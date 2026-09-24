#pragma once
#include "../protection/security_macros.h"
#include <random>
#include <__msvc_chrono.hpp>
#include <Windows.h>
#include <winioctl.h>
#include <ntstatus.h>

#include <winternl.h>
#include <TlHelp32.h>
#include <chrono>
#include <map>

//inline const auto Kernel = std::make_unique<kernel>();

//NEW DRIVER
#include "../impl/ia32/ia32.h"

extern "C" __int64 direct_device_control(
    HANDLE,
    HANDLE,
    PIO_APC_ROUTINE,
    PVOID,
    PIO_STATUS_BLOCK,
    std::uint32_t,
    PVOID,
    std::uint32_t,
    PVOID,
    std::uint32_t
);



namespace n_driver {
    enum class e_command_type : std::uint32_t {
        open_process,
        get_process_peb,
        read_memory,
        write_memory,
        get_eprocess,
        get_base_address,
        translate_linear,
        map_physical,
        get_dtb
    };

    struct s_command_data {
        std::uint32_t m_pid{};
        std::uintptr_t m_address{};
        std::uintptr_t m_address2{};
        eprocess_t* m_process{};
        std::size_t m_size{};
        int m_new_protection{};
        int m_old_protection{};
        void* m_buffer{};
        peb_t* m_peb{};
        kapc_state_t m_apc_state{};
    };

    class c_command {
    public:
        c_command() = default;
        c_command(
            e_command_type type,
            s_command_data data,
            bool completed = false)
            : m_type(type), m_data(data), m_completed(completed), m_status(STATUS_SUCCESS) {
        }

        bool is_completed() const { return m_completed; }
        void set_completed(bool state) { m_completed = state; }

        NTSTATUS get_status() const { return m_status; }
        void set_status(NTSTATUS status_code) {
            m_status = status_code;
        }

        e_command_type get_type() const { return m_type; }
        const s_command_data& get_data() const { return m_data; }

    private:
        e_command_type m_type;
        s_command_data m_data;
        bool m_completed;
        std::uint64_t m_timestamp;
        NTSTATUS m_status;
    };

    class c_driver
    {
    public:
        bool setup() {
           printf(("\nStarting driver installation\n"));

            m_driver_handle = CreateFileW(
                L"\\\\.\\{*discord_overlay2*}", 
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                nullptr
            );

            if (m_driver_handle == INVALID_HANDLE_VALUE) {
                printf(("Failed to open driver handle. Error: %x\n"), GetLastError());
                return false;
            }

            printf(("Successfully opened driver handle\n"));
            return true;
        }

        bool attach(const wchar_t* target_name) {
            printf(("Searching for process\n"));

            while (true) {
                m_process_id = get_process_id(target_name);
                if (m_process_id) break;
                Sleep(250);
            }

           printf(("Process PID: %i"), m_process_id);

            m_process_handle = open_process(m_process_id);
              if ( !m_process_handle )
                   return false;

            printf(("Handle: %llx"), m_process_handle);

            m_eprocess = get_eprocess(m_process_id);
               if ( !m_eprocess )
                   return false;

            printf(("EProcess: %llx"), m_eprocess);

            m_base_address = get_base_address(m_eprocess);
              if ( !m_base_address )
                  return false;

            printf(("Base Address: %llx"), m_base_address);

            auto time_now = std::chrono::high_resolution_clock::now();

            m_dtb = get_dtb(m_base_address);
                      if ( !m_dtb )
                          return false;

            auto time_stop = std::chrono::duration_cast<std::chrono::duration< float >>(
                std::chrono::high_resolution_clock::now() - time_now
            );

            printf(("Found DTB: %llx, MS: %f"), m_dtb, time_stop.count());
            printf(("Successfully found process\n"));

            return true;
        }

        bool read_memory(uintptr_t address, void* buffer, size_t size) {
            s_command_data data{};
            data.m_process = this->m_eprocess;
            data.m_pid = this->m_process_id;
            data.m_address = address;
            data.m_buffer = buffer;
            data.m_size = size;

            auto result = send_command(e_command_type::read_memory, data);
            return set_last_status(result.get_status());
        }

        bool write_memory(uintptr_t address, void* buffer, size_t size) {
            s_command_data data{};
            data.m_pid = this->m_process_id;
            data.m_process = this->m_eprocess;
            data.m_address = address;
            data.m_buffer = buffer;
            data.m_size = size;

            auto result = send_command(e_command_type::write_memory, data);
            buffer = result.get_data().m_buffer;
            return set_last_status(result.get_status());
        }

        std::uintptr_t get_dtb(std::uintptr_t base_address) {
            s_command_data data{};
            data.m_process = this->m_eprocess;
            data.m_address = base_address;

            auto result = send_command(e_command_type::get_dtb, data);
            set_last_status(result.get_status());
            return result.get_data().m_address2;
        }

        std::uintptr_t get_base_address(eprocess_t* process) {
            s_command_data data{};
            data.m_process = process;

            auto result = send_command(e_command_type::get_base_address, data);
            set_last_status(result.get_status());
            return result.get_data().m_address;
        }

        std::uintptr_t map_physical(std::uintptr_t address, std::uint32_t size) {
            s_command_data data{};
            data.m_address = address;
            data.m_size = size;

            auto result = send_command(e_command_type::map_physical, data);
            set_last_status(result.get_status());
            return result.get_data().m_address2;
        }

        std::pair<std::uintptr_t, std::size_t> translate_linear(std::uintptr_t address) {
            s_command_data data{};
            data.m_address = address;

            auto result = send_command(e_command_type::translate_linear, data);
            set_last_status(result.get_status());
            return { result.get_data().m_address2, result.get_data().m_size };
        }

        peb_t* get_peb(eprocess_t* process) {
            s_command_data data{};
            data.m_process = process;

            auto result = send_command(e_command_type::get_process_peb, data);
            set_last_status(result.get_status());
            return result.get_data().m_peb;
        }

        peb_t get_process_peb(eprocess_t* process) {
            peb_t process_peb;
            auto peb = get_peb(process);
            read_memory(reinterpret_cast<std::uintptr_t>(peb), &process_peb, sizeof(peb_t));
            return process_peb;
        }

        eprocess_t* get_eprocess(std::uint32_t process_id) {
            s_command_data data{};
            data.m_pid = process_id;

            auto result = send_command(e_command_type::get_eprocess, data);
            set_last_status(result.get_status());
            return result.get_data().m_process;
        }

        void* open_process(std::uint32_t process_id) {
            s_command_data data{};
            data.m_pid = process_id;

            auto result = send_command(e_command_type::open_process, data);
            set_last_status(result.get_status());
            return result.get_data().m_buffer;
        }

        template <typename T>
        std::vector<T> read_array(uintptr_t address, size_t count) {
            std::vector<T> buffer(count);
            const size_t size = count * sizeof(T);

            if (!read_memory(address, buffer.data(), size)) {
                printf(("Failed to read array of %zu elements from 0x%llX"), count, address);
                return {};
            }

            return buffer;
        }

        template<typename ret_t = std::uintptr_t, typename addr_t>
        ret_t read(addr_t address) {
            ret_t data{};
            if (!read_memory(
                address,
                &data,
                sizeof(ret_t)
            )) return ret_t{};
            return data;
        }

        std::string read_w_string(const std::uint64_t& Address) {
            if (!Address)
                return "";

            std::uint64_t NewAddress = read<std::uint64_t>(Address);
            if (!NewAddress)
                return "";

            int Length = read<int>(NewAddress + 0x10);
            if (Length <= 0)
                return "";

            std::vector<wchar_t> Buffer(Length + 1, L'\0');

            read_memory(uintptr_t(NewAddress + 0x14), Buffer.data(), Length * sizeof(wchar_t));

            std::wstring TempString(Buffer.data(), Length);

            for (auto& ch : TempString) {
                if (ch < 32 || ch > 126) {
                    ch = L'?';
                }
            }

            return std::string(TempString.begin(), TempString.end());
        }

        template<typename data_t, typename addr_t>
        bool write(addr_t address, data_t data) {
            return this->write_memory(
                address,
                &data,
                sizeof(data_t)
            );
        }


        void unload() {
            if (m_driver_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(m_driver_handle);
                m_driver_handle = INVALID_HANDLE_VALUE;
            }
        }

        std::uintptr_t find_min(std::uint32_t g, std::size_t f) {
            auto h = (std::uint32_t)f;

            return (((g) < (h)) ? (g) : (h));
        }

        std::uint32_t get_process_id(std::wstring module_name) {
            auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE)
                return false;

            PROCESSENTRY32W process_entry{ };
            process_entry.dwSize = sizeof(process_entry);
            Process32FirstW(snapshot, &process_entry);
            do {
                if (!module_name.compare(process_entry.szExeFile))
                    return process_entry.th32ProcessID;
            } while (Process32NextW(snapshot, &process_entry));

            return 0;
        }

        std::uintptr_t get_process_module(const wchar_t* module_name) {
            if (!m_process_handle || !m_eprocess)
                return 0;

            auto process_peb = get_peb(m_eprocess);
            if (!process_peb)
                return 0;

            peb_t peb_data;
            if (!read_memory(reinterpret_cast<std::uintptr_t>(process_peb), &peb_data, sizeof(peb_t)))
                return 0;

            peb_ldr_data_t ldr_data;
            if (!read_memory(reinterpret_cast<std::uintptr_t>(peb_data.m_ldr), &ldr_data, sizeof(peb_ldr_data_t)))
                return 0;

            auto current_entry = ldr_data.m_module_list_load_order.m_flink;
            auto first_entry = current_entry;

            do {
                ldr_data_table_entry_t entry;
                if (!read_memory(reinterpret_cast<std::uintptr_t>(current_entry), &entry, sizeof(ldr_data_table_entry_t)))
                    break;

                wchar_t module_name_buffer[MAX_PATH];
                if (entry.m_base_dll_name.m_length > 0 && entry.m_base_dll_name.m_length < MAX_PATH * 2) {
                    if (read_memory(reinterpret_cast<std::uintptr_t>(entry.m_base_dll_name.m_buffer),
                        module_name_buffer,
                        entry.m_base_dll_name.m_length)) {

                        module_name_buffer[entry.m_base_dll_name.m_length / 2] = L'\0';
                        if (_wcsicmp(module_name_buffer, module_name) == 0) {
                            return reinterpret_cast<std::uintptr_t>(entry.m_dll_base);
                        }
                    }
                }

                current_entry = entry.m_in_load_order_module_list.m_flink;
            } while (current_entry && current_entry != first_entry);

            return 0;
        }

        /*std::uintptr_t get_process_module( const wchar_t* module_name ) {
            const auto handle = OpenProcess( PROCESS_QUERY_INFORMATION, 0, m_process_id );
            logging::print( "handle: %llx", handle );

            auto current = 0ull;
            auto mbi = MEMORY_BASIC_INFORMATION( );
            while ( VirtualQueryEx( handle, reinterpret_cast< void* >( current ), &mbi, sizeof( MEMORY_BASIC_INFORMATION ) ) ) {
                if ( mbi.Type == MEM_MAPPED || mbi.Type == MEM_IMAGE ) {
                    const auto buffer = malloc( 1024 );
                    auto bytes = std::size_t( );
                    const static auto ntdll = GetModuleHandleA( ( "ntdll" ) );
                    const static auto nt_query_virtual_memory_fn =
                        reinterpret_cast< NTSTATUS( __stdcall* )( HANDLE, void*, std::int32_t, void*, std::size_t, std::size_t* ) > (
                            GetProcAddress( ntdll, ( "NtQueryVirtualMemory" ) ) );

                    if ( nt_query_virtual_memory_fn( handle, mbi.BaseAddress, 2, buffer, 1024, &bytes ) != 0 ||
                        !wcsstr( static_cast< UNICODE_STRING* >( buffer )->Buffer, module_name ) ||
                        wcsstr( static_cast< UNICODE_STRING* >( buffer )->Buffer, ( L".mui" ) ) ) {
                        free( buffer );
                        goto skip;
                    }
                    free( buffer );
                    CloseHandle( handle );

                    return reinterpret_cast< std::uintptr_t >( mbi.BaseAddress );
                }
            skip:
                current = reinterpret_cast< std::uintptr_t >( mbi.BaseAddress ) + mbi.RegionSize;
            }
            CloseHandle( handle );
            return 0ull;
        }*/

        std::vector<void*> get_process_threads() {
            std::vector<void*> threads;

            if (!m_process_id || !m_eprocess)
                return threads;

            list_entry_t list_head;
            if (!read_memory(reinterpret_cast<std::uintptr_t>(m_eprocess) + 0x030, &list_head, sizeof(list_entry_t)))
                return threads;

            list_entry_t current_entry;
            if (!read_memory(reinterpret_cast<std::uintptr_t>(list_head.m_flink), &current_entry, sizeof(list_entry_t)))
                return threads;

            auto first_entry = current_entry;

            do {
                ethread_t entry;
                if (!read_memory(reinterpret_cast<std::uintptr_t>(current_entry.m_flink) - 0x4e8,
                    &entry, sizeof(ethread_t)))
                    break;

                //logging::print(encrypt("Thread handle: %llx"), entry.m_client_id.m_unique_thread);
                threads.push_back(entry.m_client_id.m_unique_thread);

                read_memory(
                    reinterpret_cast<std::uintptr_t>(current_entry.m_flink),
                    &current_entry,
                    sizeof(current_entry)
                );

                if (current_entry.m_flink == first_entry.m_flink)
                    break;

            } while (current_entry.m_flink != first_entry.m_flink);
            return threads;
        }

        std::map<std::uintptr_t, std::wstring> get_process_modules() {
            std::map<std::uintptr_t, std::wstring> modules;

            if (!m_process_id || !m_eprocess)
                return modules;

            auto process_peb = get_process_peb(m_eprocess);
            if (!process_peb.m_ldr)
                return modules;

            peb_ldr_data_t ldr_data;
            if (!read_memory(reinterpret_cast<std::uintptr_t>(process_peb.m_ldr), &ldr_data, sizeof(peb_ldr_data_t)))
                return modules;

            auto current_entry = ldr_data.m_module_list_in_it_order;
            auto first_entry = current_entry;

            do {
                ldr_data_table_entry_t entry;
                if (!read_memory(reinterpret_cast<std::uintptr_t>(current_entry.m_flink) - offsetof(ldr_data_table_entry_t, m_in_initialization_order_module_list),
                    &entry, sizeof(ldr_data_table_entry_t)))
                    break;

                wchar_t module_name[MAX_PATH] = { 0 };
                if (entry.m_base_dll_name.m_length > 0 && entry.m_base_dll_name.m_buffer) {
                    if (!read_memory(reinterpret_cast<std::uintptr_t>(entry.m_base_dll_name.m_buffer),
                        module_name,
                        entry.m_base_dll_name.m_length))
                        break;
                }

                if (entry.m_dll_base && module_name[0] != L'\0') {
                    modules[reinterpret_cast<std::uintptr_t>(entry.m_dll_base)] = module_name;
                    printf(("Found module: %S at 0x%llX"), module_name, entry.m_dll_base);
                }

                read_memory(
                    reinterpret_cast<std::uintptr_t>(current_entry.m_flink),
                    &current_entry,
                    sizeof(current_entry)
                );

                if (current_entry.m_flink == first_entry.m_flink)
                    break;

            } while (current_entry.m_flink != nullptr && current_entry.m_flink != first_entry.m_flink);
            return modules;
        }

        std::uintptr_t get_module_export(std::uintptr_t module_base, const char* export_name) {
            if (!module_base || !export_name)
                return 0;

            IMAGE_DOS_HEADER dos_header;
            if (!read_memory(module_base, &dos_header, sizeof(IMAGE_DOS_HEADER)))
                return 0;

            if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
                return 0;

            IMAGE_NT_HEADERS nt_headers;
            if (!read_memory(module_base + dos_header.e_lfanew, &nt_headers, sizeof(IMAGE_NT_HEADERS)))
                return 0;

            if (nt_headers.Signature != IMAGE_NT_SIGNATURE)
                return 0;

            auto export_dir = &nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
            if (!export_dir->VirtualAddress || !export_dir->Size)
                return 0;

            IMAGE_EXPORT_DIRECTORY export_directory;
            if (!read_memory(module_base + export_dir->VirtualAddress, &export_directory, sizeof(IMAGE_EXPORT_DIRECTORY)))
                return 0;

            auto functions = read_array<DWORD>(module_base + export_directory.AddressOfFunctions,
                export_directory.NumberOfFunctions);
            if (functions.empty())
                return 0;

            auto names = read_array<DWORD>(module_base + export_directory.AddressOfNames,
                export_directory.NumberOfNames);
            if (names.empty())
                return 0;

            auto ordinals = read_array<WORD>(module_base + export_directory.AddressOfNameOrdinals,
                export_directory.NumberOfNames);
            if (ordinals.empty())
                return 0;

            for (DWORD i = 0; i < export_directory.NumberOfNames; i++) {
                char name_buffer[256] = { 0 };
                if (!read_memory(module_base + names[i], name_buffer, sizeof(name_buffer) - 1))
                    continue;

                if (strcmp(name_buffer, export_name) == 0) {
                    WORD ordinal = ordinals[i];
                    return module_base + functions[ordinal];
                }
            }

            return 0;
        }


        public:
        std::uint32_t m_process_id;
        eprocess_t* m_eprocess;
        peb_t* m_process_peb;
        void* m_process_handle;
        std::uintptr_t m_base_address;
        std::uintptr_t m_game_assembly;
        std::uintptr_t m_dtb;
        NTSTATUS m_last_error;

        void debug_last_exception() const {
            printf(( "Driver exceptions:" ) );
            printf(( "  - Exception code: 0x%d" ), m_last_error );
        }
    private:
        HANDLE m_driver_handle = INVALID_HANDLE_VALUE;

        bool set_last_status(NTSTATUS error_code) {
            this->m_last_error = error_code;
            return NT_SUCCESS(error_code);
        }

        c_command send_command(e_command_type type, const s_command_data& data, std::uint32_t timeout_ms = 2000) const {
            c_command command(type, data);

            if (this->m_driver_handle == INVALID_HANDLE_VALUE) {
                //logging::print(encrypt("Cannot send command: driver handle not valid"));
                command.set_status(STATUS_UNSUCCESSFUL);
                return command;
            }

            IO_STATUS_BLOCK block;
            auto result =
                direct_device_control(
                    this->m_driver_handle,
                    nullptr,
                    nullptr,
                    nullptr,
                    &block,
                    0,
                    &command,
                    sizeof(command),
                    &command,
                    sizeof(command));
            return command;
        }
    };

}
inline auto Kernel = new n_driver::c_driver;

//#pragma once
//#include <Windows.h>
//#include <vector>
//#include <string>
//#include <chrono>
//#include <map>
//#include <iostream>
//
//// Include the new driver header
//#pragma once
//#pragma once
//#pragma optimize("", off)
//// Designed to be a standalone, includable .hpp, thus we need to make our own definitions etc.
//
///**
// * @file driver_shared.hpp
// * @brief Shared headers and structures used for communication between user mode and driver.
// */
//
// /*
//     Typedefs and Definitions
// */
//
// /**
//  * @brief Defines the maximum path length for file paths and other strings.
//  */
//#ifndef MAX_PATH
//#define MAX_PATH 260
//#endif // !MAX_PATH
//
//  /**
//   * @brief Typedef for unsigned 8-bit integer.
//   */
//using uint8_t = unsigned char;
//
///**
// * @brief Typedef for unsigned 16-bit integer.
// */
//using uint16_t = unsigned short;
//
///**
// * @brief Typedef for unsigned 32-bit integer.
// */
//using uint32_t = unsigned int;
//
///**
// * @brief Typedef for unsigned 64-bit integer.
// */
//using uint64_t = unsigned long long;
//
///*
//    Communication structs
//*/
//
///**
// * @brief Structure for copying virtual memory between two different processes.
// */
//struct copy_virtual_memory_t {
//    _In_ uint64_t src_cr3;  ///< The CR3 value of the source process.
//    _In_ uint64_t dst_cr3;  ///< The CR3 value of the destination process.
//    _In_ void* src;         ///< Pointer to the source memory location.
//    _In_ void* dst;         ///< Pointer to the destination memory location.
//    _In_ uint64_t size;     ///< The size of memory to copy.
//};
//
///**
// * @brief Structure for retrieving the CR3 value for a specified process.
// */
//struct get_cr3_t {
//    _In_ uint64_t pid;      ///< The process ID of the target process.
//    _Out_ uint64_t cr3;     ///< The CR3 value for the process.
//};
//
///**
// * @brief Structure for retrieving the base address of a specified module.
// */
//struct get_module_base_t {
//    _In_ char module_name[MAX_PATH]; ///< The name of the module.
//    _In_ uint64_t pid;               ///< The process ID of the target process.
//    _Out_ uint64_t module_base;      ///< The base address of the module.
//};
//
///**
// * @brief Structure for retrieving the size of a specified module.
// */
//struct get_module_size_t {
//    _In_ char module_name[MAX_PATH]; ///< The name of the module.
//    _In_ uint64_t pid;               ///< The process ID of the target process.
//    _Out_ uint64_t module_size;      ///< The size of the module.
//};
//
///**
// * @brief Structure for retrieving the process ID by process name.
// */
//struct get_pid_by_name_t {
//    _In_ char name[MAX_PATH];  ///< The name of the process.
//    _Out_ uint64_t pid;        ///< The process ID of the target process.
//};
//
///**
// * @brief Structure for retrieving the count of loaded modules in a target process.
// */
//struct get_ldr_data_table_entry_count_t {
//    _In_ uint64_t pid;         ///< The process ID of the target process.
//    _Out_ uint64_t count;      ///< The number of loaded modules in the process.
//};
//
///**
// * @brief Structure for storing module information.
// */
//struct module_info_t {
//    _In_ char name[MAX_PATH]; ///< The name of the module.
//    _In_ uint64_t base;       ///< The base address of the module.
//    _In_ uint64_t size;       ///< The size of the module.
//};
//
///**
// * @brief Structure for retrieving module information about loaded modules.
// */
//struct cmd_get_data_table_entry_info_t {
//    _In_ uint64_t pid;              ///< The process ID of the target process.
//    _In_ module_info_t* info_array; ///< Array of module information.
//};
//
///**
// * @brief Maximum number of messages that can be stored in the log.
// */
//#define MAX_MESSAGES 512
//
// /**
//  * @brief Maximum size of each log message.
//  */
//#define MAX_MESSAGE_SIZE 256
//
//  /**
//   * @brief Structure for storing a log entry.
//   */
//struct log_entry_t {
//    bool present;                ///< Indicates if the log entry is present.
//    char payload[MAX_MESSAGE_SIZE]; ///< The log message content.
//};
//
///**
// * @brief Structure for retrieving logs from the driver.
// */
//struct cmd_output_logs_t {
//    _In_ uint32_t count;        ///< The number of log entries to retrieve.
//    _In_ log_entry_t* log_array; ///< The array to store log entries.
//};
//
///**
// * @brief Enum representing the different types of commands that can be sent to the driver.
// */
//enum call_types_t : uint32_t {
//    cmd_get_pid_by_name,                ///< Get process ID by process name.
//    cmd_get_cr3,                         ///< Get CR3 value for a process.
//    cmd_get_module_base,                 ///< Get the base address of a module.
//    cmd_get_module_size,                 ///< Get the size of a module.
//    cmd_get_ldr_data_table_entry_count,  ///< Get the number of loaded modules in a process.
//    cmd_get_data_table_entry_info,       ///< Get information about loaded modules.
//    cmd_copy_virtual_memory,             ///< Copy virtual memory between processes.
//    cmd_output_logs,                     ///< Retrieve logs from the driver.
//    cmd_remove_from_system_page_tables,  ///< Remove the driver from the system page tables.
//    cmd_unload_driver,                   ///< Unload the driver.
//    cmd_ping_driver,                     ///< Ping the driver to check its status.
//};
//
///**
// * @brief Structure representing a command to be sent to the driver.
// */
//struct command_t {
//    bool status;               ///< Status of the command (success or failure).
//    call_types_t call_type;    ///< The type of command being sent.
//    void* sub_command_ptr;     ///< Pointer to the sub-command data.
//};
//
//#pragma optimize("", on)
//
//
//// Constants
///// Stack identifier used for handling specific memory stack operations.
//constexpr uint64_t stack_id = 0xdeed;
//
///// Signature representing an NMI (Non-Maskable Interrupt) occurrence.
//constexpr uint64_t nmi_occured = 0x01010101;
//
///// A unique signature identifier for the caller (used for validation).
//constexpr uint32_t caller_signature = 0x6969;
//
///// Win11 23H2+ build number threshold (matches the driver-side check).
//constexpr uint32_t win11_23h2_build = 22631;
//
///// Function pointer type for the `NtUserGetCPD` function from the `win32u.dll` library (Win10/Win11 22H2).
//typedef __int64(__fastcall* NtUserGetCPD_type)(uint64_t hwnd, uint32_t flags, ULONG_PTR dw_data);
//extern "C" NtUserGetCPD_type NtUserGetCPD;
//
///// Function pointer type for the Win11 23H2+ hooked function (2-arg signature).
//typedef __int64(__fastcall* Win11HookFunc_type)(uint64_t a1, uint64_t a2);
//extern "C" Win11HookFunc_type Win11HookFunc;
//
///// External ASM functions for handling NMI interrupts.
///// These functions are invoked in assembly code for specific NMI-related operations.
//extern "C" void asm_nmi_wrapper(void);
//extern "C" void asm_nmi_restoring(void);
//extern "C" __int64 __fastcall asm_call_driver(uint64_t hwnd, uint32_t flags, ULONG_PTR dw_data);
//extern "C" __int64 __fastcall asm_call_driver_win11(uint64_t a1, uint64_t a2, uint64_t nmi_restoring_addr);
//
//namespace rose {
//    /// Flag indicating whether the Roseware library has been initialized.
//    inline bool inited = false;
//
//    /// Flag indicating whether the Win11 23H2+ hook path is active.
//    inline bool use_win11_hook = false;
//
//    /**
//     * @brief Sends a request command to the driver.
//     *
//     * This function sends a command to the driver and waits for a response.
//     *
//     * @param cmd Pointer to the command data to be sent.
//     * @return The result of the request, typically an integer or pointer.
//     */
//    __int64 send_request(void* cmd);
//
//    /**
//     * @brief Copies virtual memory between two processes.
//     *
//     * This function uses CR3 registers to copy memory from one process to another.
//     *
//     * @param src_cr3 The CR3 value of the source process.
//     * @param dst_cr3 The CR3 value of the destination process.
//     * @param src Pointer to the source memory location.
//     * @param dst Pointer to the destination memory location.
//     * @param size The size of memory to copy.
//     * @return true if the memory copy was successful, false otherwise.
//     */
//    bool copy_virtual_memory(uint64_t src_cr3, uint64_t dst_cr3, void* src, void* dst, uint64_t size);
//
//    /**
//     * @brief Retrieves the CR3 value for a given process.
//     *
//     * The CR3 register holds the physical address of the page table directory for the process.
//     *
//     * @param pid The process ID of the target process.
//     * @return The CR3 value of the target process, or 0 if an error occurs.
//     */
//    uint64_t get_cr3(uint64_t pid);
//    // Add these inside namespace rose
//    bool init_mouse();
//    bool inject_mouse(long x, long y, unsigned short button_flags);
//    /**
//     * @brief Gets the base address of a module in a target process.
//     *
//     * This function retrieves the base address of a loaded module within a target process.
//     *
//     * @param module_name The name of the module to search for.
//     * @param pid The process ID of the target process.
//     * @return The base address of the module, or 0 if not found.
//     */
//    uint64_t get_module_base(const char* module_name, uint64_t pid);
//
//    /**
//     * @brief Gets the size of a module in a target process.
//     *
//     * This function retrieves the size of a loaded module within a target process.
//     *
//     * @param module_name The name of the module to search for.
//     * @param pid The process ID of the target process.
//     * @return The size of the module, or 0 if not found.
//     */
//    uint64_t get_module_size(const char* module_name, uint64_t pid);
//
//    /**
//     * @brief Retrieves the process ID (PID) of a process by its name.
//     *
//     * This function finds the PID of a process based on its executable name.
//     *
//     * @param name The name of the process to search for.
//     * @return The PID of the process, or 0 if the process could not be found.
//     */
//    uint64_t get_pid_by_name(const char* name);
//
//    /**
//     * @brief Gets the number of loaded modules in a target process.
//     *
//     * This function retrieves the number of modules that are loaded in the target process.
//     *
//     * @param pid The process ID of the target process.
//     * @return The number of modules loaded, or 0 if an error occurs.
//     */
//    uint64_t get_ldr_data_table_entry_count(uint64_t pid);
//
//    /**
//     * @brief Retrieves detailed information about the modules loaded in a target process.
//     *
//     * This function fills the provided array with information about each module in the target process.
//     *
//     * @param pid The process ID of the target process.
//     * @param info_array Pointer to an array where module information will be stored.
//     * @return true if the module data is successfully retrieved, false otherwise.
//     */
//    bool get_data_table_entry_info(uint64_t pid, module_info_t* info_array);
//
//    /**
//     * @brief Hides the driver from detection.
//     *
//     * This function attempts to hide the driver to avoid detection by the system.
//     * It should be called during the initialization phase.
//     *
//     * @return true if the driver was successfully hidden, false otherwise.
//     */
//    bool hide_driver(void);
//
//    /**
//     * @brief Unloads the driver from memory.
//     *
//     * This function unloads the driver, effectively terminating its operation.
//     *
//     * @return true if the driver was successfully unloaded, false otherwise.
//     */
//    bool unload_driver(void);
//
//    /**
//     * @brief Pings the driver to check its status.
//     *
//     * This function verifies whether the driver is loaded and functioning correctly.
//     *
//     * @return true if the driver is responsive, false otherwise.
//     */
//    bool ping_driver(void);
//
//    /**
//     * @brief Flushes all logs accumulated during driver operations.
//     *
//     * This function flushes any logs that have been generated during the driver's operations.
//     * Typically used for debugging purposes.
//     */
//    void flush_logs(void);
//
//    /**
//     * @brief Initializes the Roseware library.
//     *
//     * This function performs initialization of the Roseware library, which includes loading necessary
//     * DLLs and preparing the driver for interaction. Detects Win11 23H2+ and uses the appropriate
//     * hook calling convention.
//     *
//     * @return true if the library is successfully initialized, false otherwise.
//     */
//    inline bool init_roseware_lib() {
//        if (inited)
//            return true;
//
//        // Load user32.dll to ensure compatibility with NtUser functions
//        if (!LoadLibraryW(L"user32.dll")) {
//           
//            return false;
//        }
//
//        // Load win32u.dll and resolve the appropriate hook function
//        HMODULE win32u = LoadLibraryW(L"win32u.dll");
//        if (!win32u) {
//            
//            return false;
//        }
//
//        // Detect Win11 23H2+ to match the driver-side hook path
//        OSVERSIONINFOW os_info = { 0 };
//        os_info.dwOSVersionInfoSize = sizeof(os_info);
//        typedef LONG(WINAPI* RtlGetVersion_t)(PRTL_OSVERSIONINFOW);
//        RtlGetVersion_t pRtlGetVersion = (RtlGetVersion_t)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
//        if (pRtlGetVersion) {
//            pRtlGetVersion((PRTL_OSVERSIONINFOW)&os_info);
//        }
//
//        use_win11_hook = (os_info.dwBuildNumber >= win11_23h2_build);
//
//        if (use_win11_hook) {
//            // Win11 23H2+: resolve the function that triggers the gSessionGlobalSlots table entry.
//            // The hooked entry at table[0xA00/8] corresponds to NtUserGetCPD on Win11 23H2+
//            // but is dispatched through the session global slots mechanism with a 2-arg signature.
//            // We still call NtUserGetCPD from usermode — the syscall routing reaches the hooked entry.
//            uint64_t handler_address = (uint64_t)GetProcAddress(win32u, "NtUserGetCPD");
//            if (!handler_address) {
//                
//                return false;
//            }
//            Win11HookFunc = (Win11HookFunc_type)handler_address;
//            NtUserGetCPD = nullptr;
//            
//        }
//        else {
//            // Win10 / Win11 22H2: use the classic NtUserGetCPD 3-arg path
//            uint64_t handler_address = (uint64_t)GetProcAddress(win32u, "NtUserGetCPD");
//            NtUserGetCPD = (NtUserGetCPD_type)handler_address;
//            Win11HookFunc = nullptr;
//        }
//
//        inited = true;
//
//        // Ensure the driver is loaded and can be pinged
//        if (!ping_driver()) {
//            std::cout <<("Driver is not loaded");
//            return false;
//        }
//
//        // Hide the driver for stealth purposes
//        if (!hide_driver()) {
//            
//            return false;
//        }
//
//        return true;
//    }
//
//    /**
//     * @brief Checks if the Roseware library has been initialized.
//     *
//     * This function checks if the library has been properly initialized.
//     *
//     * @return true if the library is initialized, false otherwise.
//     */
//    inline bool is_lib_inited(void) {
//        return inited;
//    }
//
//    /**
//     * @brief Checks if a valid hook function pointer is available.
//     *
//     * @return true if either the Win10 or Win11 hook function is resolved.
//     */
//    inline bool is_hook_ready(void) {
//        if (use_win11_hook)
//            return Win11HookFunc != nullptr;
//        return NtUserGetCPD != nullptr;
//    }
//};
//
//// Structure representing a CPU trap frame.
//#pragma pack(push, 1)
//struct trap_frame_t {
//    uint64_t r15; ///< Value of the R15 register.
//    uint64_t r14; ///< Value of the R14 register.
//    uint64_t r13; ///< Value of the R13 register.
//    uint64_t r12; ///< Value of the R12 register.
//    uint64_t r11; ///< Value of the R11 register.
//    uint64_t r10; ///< Value of the R10 register.
//    uint64_t r9;  ///< Value of the R9 register.
//    uint64_t r8;  ///< Value of the R8 register.
//    uint64_t rbp; ///< Value of the RBP register.
//    uint64_t rdi; ///< Value of the RDI register.
//    uint64_t rsi; ///< Value of the RSI register.
//    uint64_t rdx; ///< Value of the RDX register.
//    uint64_t rcx; ///< Value of the RCX register.
//    uint64_t rbx; ///< Value of the RBX register.
//    uint64_t rax; ///< Value of the RAX register.
//    uint64_t rsp; ///< Value of the RSP register.
//};
//#pragma pack(pop)
//
//
//#include <winternl.h> // Brings in NTSTATUS definitions
//
//// Re-define the missing macro for your main.cpp
//#ifndef STATUS_UNSUCCESSFUL
//#define STATUS_UNSUCCESSFUL 0xC0000001
//#endif
//
//
//namespace n_driver {
//
//    class c_driver {
//    public:
//        std::uint32_t m_process_id = 0;
//        eprocess_t* m_eprocess = nullptr;
//        peb_t* m_process_peb = nullptr;
//        void* m_process_handle = nullptr;
//        std::uintptr_t m_base_address = 0;
//        std::uintptr_t m_game_assembly = 0;
//        std::uintptr_t m_dtb = 0;
//        NTSTATUS m_last_error = 0;
//
//    private:
//        std::uint64_t m_my_cr3 = 0; // Stores current process CR3 for the new memory copy function
//
//        bool set_last_status(NTSTATUS error_code) {
//            this->m_last_error = error_code;
//            return error_code == 0; // STATUS_SUCCESS is 0
//        }
//
//    public:
//        bool setup() {
//            printf("\nStarting driver installation\n");
//
//            if (!rose::init_roseware_lib()) {
//                printf("Failed to open driver handle or initialize hooks.\n");
//                return false;
//            }
//
//            // The new driver requires the caller's CR3 (Directory Table Base) for memory routing
//            m_my_cr3 = rose::get_cr3(GetCurrentProcessId());
//
//            printf("Successfully initialized driver\n");
//            return true;
//        }
//
//        bool attach(const wchar_t* target_name) {
//                        printf(("Searching for process\n"));
//            
//                        while (true) {
//                            m_process_id = get_process_id(target_name);
//                            if (m_process_id) break;
//                            Sleep(250);
//                        }
//            
//                       printf(("Process PID: %i"), m_process_id);
//            
//                        m_process_handle = open_process(m_process_id);
//                          if ( !m_process_handle )
//                               return false;
//            
//                        printf(("Handle: %llx"), m_process_handle);
//            
//                        m_eprocess = get_eprocess(m_process_id);
//                           if ( !m_eprocess )
//                               return false;
//            
//                        printf(("EProcess: %llx"), m_eprocess);
//            
//                        m_base_address = get_base_address(m_eprocess);
//                          if ( !m_base_address )
//                              return false;
//            
//                        printf(("Base Address: %llx"), m_base_address);
//            
//                        auto time_now = std::chrono::high_resolution_clock::now();
//            
//                        m_dtb = get_dtb(m_base_address);
//                                  if ( !m_dtb )
//                                      return false;
//            
//                        auto time_stop = std::chrono::duration_cast<std::chrono::duration< float >>(
//                            std::chrono::high_resolution_clock::now() - time_now
//                        );
//            
//                        printf(("Found DTB: %llx, MS: %f"), m_dtb, time_stop.count());
//                        printf(("Successfully found process\n"));
//            
//                        return true;
//                    }
//
//        bool read_memory(uintptr_t address, void* buffer, size_t size) {
//            if (!address || !buffer || !size || !m_dtb || !m_my_cr3) return false;
//
//            bool success = rose::copy_virtual_memory(
//                m_dtb,                 // Source CR3 (Target game)
//                m_my_cr3,              // Destination CR3 (Usermode cheat)
//                (void*)address,        // Source address
//                buffer,                // Destination address
//                size                   // Size
//            );
//
//            set_last_status(success ? 0 : 0xC0000001); // STATUS_UNSUCCESSFUL
//            return success;
//        }
//
//        bool write_memory(uintptr_t address, void* buffer, size_t size) {
//            if (!address || !buffer || !size || !m_dtb || !m_my_cr3) return false;
//
//            bool success = rose::copy_virtual_memory(
//                m_my_cr3,              // Source CR3 (Usermode cheat)
//                m_dtb,                 // Destination CR3 (Target game)
//                buffer,                // Source address
//                (void*)address,        // Destination address
//                size                   // Size
//            );
//
//            set_last_status(success ? 0 : 0xC0000001);
//            return success;
//        }
//
//        std::uintptr_t get_process_module(const wchar_t* module_name) {
//            char narrow_name[MAX_PATH];
//            size_t converted;
//            wcstombs_s(&converted, narrow_name, MAX_PATH, module_name, _TRUNCATE);
//
//            return rose::get_module_base(narrow_name, m_process_id);
//        }
//
//        std::map<std::uintptr_t, std::wstring> get_process_modules() {
//            std::map<std::uintptr_t, std::wstring> modules;
//
//            if (!m_process_id) return modules;
//
//            uint64_t count = rose::get_ldr_data_table_entry_count(m_process_id);
//            if (count > 0 && count < 1000) { // arbitrary cap to prevent huge allocations
//                module_info_t* info_array = new module_info_t[count];
//
//                if (rose::get_data_table_entry_info(m_process_id, info_array)) {
//                    for (uint64_t i = 0; i < count; i++) {
//                        if (info_array[i].base != 0) {
//                            std::string s(info_array[i].name);
//                            std::wstring ws(s.begin(), s.end());
//                            modules[info_array[i].base] = ws;
//                            printf("Found module: %s at 0x%llX\n", info_array[i].name, info_array[i].base);
//                        }
//                    }
//                }
//                delete[] info_array;
//            }
//            return modules;
//        }
//
//        template <typename T>
//        std::vector<T> read_array(uintptr_t address, size_t count) {
//            std::vector<T> buffer(count);
//            const size_t size = count * sizeof(T);
//
//            if (!read_memory(address, buffer.data(), size)) {
//                return {};
//            }
//            return buffer;
//        }
//
//        template<typename ret_t = std::uintptr_t, typename addr_t>
//        ret_t read(addr_t address) {
//            ret_t data{};
//            if (!read_memory((uintptr_t)address, &data, sizeof(ret_t)))
//                return ret_t{};
//            return data;
//        }
//
//        template<typename data_t, typename addr_t>
//        bool write(addr_t address, data_t data) {
//            return this->write_memory((uintptr_t)address, &data, sizeof(data_t));
//        }
//
//        std::string read_w_string(const std::uint64_t& Address) {
//            if (!Address) return "";
//
//            std::uint64_t NewAddress = read<std::uint64_t>(Address);
//            if (!NewAddress) return "";
//
//            int Length = read<int>(NewAddress + 0x10);
//            if (Length <= 0 || Length > 2000) return ""; // Adding a safety cap
//
//            std::vector<wchar_t> Buffer(Length + 1, L'\0');
//            read_memory(uintptr_t(NewAddress + 0x14), Buffer.data(), Length * sizeof(wchar_t));
//
//            std::wstring TempString(Buffer.data(), Length);
//            for (auto& ch : TempString) {
//                if (ch < 32 || ch > 126) {
//                    ch = L'?';
//                }
//            }
//            return std::string(TempString.begin(), TempString.end());
//        }
//
//        void unload() {
//            rose::unload_driver();
//        }
//
//        std::uintptr_t get_module_export(std::uintptr_t module_base, const char* export_name) {
//            if (!module_base || !export_name) return 0;
//
//            IMAGE_DOS_HEADER dos_header;
//            if (!read_memory(module_base, &dos_header, sizeof(IMAGE_DOS_HEADER))) return 0;
//            if (dos_header.e_magic != IMAGE_DOS_SIGNATURE) return 0;
//
//            IMAGE_NT_HEADERS nt_headers;
//            if (!read_memory(module_base + dos_header.e_lfanew, &nt_headers, sizeof(IMAGE_NT_HEADERS))) return 0;
//            if (nt_headers.Signature != IMAGE_NT_SIGNATURE) return 0;
//
//            auto export_dir = &nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
//            if (!export_dir->VirtualAddress || !export_dir->Size) return 0;
//
//            IMAGE_EXPORT_DIRECTORY export_directory;
//            if (!read_memory(module_base + export_dir->VirtualAddress, &export_directory, sizeof(IMAGE_EXPORT_DIRECTORY))) return 0;
//
//            auto functions = read_array<DWORD>(module_base + export_directory.AddressOfFunctions, export_directory.NumberOfFunctions);
//            if (functions.empty()) return 0;
//
//            auto names = read_array<DWORD>(module_base + export_directory.AddressOfNames, export_directory.NumberOfNames);
//            if (names.empty()) return 0;
//
//            auto ordinals = read_array<WORD>(module_base + export_directory.AddressOfNameOrdinals, export_directory.NumberOfNames);
//            if (ordinals.empty()) return 0;
//
//            for (DWORD i = 0; i < export_directory.NumberOfNames; i++) {
//                char name_buffer[256] = { 0 };
//                if (!read_memory(module_base + names[i], name_buffer, sizeof(name_buffer) - 1)) continue;
//
//                if (strcmp(name_buffer, export_name) == 0) {
//                    WORD ordinal = ordinals[i];
//                    return module_base + functions[ordinal];
//                }
//            }
//            return 0;
//        }
//
//        // --- STUBBED FUNCTIONS --- 
//        // The new CR3-based driver does not parse physical linear translations or EPROCESS natively.
//        // Returning zeros or dummies to prevent compile breaks on the old cheat source.
//        std::uintptr_t get_dtb(std::uintptr_t base_address) { return m_dtb; }
//        std::uintptr_t get_base_address(eprocess_t* process) { return m_base_address; }
//        std::uintptr_t map_physical(std::uintptr_t address, std::uint32_t size) { return 0; }
//        std::pair<std::uintptr_t, std::size_t> translate_linear(std::uintptr_t address) { return { 0, 0 }; }
//        peb_t* get_peb(eprocess_t* process) { return nullptr; }
//        peb_t get_process_peb(eprocess_t* process) { return peb_t{}; }
//        eprocess_t* get_eprocess(std::uint32_t process_id) { return (eprocess_t*)1; }
//        void* open_process(std::uint32_t process_id) { return (void*)1; }
//        std::uintptr_t find_min(std::uint32_t g, std::size_t f) { return (((g) < (f)) ? (g) : (static_cast<std::uint32_t>(f))); }
//        std::uint32_t get_process_id(std::wstring module_name) {
//            char narrow_name[MAX_PATH];
//            size_t converted;
//            wcstombs_s(&converted, narrow_name, MAX_PATH, module_name.c_str(), _TRUNCATE);
//            return static_cast<std::uint32_t>(rose::get_pid_by_name(narrow_name));
//        }
//        std::vector<void*> get_process_threads() { return {}; }
//        void debug_last_exception() const { printf("Driver exceptions: 0x%x\n", m_last_error); }
//    };
//}
//
//inline auto Kernel = new n_driver::c_driver;