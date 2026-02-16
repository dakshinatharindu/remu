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
constexpr std::uint16_t CSR_MVENDORID= 0xF11;
constexpr std::uint16_t CSR_MARCHID  = 0xF12;
constexpr std::uint16_t CSR_MIMPID   = 0xF13;

// Supervisor CSRs
constexpr std::uint16_t CSR_SSTATUS = 0x100;
constexpr std::uint16_t CSR_SIE     = 0x104;
constexpr std::uint16_t CSR_STVEC   = 0x105;
constexpr std::uint16_t CSR_SEPC    = 0x141;
constexpr std::uint16_t CSR_SCAUSE  = 0x142;
constexpr std::uint16_t CSR_STVAL   = 0x143;
constexpr std::uint16_t CSR_SIP     = 0x144;

constexpr std::uint16_t CSR_MEDELEG = 0x302;
constexpr std::uint16_t CSR_MIDELEG = 0x303;

constexpr std::uint16_t CSR_MCYCLE   = 0xB00;
constexpr std::uint16_t CSR_MINSTRET = 0xB02;

constexpr std::uint32_t SSTATUS_MASK = (1u<<1) | (1u<<5) | (1u<<8);
constexpr std::uint32_t SIE_MASK     = (1u<<1) | (1u<<5) | (1u<<9);
constexpr std::uint32_t SIP_MASK     = (1u<<1) | (1u<<5) | (1u<<9);


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
    mvendorid_= 0;
    marchid_  = 0;
    mimpid_   = 0;

    stvec_    = 0;
    sepc_     = 0;
    scause_   = 0;
    stval_    = 0;
    medeleg_  = 0;
    mideleg_  = 0;
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
        case CSR_MVENDORID:out = mvendorid_; return true;
        case CSR_MARCHID:  out = marchid_; return true;
        case CSR_MIMPID:   out = mimpid_;  return true;
        case CSR_MCYCLE:   out = static_cast<std::uint32_t>(mcycle_ & 0xFFFF'FFFFull); return true;
        case CSR_MINSTRET: out = static_cast<std::uint32_t>(minstret_ & 0xFFFF'FFFFull); return true;

        case CSR_SSTATUS: out = (mstatus_ & SSTATUS_MASK); return true;
        case CSR_SIE:     out = (mie_ & SIE_MASK);         return true;
        case CSR_SIP:     out = (mip_ & SIP_MASK);         return true;

        case CSR_STVEC:   out = stvec_;   return true;
        case CSR_SEPC:    out = sepc_;    return true;
        case CSR_SCAUSE:  out = scause_;  return true;
        case CSR_STVAL:   out = stval_;   return true;

        case CSR_MEDELEG: out = medeleg_; return true;
        case CSR_MIDELEG: out = mideleg_; return true;

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

        case CSR_MVENDORID:
            mvendorid_ = value;
            return true;

        case CSR_MARCHID:
            marchid_ = value;
            return true;

        case CSR_MIMPID:
            mimpid_ = value;
            return true;

        case CSR_MCYCLE:
            mcycle_ = (mcycle_ & 0xFFFF'FFFF'0000'0000ull) | value;
            return true;

        case CSR_MINSTRET:
            minstret_ = (minstret_ & 0xFFFF'FFFF'0000'0000ull) | value;
            return true;
        
        case CSR_SSTATUS:
            // Only allow supervisor bits to change
            mstatus_ = (mstatus_ & ~SSTATUS_MASK) | (value & SSTATUS_MASK);
            return true;

        case CSR_SIE:
            mie_ = (mie_ & ~SIE_MASK) | (value & SIE_MASK);
            return true;

        case CSR_SIP:
            // In real hardware many SIP bits are read-only.
            // For minimal model, allow writing only the masked bits.
            mip_ = (mip_ & ~SIP_MASK) | (value & SIP_MASK);
            return true;

        case CSR_STVEC:  stvec_  = value; return true;
        case CSR_SEPC:   sepc_   = value; return true;
        case CSR_SCAUSE: scause_ = value; return true;
        case CSR_STVAL:  stval_  = value; return true;

        case CSR_MEDELEG: medeleg_ = value; return true;
        case CSR_MIDELEG: mideleg_ = value; return true;

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
