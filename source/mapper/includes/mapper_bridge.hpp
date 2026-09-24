#pragma once

#include <Windows.h>
#include <cstddef>
#include <cstdint>

namespace mapper_bridge
{
    bool map_driver(const std::uint8_t* image, std::size_t image_size, NTSTATUS* exit_code = nullptr);
}
