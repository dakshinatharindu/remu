#pragma once
#include <cstdint>

namespace remu::cpu {

// Synchronous exception causes (mcause, interrupt bit = 0)
namespace exc {
constexpr std::uint32_t Breakpoint = 3;
constexpr std::uint32_t EcallFromU = 8;
constexpr std::uint32_t EcallFromS = 9;
constexpr std::uint32_t EcallFromM = 11;
}  // namespace exc

}  // namespace remu::cpu
