#pragma once
#include <cstdint>
#include <string>

namespace remu::runtime {

struct Arguments {
    std::string kernel_path;     // from -k
    std::uint64_t mem_size_bytes = 128ull * 1024 * 1024; // default 128 MiB
};

} // namespace remu::runtime
