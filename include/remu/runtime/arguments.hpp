#pragma once
#include <cstdint>
#include <string>

namespace remu::runtime {

struct Arguments {
    std::string kernel_path;     // from -k
    std::uint64_t mem_size_bytes = 128ull * 1024 * 1024; // default 128 MiB
    std::string dtb_path = "resources/dtb/mini.dtb"; // optional, matches the hardcoded remu memmap
    // Instructions retired per mtime tick (from -t). The DTB timebase is
    // 1 MHz, so this is effectively the modelled CPU speed in MIPS.
    std::uint32_t insns_per_mtime_tick = 1;
};

} // namespace remu::runtime
