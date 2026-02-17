#include <remu/cpu/trap.hpp>

namespace remu::cpu {

namespace {

// mstatus bits
constexpr std::uint32_t MSTATUS_MIE      = 1u << 3;
constexpr std::uint32_t MSTATUS_MPIE     = 1u << 7;
constexpr std::uint32_t MSTATUS_MPP_MASK = 3u << 11;

// mie/mip bits
constexpr std::uint32_t MIE_MTIE = 1u << 7;
constexpr std::uint32_t MIP_MTIP = 1u << 7;

// mcause for machine timer interrupt: interrupt bit + code 7
constexpr std::uint32_t MCAUSE_MTI = 0x80000007u;

// Trap entry common
inline void enter_trap_machine(Cpu& cpu, std::uint32_t mcause, std::uint32_t mtval) {
    // mepc points to faulting/trapping instruction address
    cpu.csr.set_mepc(cpu.pc);
    cpu.csr.set_mcause(mcause);
    cpu.csr.set_mtval(mtval);

    // Update mstatus: MPIE <- MIE, MIE <- 0, MPP <- current priv
    std::uint32_t ms = cpu.csr.mstatus();

    const bool prev_mie = (ms & MSTATUS_MIE) != 0;
    if (prev_mie) ms |= MSTATUS_MPIE;
    else          ms &= ~MSTATUS_MPIE;

    ms &= ~MSTATUS_MIE;

    // Set MPP = current priv (we only model Machine well right now)
    // Machine=3, Supervisor=1, User=0
    std::uint32_t mpp = 3u;
    if (cpu.priv == PrivMode::Supervisor) mpp = 1u;
    else if (cpu.priv == PrivMode::User) mpp = 0u;

    ms = (ms & ~MSTATUS_MPP_MASK) | (mpp << 11);
    cpu.csr.set_mstatus(ms);

    // Enter machine mode
    cpu.priv = PrivMode::Machine;

    // mtvec direct mode only
    cpu.pc = cpu.csr.mtvec() & ~0x3u;
}

} // namespace

bool check_and_take_interrupt(Cpu& cpu) {
    const std::uint32_t ms = cpu.csr.mstatus();
    const std::uint32_t mie = cpu.csr.mie();
    const std::uint32_t mip = cpu.csr.mip();

    const bool take_mti =
        (ms  & MSTATUS_MIE) &&
        (mie & MIE_MTIE) &&
        (mip & MIP_MTIP);

    if (!take_mti) return false;

    enter_trap_machine(cpu, MCAUSE_MTI, 0);
    return true;
}

bool take_pending_exception(Cpu& cpu) {
    if (!cpu.exception_pending) return false;

    // exception mcause has interrupt bit = 0 already
    const std::uint32_t cause = cpu.exception_cause;
    const std::uint32_t tval  = cpu.exception_tval;

    cpu.clear_pending_exception();
    enter_trap_machine(cpu, cause, tval);
    return true;
}

} // namespace rvemu::cpu
