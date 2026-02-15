#pragma once

#include <cstdint>

#include <remu/cpu/cpu.hpp>
#include <remu/platform/virt.hpp>
#include <remu/runtime/arguments.hpp>

namespace remu::runtime {

enum class StopReason : std::uint8_t {
    None = 0,
    InstructionLimit,
    BusFaultFetch,
    IllegalInstruction,
    ExecuteFailed,
    EcallOrEbreak,
};

struct RunResult {
    StopReason reason = StopReason::None;
    std::uint64_t instructions = 0;
    std::uint32_t last_pc = 0;
};

// Simple interpreter simulator.
// Owns nothing: it operates on a machine + cpu provided by caller.
class Sim {
public:
    Sim(remu::platform::VirtMachine& machine,
        remu::cpu::Cpu& cpu,
        const Arguments& opts);

    // Execute one instruction. Returns false if stopped.
    bool step();

    // Run until stop condition. Optional max_instructions override:
    // if max_instructions == 0, no limit (be careful).
    RunResult run(std::uint64_t max_instructions = 0);

    StopReason stop_reason() const { return stop_reason_; }
    std::uint64_t instructions() const { return instructions_; }

private:
    bool fetch32_(std::uint32_t addr, std::uint32_t& out);

private:
    remu::platform::VirtMachine& machine_;
    remu::cpu::Cpu& cpu_;
    const Arguments& opts_;

    StopReason stop_reason_ = StopReason::None;
    std::uint64_t instructions_ = 0;
};

} // namespace remu::runtime
