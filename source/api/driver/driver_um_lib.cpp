#include "driver_um_lib.hpp"
#include <winnt.h>
#include <winternl.h>
#include <windows.h>      
// The IOCTL code that User-Mode will use to talk to Kernel-Mode
#define IOCTL_INJECT_MOUSE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Call types (optional for standalone, but keeps it consistent with your base)
typedef enum _command_call_type {
    cmd_inject_mouse = 12,
} command_call_type;

// The payload structure
typedef struct _cmd_inject_mouse_t {
    long x;
    long y;
    unsigned short button_flags;
} cmd_inject_mouse_t;
// Added '' to globals to prevent multiple definitions
extern "C" NtUserGetCPD_type NtUserGetCPD = nullptr;
extern "C" Win11HookFunc_type Win11HookFunc = nullptr;

// Restores from a nmi
extern "C" void nmi_restoring(trap_frame_t* trap_frame) {
    uint64_t* stack_ptr = (uint64_t*)trap_frame->rsp;

    while (true) {
        if (*stack_ptr != stack_id) {
            stack_ptr++; // Move up the stack
            continue;
        }

        trap_frame->rax = nmi_occured;

        // Restore rsp
        trap_frame->rsp = (uint64_t)stack_ptr + 0x8; // Point top of rsp to a ret address

        // Return to the send request
        return;
    }
}


// Fallback if MASM unit is not linked
extern "C" Win11HookFunc_type Win11HookFunc;
extern "C" __int64 __fastcall asm_call_driver_win11(uint64_t a1, uint64_t a2, uint64_t nmi_restoring_addr) {
    (void)nmi_restoring_addr;
    if (!Win11HookFunc) return 0;
    return Win11HookFunc(a1, a2);
}

namespace rose {

    // --- Standalone Mouse Driver Handle ---
    HANDLE hMouseDevice = INVALID_HANDLE_VALUE;

    __int64 send_request(void* cmd) {
        __int64 ret;

        if (use_win11_hook) {
            ret = asm_call_driver_win11((uint64_t)cmd, (uint64_t)caller_signature, (uint64_t)asm_nmi_restoring);
        }
        else {
            ret = asm_call_driver((uint64_t)cmd, caller_signature, (uint64_t)asm_nmi_restoring);
        }

        if (ret == nmi_occured) {
            return send_request(cmd);
        }

        return ret;
    }

    bool copy_virtual_memory(uint64_t src_cr3, uint64_t dst_cr3, void* src, void* dst, uint64_t size) {
        if (!inited || !is_hook_ready())
            return false;

        copy_virtual_memory_t copy_mem_cmd = { 0 };
        copy_mem_cmd.src_cr3 = src_cr3;
        copy_mem_cmd.dst_cr3 = dst_cr3;
        copy_mem_cmd.src = src;
        copy_mem_cmd.dst = dst;
        copy_mem_cmd.size = size;

        command_t cmd = { 0 };
        cmd.call_type = cmd_copy_virtual_memory;
        cmd.sub_command_ptr = &copy_mem_cmd;

        send_request(&cmd);

        return cmd.status;
    }

    uint64_t get_cr3(uint64_t pid) {
        if (!inited || !is_hook_ready())
            return 0;

        get_cr3_t get_cr3_cmd = { 0 };
        get_cr3_cmd.pid = pid;

        command_t cmd = { 0 };
        cmd.call_type = cmd_get_cr3;
        cmd.sub_command_ptr = &get_cr3_cmd;

        send_request(&cmd);

        return get_cr3_cmd.cr3;
    }

    uint64_t get_module_base(const char* module_name, uint64_t pid) {
        if (!inited || !is_hook_ready())
            return 0;

        get_module_base_t get_module_base_cmd = { 0 };
        strncpy(get_module_base_cmd.module_name, module_name, MAX_PATH - 1);
        get_module_base_cmd.pid = pid;

        command_t cmd = { 0 };
        cmd.call_type = cmd_get_module_base;
        cmd.sub_command_ptr = &get_module_base_cmd;

        send_request(&cmd);

        return get_module_base_cmd.module_base;
    }

    uint64_t get_module_size(const char* module_name, uint64_t pid) {
        if (!inited || !is_hook_ready())
            return 0;

        get_module_size_t get_module_size_cmd = { 0 };
        strncpy(get_module_size_cmd.module_name, module_name, MAX_PATH - 1);
        get_module_size_cmd.pid = pid;

        command_t cmd = { 0 };
        cmd.call_type = cmd_get_module_size;
        cmd.sub_command_ptr = &get_module_size_cmd;

        send_request(&cmd);

        return get_module_size_cmd.module_size;
    }

    uint64_t get_pid_by_name(const char* name) {
        if (!inited || !is_hook_ready())
            return 0;

        get_pid_by_name_t get_pid_by_name_cmd = { 0 };
        strncpy(get_pid_by_name_cmd.name, name, MAX_PATH - 1);

        command_t cmd = { 0 };
        cmd.call_type = cmd_get_pid_by_name;
        cmd.sub_command_ptr = &get_pid_by_name_cmd;

        send_request(&cmd);

        return get_pid_by_name_cmd.pid;
    }

    uint64_t get_ldr_data_table_entry_count(uint64_t pid) {
        if (!inited || !is_hook_ready())
            return 0;

        get_ldr_data_table_entry_count_t get_ldr_data_table_entry = { 0 };
        get_ldr_data_table_entry.pid = pid;

        command_t cmd = { 0 };
        cmd.call_type = cmd_get_ldr_data_table_entry_count;
        cmd.sub_command_ptr = &get_ldr_data_table_entry;

        send_request(&cmd);

        return get_ldr_data_table_entry.count;
    }

    bool get_data_table_entry_info(uint64_t pid, module_info_t* info_array) {
        if (!inited || !is_hook_ready())
            return false;

        cmd_get_data_table_entry_info_t get_module_at_index = { 0 };
        get_module_at_index.pid = pid;
        get_module_at_index.info_array = info_array;

        command_t cmd = { 0 };
        cmd.call_type = cmd_get_data_table_entry_info;
        cmd.sub_command_ptr = &get_module_at_index;

        send_request(&cmd);

        return cmd.status;
    }

    bool hide_driver(void) {
        if (!inited || !is_hook_ready())
            return false;

        command_t cmd = { 0 };
        cmd.call_type = cmd_remove_from_system_page_tables;

        send_request(&cmd);

        return cmd.status;
    }

    bool unload_driver(void) {
        if (!inited || !is_hook_ready()) {
            if (!rose::init_roseware_lib())
                return false;
        }

        command_t cmd = { 0 };
        cmd.call_type = cmd_unload_driver;

        send_request(&cmd);

        return cmd.status;
    }

    bool ping_driver(void) {
        if (!inited || !is_hook_ready())
            return false;

        command_t cmd = { 0 };
        cmd.call_type = cmd_ping_driver;

        send_request(&cmd);

        return cmd.status;
    }

    void flush_logs(void) {
        if (!inited || !is_hook_ready())
            return;

        log_entry_t* log_array = new log_entry_t[512];
        memset(log_array, 0, sizeof(log_entry_t) * 512);

        cmd_output_logs_t sub_cmd;
        sub_cmd.count = 512;
        sub_cmd.log_array = log_array;

        command_t cmd = { 0 };
        cmd.call_type = cmd_output_logs;
        cmd.sub_command_ptr = &sub_cmd;

        send_request(&cmd);
        if (!cmd.status) {

            return;
        }

        if (!log_array[0].present) {
            delete[] log_array;
            return;
        }


        for (uint32_t i = 0; i < 512; i++) {
            if (!log_array[i].present)
                continue;

            
        }


        delete[] log_array;
    }

    // --- NEW MOUSE IMPLEMENTATION ---

    bool init_mouse() {
        if (hMouseDevice != INVALID_HANDLE_VALUE) {
            return true; // Already initialized
        }

        hMouseDevice = CreateFileA("\\\\.\\MouseMemeDev", GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, 0, NULL);

        if (hMouseDevice == INVALID_HANDLE_VALUE) {
            return false;
        }
        return true;
    }

    bool inject_mouse(long x, long y, unsigned short button_flags) {
        // Auto-initialize if you forgot to call init_mouse()
        if (hMouseDevice == INVALID_HANDLE_VALUE) {
            if (!init_mouse()) return false;
        }

        cmd_inject_mouse_t cmd = { 0 };
        cmd.x = x;
        cmd.y = y;
        cmd.button_flags = button_flags;

        DWORD bytesReturned;
        return DeviceIoControl(hMouseDevice, IOCTL_INJECT_MOUSE,
            &cmd, sizeof(cmd),
            &cmd, sizeof(cmd),
            &bytesReturned, NULL);
    }
};

