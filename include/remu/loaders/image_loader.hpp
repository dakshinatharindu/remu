#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <remu/common/result.hpp>   // if you have it; otherwise you can return bool + error string
#include <remu/mem/bus.hpp>         // your bus interface

namespace remu::loaders {

// Read an entire file into memory (host-side bytes).
remu::common::Result<std::vector<std::uint8_t>>
read_file_bytes(const std::string& path);

// Copy bytes into guest memory at a given physical address.
remu::common::Result<void>
load_blob(remu::mem::Bus& bus,
          std::uint32_t guest_paddr,
          std::span<const std::uint8_t> bytes);

} // namespace r    emu::loaders
