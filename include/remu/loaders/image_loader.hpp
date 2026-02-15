#pragma once

#include <cstdint>
#include <remu/common/result.hpp>  // if you have it; otherwise you can return bool + error string
#include <remu/mem/bus.hpp>  // your bus interface
#include <remu/mem/memory.hpp>
#include <span>
#include <string>
#include <vector>

namespace remu::loaders {

// Read directly from a file into guest memory (optional convenience, can be done by caller).
remu::common::Result<long> load_file_into_guest(remu::mem::Memory& ram,
                                               const std::string& path);

}  // namespace remu::loaders
