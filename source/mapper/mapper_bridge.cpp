#include "includes/mapper_bridge.hpp"

#include "includes/intel_driver.hpp"
#include "includes/kdmapper.hpp"
#include "includes/loader_offsets.hpp"
#include "includes/pdb_offsets.hpp"

#include <vector>

namespace
{
    loader_offsets::block g_loader_offsets = {};
    ULONG64 g_remote_loader_offsets = 0;

    bool mapper_callback(ULONG64* param1, ULONG64* param2, ULONG64 allocation_ptr, ULONG64 allocation_size)
    {
        g_loader_offsets.driver_size = allocation_size;

        g_remote_loader_offsets = intel_driver::AllocatePool(nt::POOL_TYPE::NonPagedPool, sizeof(g_loader_offsets));
        if (!g_remote_loader_offsets)
            return false;

        if (!intel_driver::WriteMemory(g_remote_loader_offsets, &g_loader_offsets, sizeof(g_loader_offsets))) {
            intel_driver::FreePool(g_remote_loader_offsets);
            g_remote_loader_offsets = 0;
            return false;
        }

        *param1 = allocation_ptr;
        *param2 = g_remote_loader_offsets;
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
        if (!image || !image_size)
            return false;

        g_loader_offsets = {};
        g_loader_offsets.magic = loader_offsets::magic;
        g_loader_offsets.version = loader_offsets::version;
        g_loader_offsets.size = sizeof(g_loader_offsets);

        if (!pdb_offsets::resolve(&g_loader_offsets))
            return false;

        if (!intel_driver::Load())
            return false;

        std::vector<BYTE> raw_image(image, image + image_size);
        NTSTATUS local_exit_code = 0;

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

        free_remote_offsets();
        const bool unloaded = intel_driver::Unload();
        (void)unloaded;

        if (exit_code)
            *exit_code = local_exit_code;

        return mapped_base != 0 && NT_SUCCESS(local_exit_code);
    }
}
