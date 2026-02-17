#pragma once

#include <remu/cpu/cpu.hpp>

namespace remu::cpu {

// Returns true if a trap was taken and PC was modified
bool check_and_take_interrupt(Cpu& cpu);

}  // namespace remu::cpu
