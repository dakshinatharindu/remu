#include <remu/cpu/trap.hpp>

namespace remu::cpu {

namespace {

constexpr std::uint32_t MSTATUS_MIE   = 1u << 3;
constexpr std::uint32_t MSTATUS_MPIE  = 1u << 7;
constexpr std::uint32_t MSTATUS_MPP_MASK = 3u << 11;

constexpr std::uint32_t MIE_MTIE = 1u << 7;
constexpr std::uint32_t MIP_MTIP = 1u << 7;

constexpr std::uint32_t MCAUSE_MTI = 0x80000007u;

} // namespace

bool check_and_take_interrupt(Cpu& cpu)
{
    const std::uint32_t mstatus = cpu.csr.mstatus();
    const std::uint32_t mie     = cpu.csr.mie();
    const std::uint32_t mip     = cpu.csr.mip();

    const bool take_mti =
        (mstatus & MSTATUS_MIE) &&
        (mie     & MIE_MTIE) &&
        (mip     & MIP_MTIP);

    if (!take_mti)
        return false;

    // --- Trap entry ---

    cpu.csr.set_mepc(cpu.pc);
    cpu.csr.set_mcause(MCAUSE_MTI);
    cpu.csr.set_mtval(0);

    std::uint32_t new_ms = mstatus;

    const bool prev_mie = (new_ms & MSTATUS_MIE) != 0;

    // MPIE <- MIE
    if (prev_mie)
        new_ms |= MSTATUS_MPIE;
    else
        new_ms &= ~MSTATUS_MPIE;

    // MIE <- 0
    new_ms &= ~MSTATUS_MIE;

    // MPP <- current privilege (Machine = 3)
    new_ms = (new_ms & ~MSTATUS_MPP_MASK) | (3u << 11);

    cpu.csr.set_mstatus(new_ms);
    cpu.priv = PrivMode::Machine;

    // Direct mode only (mtvec[1:0] = 0)
    cpu.pc = cpu.csr.mtvec() & ~0x3u;

    return true;
}

}
