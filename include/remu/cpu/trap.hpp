#pragma once

#include <remu/cpu/cpu.hpp>

namespace remu::cpu {

// Returns true if a trap was taken and PC was modified
bool check_and_take_interrupt(Cpu& cpu);

// Returns true if a pending exception was taken and PC changed.
bool take_pending_exception(Cpu& cpu);

}  // namespace remu::cpu
