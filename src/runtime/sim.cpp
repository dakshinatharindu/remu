#include <remu/runtime/sim.hpp>

#include <remu/common/log.hpp>
#include <remu/cpu/decode.hpp>
#include <remu/cpu/execute.hpp>
#include <remu/mem/bus.hpp>
#include <remu/cpu/trap.hpp>

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
    const bool ok = remu::cpu::execute(d, cpu_, machine_.bus());
    if (!ok) {
        // In our execute_rv32i(), we currently return false for ECALL/EBREAK too.
        // You can refine later by adding trap handling.
        if (d.kind == remu::cpu::InsnKind::ECALL || d.kind == remu::cpu::InsnKind::EBREAK) {
            stop_reason_ = StopReason::EcallOrEbreak;
        } else {
            stop_reason_ = StopReason::ExecuteFailed;
        }
        return false;
    }

    // 4) Accounting / ticks
    instructions_ += 1;
    cpu_.csr.increment_instret(1);
    
    if (remu::cpu::take_pending_exception(cpu_)) {
        return true;
    }

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
