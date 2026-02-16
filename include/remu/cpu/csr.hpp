#pragma once

#include <cstdint>

namespace remu::cpu {

// Privilege modes (minimal for now)
enum class PrivMode : std::uint8_t {
    User = 0,
    Supervisor = 1,
    Machine = 3,
};

// Minimal CSR file for RV32 (M-mode first)
class CsrFile {
public:
    CsrFile();

    void reset();

    // Read/write by CSR address (12-bit)
    bool read(std::uint16_t csr_addr, std::uint32_t& out) const;
    bool write(std::uint16_t csr_addr, std::uint32_t value);

    // Direct accessors (handy for trap logic later)
    std::uint32_t mstatus() const { return mstatus_; }
    std::uint32_t misa() const { return misa_; }
    std::uint32_t mtvec() const { return mtvec_; }
    std::uint32_t mepc() const { return mepc_; }
    std::uint32_t mcause() const { return mcause_; }
    std::uint32_t mtval() const { return mtval_; }
    std::uint32_t mie() const { return mie_; }
    std::uint32_t mip() const { return mip_; }
    std::uint32_t mscratch() const { return mscratch_; }
    std::uint32_t mhartid() const { return mhartid_; }

    void set_mepc(std::uint32_t v) { mepc_ = v; }
    void set_mcause(std::uint32_t v) { mcause_ = v; }
    void set_mtval(std::uint32_t v) { mtval_ = v; }
    void set_mip(std::uint32_t v) { mip_ = v; }
    void set_mie(std::uint32_t v) { mie_ = v; }
    void set_mtvec(std::uint32_t v) { mtvec_ = v; }
    void set_mhartid(std::uint32_t v) { mhartid_ = v; }

    std::uint32_t stvec() const { return stvec_; }
    std::uint32_t sepc()  const { return sepc_; }
    std::uint32_t scause() const { return scause_; }
    std::uint32_t stval() const { return stval_; }
    std::uint32_t medeleg() const { return medeleg_; }
    std::uint32_t mideleg() const { return mideleg_; }

    void set_sepc(std::uint32_t v) { sepc_ = v; }
    void set_scause(std::uint32_t v) { scause_ = v; }
    void set_stval(std::uint32_t v) { stval_ = v; }
    void set_stvec(std::uint32_t v) { stvec_ = v; }


    // Counters (very minimal)
    void increment_cycle(std::uint64_t delta = 1);
    void increment_instret(std::uint64_t delta = 1);

private:
    // Core M-mode CSRs
    std::uint32_t mstatus_{0};
    std::uint32_t misa_{0};
    std::uint32_t mtvec_{0};
    std::uint32_t mscratch_{0};
    std::uint32_t mepc_{0};
    std::uint32_t mcause_{0};
    std::uint32_t mtval_{0};
    std::uint32_t mie_{0};
    std::uint32_t mip_{0};
    std::uint32_t pmpcfg0_{0};
    std::uint32_t pmpaddr0_{0};
    std::uint32_t mhartid_{0};
    std::uint32_t mvendorid_{0};
    std::uint32_t marchid_{0};
    std::uint32_t mimpid_{0};

    // Supervisor trap CSRs
    std::uint32_t stvec_{0};
    std::uint32_t sepc_{0};
    std::uint32_t scause_{0};
    std::uint32_t stval_{0};

    // Delegation
    std::uint32_t medeleg_{0};
    std::uint32_t mideleg_{0};

    // Basic counters (lower 32; you can extend to 64-bit CSRs later)
    std::uint64_t mcycle_{0};
    std::uint64_t minstret_{0};

private:
    static std::uint32_t build_misa_rv32ima_();
};

} // namespace remu::cpu
