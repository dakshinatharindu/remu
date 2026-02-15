#pragma once
#include <remu/runtime/arguments.hpp>

namespace remu::runtime {

// High-level entry point for the emulator core.
// Returns 0 on success, non-zero on failure.
int run(const Arguments& args);

}  // namespace remu::runtime
