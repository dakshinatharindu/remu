#include <remu/runtime/sim.hpp>

#include <remu/common/log.hpp>
#include <remu/cpu/decode.hpp>
#include <remu/cpu/execute.hpp>
#include <remu/mem/bus.hpp>
#include <remu/cpu/trap.hpp>
#include <remu/cpu/exec_result.hpp>

namespace remu::runtime {

Sim::Sim(remu::platform::VirtMachine& machine,
         remu::cpu::Cpu& cpu,
         const Arguments& opts)
    : machine_(machine), cpu_(cpu), opts_(opts) {}

bool Sim::fetch32_(std::uint32_t addr, std::uint32_t& out) {
    return machine_.bus().read32(addr, out);
}

namespace {
constexpr std::uint32_t MSTATUS_MIE   = 1u << 3;
constexpr std::uint32_t MSTATUS_MPIE  = 1u << 7;
constexpr std::uint32_t MSTATUS_MPP_MASK = 3u << 11;

constexpr std::uint32_t MIE_MTIE = 1u << 7;
constexpr std::uint32_t MIP_MTIP = 1u << 7;

constexpr std::uint32_t MCAUSE_MTI = 0x80000007u; // interrupt + machine timer
}

bool Sim::step() {
    if (stop_reason_ != StopReason::None) return false;

    // Still tick time forward
    machine_.tick(1, cpu_);
    cpu_.csr.increment_cycle(1);

    // Check for interrupts (trap handling)
    if (remu::cpu::check_and_take_interrupt(cpu_)) {
        return true; // took an interrupt, new PC is set, continue execution
    }

    const std::uint32_t pc = cpu_.pc;

    // 1) Fetch
    std::uint32_t insn = 0;
    if (!fetch32_(pc, insn)) {
        stop_reason_ = StopReason::BusFaultFetch;
        return false;
    }

    // 2) Decode
    const remu::cpu::DecodedInsn d = remu::cpu::decode_rv32(insn);
    if (d.kind == remu::cpu::InsnKind::Illegal) {
        stop_reason_ = StopReason::IllegalInstruction;
        return false;
    }

    // Optional trace hook
#ifdef REMU_ENABLE_TRACE
    remu::common::log_debug("pc=0x" + std::to_string(pc) + " insn=0x" + std::to_string(insn));
#endif

    // 3) Execute
    auto ok = remu::cpu::execute(d, cpu_, machine_.bus());
    if (ok == remu::cpu::ExecResult::Fault) {
        stop_reason_ = StopReason::ExecuteFailed;
        return false;
    }

    // 4) Accounting / ticks
    instructions_ += 1;
    cpu_.csr.increment_instret(1);
    
    if (ok == remu::cpu::ExecResult::TrapRaised) {
        remu::cpu::take_pending_exception(cpu_);
        return true;
    }

    // if (ok == remu::cpu::ExecResult::Wfi) {
    //     // If already pending, no need to idle.
    //     // Otherwise tick until something becomes pending.
    //     // Keep it bounded to avoid infinite loops if interrupts never come.
    //     constexpr std::uint64_t kMaxIdleTicks = 10'000'000;

    //     std::uint64_t idle = 0;
    //     while (idle < kMaxIdleTicks) {
    //         // Let time pass
    //         machine_.tick(10000, cpu_);
    //         cpu_.csr.increment_cycle(10000);

    //         // If an interrupt is now pending+enabled and global MIE set,
    //         // the next check will take it.
    //         if (remu::cpu::check_and_take_interrupt(cpu_)) {
    //             return true;
    //         }
    //         idle++;
    //     }

    //     // If we get here, nothing woke us up: treat as stop/fault (or just return true).
    //     stop_reason_ = StopReason::InstructionLimit; // or new StopReason::WfiStuck
    //     return false;
    // }

    return true;
}

RunResult Sim::run(std::uint64_t max_instructions) {
    stop_reason_ = StopReason::None;
    instructions_ = 0;

    // Use override if provided, else default to "no limit" for now.
    const std::uint64_t limit = (max_instructions != 0) ? max_instructions : 0;

    while (true) {
        if (limit != 0 && instructions_ >= limit) {
            stop_reason_ = StopReason::InstructionLimit;
            break;
        }
        if (!step()) break;
    }

    RunResult rr;
    rr.reason = stop_reason_;
    rr.instructions = instructions_;
    rr.last_pc = cpu_.pc;
    return rr;
}

} // namespace remu::runtime
