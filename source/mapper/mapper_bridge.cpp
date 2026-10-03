#include "includes/mapper_bridge.hpp"

#include "includes/intel_driver.hpp"
#include "includes/kdmapper.hpp"
#include "includes/loader_offsets.hpp"
#include "includes/pdb_offsets.hpp"

#include <vector>
#include <fstream>
#include <string>
#include <chrono>
#include <Windows.h>

namespace
{
    loader_offsets::block g_loader_offsets = {};
    ULONG64 g_remote_loader_offsets = 0;
    std::ofstream g_log;

    void open_log()
    {
        if (g_log.is_open()) return;
        char path[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string p(path);
        auto pos = p.find_last_of("\\/");
        if (pos != std::string::npos) p = p.substr(0, pos + 1);
        p += "mapper_log.txt";
        g_log.open(p, std::ios::out | std::ios::app);
    }

    void log_line(const std::string& msg)
    {
        open_log();
        SYSTEMTIME st{};
        GetLocalTime(&st);
        char ts[64];
        sprintf_s(ts, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
        if (g_log.is_open()) {
            g_log << ts << msg << std::endl;
            g_log.flush();
        }
    }

    bool mapper_callback(ULONG64* param1, ULONG64* param2, ULONG64 allocation_ptr, ULONG64 allocation_size)
    {
        log_line("callback: allocation_ptr=" + std::to_string(allocation_ptr) +
                 " size=" + std::to_string(allocation_size));
        g_loader_offsets.driver_size = allocation_size;

        g_remote_loader_offsets = intel_driver::AllocatePool(nt::POOL_TYPE::NonPagedPool, sizeof(g_loader_offsets));
        if (!g_remote_loader_offsets) {
            log_line("callback FAIL: AllocatePool returned 0");
            return false;
        }
        log_line("callback: remote_offsets=" + std::to_string(g_remote_loader_offsets));

        if (!intel_driver::WriteMemory(g_remote_loader_offsets, &g_loader_offsets, sizeof(g_loader_offsets))) {
            log_line("callback FAIL: WriteMemory failed");
            intel_driver::FreePool(g_remote_loader_offsets);
            g_remote_loader_offsets = 0;
            return false;
        }

        *param1 = allocation_ptr;
        *param2 = g_remote_loader_offsets;
        log_line("callback OK");
        return true;
    }

    void free_remote_offsets()
    {
        if (g_remote_loader_offsets) {
            intel_driver::FreePool(g_remote_loader_offsets);
            g_remote_loader_offsets = 0;
        }
    }
}

namespace mapper_bridge
{
    bool map_driver(const std::uint8_t* image, std::size_t image_size, NTSTATUS* exit_code)
    {
        open_log();
        log_line("=== map_driver start ===");
        log_line("image=" + std::to_string(reinterpret_cast<uintptr_t>(image)) +
                 " size=" + std::to_string(image_size));

        if (!image || !image_size) {
            log_line("FAIL: null image or zero size");
            return false;
        }

        g_loader_offsets = {};
        g_loader_offsets.magic = loader_offsets::magic;
        g_loader_offsets.version = loader_offsets::version;
        g_loader_offsets.size = sizeof(g_loader_offsets);

        log_line("resolving pdb offsets...");
        if (!pdb_offsets::resolve(&g_loader_offsets)) {
            log_line("FAIL: pdb_offsets::resolve returned false (kernel symbols)");
            return false;
        }
        log_line("pdb offsets OK");

        log_line("calling intel_driver::Load() NOW");
        {
            std::ofstream f2("C:\\mapper_debug.txt", std::ios::app);
            if (f2) f2 << "before Load\n";
        }
        const bool load_ok = intel_driver::Load();
        {
            std::ofstream f2("C:\\mapper_debug.txt", std::ios::app);
            if (f2) f2 << "after Load ok=" << (load_ok ? 1 : 0) << "\n";
        }
        log_line(std::string("intel_driver::Load returned ") + (load_ok ? "TRUE" : "FALSE"));
        if (!load_ok) {
            log_line("FAIL: intel_driver::Load returned false — see lines above from intel (Load enter / NtLoadDriver Status)");
            return false;
        }
        log_line("intel_driver::Load OK");

        std::vector<BYTE> raw_image(image, image + image_size);
        NTSTATUS local_exit_code = 0;

        log_line("calling kdmapper::MapDriver...");
        const ULONG64 mapped_base = kdmapper::MapDriver(
            raw_image.data(),
            0,
            0,
            false,
            false,
            false,
            kdmapper::AllocationMode::AllocateContiguousMemory,
            true,
            true,
            mapper_callback,
            &local_exit_code
        );

        char buf[128];
        sprintf_s(buf, "MapDriver result: base=0x%llX exit=0x%08X",
                  (unsigned long long)mapped_base, (unsigned)local_exit_code);
        log_line(buf);

        free_remote_offsets();
        const bool unloaded = intel_driver::Unload();
        log_line(std::string("intel_driver::Unload ") + (unloaded ? "OK" : "FAIL"));

        if (exit_code)
            *exit_code = local_exit_code;

        const bool ok = mapped_base != 0 && NT_SUCCESS(local_exit_code);
        log_line(ok ? "=== map_driver SUCCESS ===" : "=== map_driver FAILED ===");
        return ok;
    }
}
