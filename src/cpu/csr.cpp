#include <remu/cpu/csr.hpp>

namespace remu::cpu {

namespace {
// CSR addresses we care about first (RV privileged spec)
constexpr std::uint16_t CSR_MSTATUS  = 0x300;
constexpr std::uint16_t CSR_MISA     = 0x301;
constexpr std::uint16_t CSR_MTVEC    = 0x305;
constexpr std::uint16_t CSR_MSCRATCH = 0x340;
constexpr std::uint16_t CSR_MEPC     = 0x341;
constexpr std::uint16_t CSR_MCAUSE   = 0x342;
constexpr std::uint16_t CSR_MTVAL    = 0x343;
constexpr std::uint16_t CSR_MIP      = 0x344;
constexpr std::uint16_t CSR_MIE      = 0x304;
constexpr std::uint16_t CSR_PMPCFG0  = 0x3A0;
constexpr std::uint16_t CSR_PMPADDR0 = 0x3B0;
constexpr std::uint16_t CSR_MHARTID  = 0xF14;

constexpr std::uint16_t CSR_MCYCLE   = 0xB00;
constexpr std::uint16_t CSR_MINSTRET = 0xB02;

// For RV32, cycle/minstret low halves are enough to start.
// (Linux might read time via CLINT rather than cycle; still useful.)
} // namespace

CsrFile::CsrFile() {
    reset();
}

void CsrFile::reset() {
    mstatus_  = 0;
    misa_     = build_misa_rv32ima_();
    mtvec_    = 0;
    mscratch_ = 0;
    mepc_     = 0;
    mcause_   = 0;
    mtval_    = 0;
    mie_      = 0;
    mip_      = 0;
    pmpcfg0_  = 0;
    pmpaddr0_ = 0;
    mcycle_   = 0;
    minstret_ = 0;
    mhartid_  = 0;
}

std::uint32_t CsrFile::build_misa_rv32ima_() {
    // misa layout:
    // - For RV32, MXL is in bits [31:30] and should be 1 (0b01) for RV32.
    // - Extension bits: A=0, B=1, ... I=8, M=12
    constexpr std::uint32_t MXL_RV32 = (1u << 30);

    auto ext_bit = [](char c) -> std::uint32_t {
        if (c < 'A' || c > 'Z') return 0;
        return 1u << (static_cast<std::uint32_t>(c - 'A'));
    };

    std::uint32_t ext = 0;
    ext |= ext_bit('I');
    ext |= ext_bit('M');
    ext |= ext_bit('A');
    // If you later add C, F, D, etc, set bits here.

    return MXL_RV32 | ext;
}

bool CsrFile::read(std::uint16_t csr_addr, std::uint32_t& out) const {
    switch (csr_addr) {
        case CSR_MSTATUS:  out = mstatus_; return true;
        case CSR_MISA:     out = misa_;    return true;
        case CSR_MTVEC:    out = mtvec_;   return true;
        case CSR_MSCRATCH: out = mscratch_;return true;
        case CSR_MEPC:     out = mepc_;    return true;
        case CSR_MCAUSE:   out = mcause_;  return true;
        case CSR_MTVAL:    out = mtval_;   return true;
        case CSR_MIE:      out = mie_;     return true;
        case CSR_MIP:      out = mip_;     return true;
        case CSR_PMPCFG0:  out = pmpcfg0_; return true;
        case CSR_PMPADDR0: out = pmpaddr0_;return true;
        case CSR_MHARTID:  out = mhartid_; return true;
        case CSR_MCYCLE:   out = static_cast<std::uint32_t>(mcycle_ & 0xFFFF'FFFFull); return true;
        case CSR_MINSTRET: out = static_cast<std::uint32_t>(minstret_ & 0xFFFF'FFFFull); return true;

        default:
            return false; // unimplemented CSR for now
    }
}

bool CsrFile::write(std::uint16_t csr_addr, std::uint32_t value) {
    switch (csr_addr) {
        case CSR_MSTATUS:
            mstatus_ = value;
            return true;

        case CSR_MISA:
            // misa is usually read-only for most implementations.
            // For now: ignore writes (return true so SW doesn't crash),
            // or return false if you prefer strictness.
            return true;

        case CSR_MTVEC:
            mtvec_ = value;
            return true;

        case CSR_MSCRATCH:
            mscratch_ = value;
            return true;

        case CSR_MEPC:
            mepc_ = value;
            return true;

        case CSR_MCAUSE:
            mcause_ = value;
            return true;

        case CSR_MTVAL:
            mtval_ = value;
            return true;

        case CSR_MIE:
            mie_ = value;
            return true;

        case CSR_MIP:
            // Typically some bits are writable, some are read-only (set by hardware).
            // For now allow writing; you can refine later.
            mip_ = value;
            return true;
        
        case CSR_PMPCFG0:
            pmpcfg0_ = value;
            return true;
        
        case CSR_PMPADDR0:
            pmpaddr0_ = value;
            return true;
        
        case CSR_MHARTID:
            mhartid_ = value;
            return true;

        case CSR_MCYCLE:
            mcycle_ = (mcycle_ & 0xFFFF'FFFF'0000'0000ull) | value;
            return true;

        case CSR_MINSTRET:
            minstret_ = (minstret_ & 0xFFFF'FFFF'0000'0000ull) | value;
            return true;

        default:
            return false;
    }
}

void CsrFile::increment_cycle(std::uint64_t delta) {
    mcycle_ += delta;
}

void CsrFile::increment_instret(std::uint64_t delta) {
    minstret_ += delta;
}

} // namespace remu::cpu
