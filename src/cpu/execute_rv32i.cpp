#include <remu/cpu/decode.hpp>
#include <remu/cpu/cpu.hpp>
#include <remu/mem/bus.hpp>

#include <cstdint>

namespace remu::cpu {

namespace {

inline std::uint32_t u32(std::int32_t v) { return static_cast<std::uint32_t>(v); }

inline bool load_u8(remu::mem::Bus& bus, std::uint32_t addr, std::uint8_t& out) {
    return bus.read8(addr, out);
}
inline bool load_u16(remu::mem::Bus& bus, std::uint32_t addr, std::uint16_t& out) {
    return bus.read16(addr, out);
}
inline bool load_u32(remu::mem::Bus& bus, std::uint32_t addr, std::uint32_t& out) {
    return bus.read32(addr, out);
}

inline bool store_u8(remu::mem::Bus& bus, std::uint32_t addr, std::uint8_t v) {
    return bus.write8(addr, v);
}
inline bool store_u16(remu::mem::Bus& bus, std::uint32_t addr, std::uint16_t v) {
    return bus.write16(addr, v);
}
inline bool store_u32(remu::mem::Bus& bus, std::uint32_t addr, std::uint32_t v) {
    return bus.write32(addr, v);
}

inline std::int32_t sext8(std::uint8_t v)  { return static_cast<std::int32_t>(static_cast<std::int8_t>(v)); }
inline std::int32_t sext16(std::uint16_t v){ return static_cast<std::int32_t>(static_cast<std::int16_t>(v)); }

} // namespace

bool execute_rv32i(const DecodedInsn& d, Cpu& cpu, remu::mem::Bus& bus) {
    const std::uint32_t pc = cpu.pc;
    const std::uint32_t rs1v = cpu.regs.read(d.rs1);
    const std::uint32_t rs2v = cpu.regs.read(d.rs2);

    // Default next PC (most instructions)
    std::uint32_t next_pc = pc + d.length;

    switch (d.kind) {
        case InsnKind::LUI:
            cpu.regs.write(d.rd, u32(d.imm));
            cpu.pc = next_pc;
            return true;

        case InsnKind::AUIPC:
            cpu.regs.write(d.rd, pc + u32(d.imm));
            cpu.pc = next_pc;
            return true;

        case InsnKind::JAL:
            cpu.regs.write(d.rd, pc + d.length);
            cpu.pc = pc + u32(d.imm);
            return true;

        case InsnKind::JALR: {
            cpu.regs.write(d.rd, pc + d.length);
            std::uint32_t target = rs1v + u32(d.imm);
            target &= ~1u;
            cpu.pc = target;
            return true;
        }

        // Branches
        case InsnKind::BEQ:
            cpu.pc = (rs1v == rs2v) ? (pc + u32(d.imm)) : next_pc;
            return true;
        case InsnKind::BNE:
            cpu.pc = (rs1v != rs2v) ? (pc + u32(d.imm)) : next_pc;
            return true;
        case InsnKind::BLT:
            cpu.pc = (static_cast<std::int32_t>(rs1v) < static_cast<std::int32_t>(rs2v)) ? (pc + u32(d.imm)) : next_pc;
            return true;
        case InsnKind::BGE:
            cpu.pc = (static_cast<std::int32_t>(rs1v) >= static_cast<std::int32_t>(rs2v)) ? (pc + u32(d.imm)) : next_pc;
            return true;
        case InsnKind::BLTU:
            cpu.pc = (rs1v < rs2v) ? (pc + u32(d.imm)) : next_pc;
            return true;
        case InsnKind::BGEU:
            cpu.pc = (rs1v >= rs2v) ? (pc + u32(d.imm)) : next_pc;
            return true;

        // Loads
        case InsnKind::LB: {
            std::uint8_t b{};
            if (!load_u8(bus, rs1v + u32(d.imm), b)) return false;
            cpu.regs.write(d.rd, static_cast<std::uint32_t>(sext8(b)));
            cpu.pc = next_pc;
            return true;
        }
        case InsnKind::LBU: {
            std::uint8_t b{};
            if (!load_u8(bus, rs1v + u32(d.imm), b)) return false;
            cpu.regs.write(d.rd, b);
            cpu.pc = next_pc;
            return true;
        }
        case InsnKind::LH: {
            std::uint16_t h{};
            if (!load_u16(bus, rs1v + u32(d.imm), h)) return false;
            cpu.regs.write(d.rd, static_cast<std::uint32_t>(sext16(h)));
            cpu.pc = next_pc;
            return true;
        }
        case InsnKind::LHU: {
            std::uint16_t h{};
            if (!load_u16(bus, rs1v + u32(d.imm), h)) return false;
            cpu.regs.write(d.rd, h);
            cpu.pc = next_pc;
            return true;
        }
        case InsnKind::LW: {
            std::uint32_t w{};
            if (!load_u32(bus, rs1v + u32(d.imm), w)) return false;
            cpu.regs.write(d.rd, w);
            cpu.pc = next_pc;
            return true;
        }

        // Stores
        case InsnKind::SB:
            if (!store_u8(bus, rs1v + u32(d.imm), static_cast<std::uint8_t>(rs2v & 0xFFu))) return false;
            cpu.pc = next_pc;
            return true;
        case InsnKind::SH:
            if (!store_u16(bus, rs1v + u32(d.imm), static_cast<std::uint16_t>(rs2v & 0xFFFFu))) return false;
            cpu.pc = next_pc;
            return true;
        case InsnKind::SW:
            if (!store_u32(bus, rs1v + u32(d.imm), rs2v)) return false;
            cpu.pc = next_pc;
            return true;

        // OP-IMM
        case InsnKind::ADDI:
            cpu.regs.write(d.rd, rs1v + u32(d.imm));
            cpu.pc = next_pc;
            return true;
        case InsnKind::SLTI:
            cpu.regs.write(d.rd, (static_cast<std::int32_t>(rs1v) < d.imm) ? 1u : 0u);
            cpu.pc = next_pc;
            return true;
        case InsnKind::SLTIU:
            cpu.regs.write(d.rd, (rs1v < u32(d.imm)) ? 1u : 0u);
            cpu.pc = next_pc;
            return true;
        case InsnKind::XORI:
            cpu.regs.write(d.rd, rs1v ^ u32(d.imm));
            cpu.pc = next_pc;
            return true;
        case InsnKind::ORI:
            cpu.regs.write(d.rd, rs1v | u32(d.imm));
            cpu.pc = next_pc;
            return true;
        case InsnKind::ANDI:
            cpu.regs.write(d.rd, rs1v & u32(d.imm));
            cpu.pc = next_pc;
            return true;
        case InsnKind::SLLI:
            cpu.regs.write(d.rd, rs1v << (static_cast<std::uint32_t>(d.imm) & 31u));
            cpu.pc = next_pc;
            return true;
        case InsnKind::SRLI:
            cpu.regs.write(d.rd, rs1v >> (static_cast<std::uint32_t>(d.imm) & 31u));
            cpu.pc = next_pc;
            return true;
        case InsnKind::SRAI:
            cpu.regs.write(d.rd, static_cast<std::uint32_t>(static_cast<std::int32_t>(rs1v) >> (static_cast<std::uint32_t>(d.imm) & 31u)));
            cpu.pc = next_pc;
            return true;

        // OP
        case InsnKind::ADD:
            cpu.regs.write(d.rd, rs1v + rs2v);
            cpu.pc = next_pc;
            return true;
        case InsnKind::SUB:
            cpu.regs.write(d.rd, rs1v - rs2v);
            cpu.pc = next_pc;
            return true;
        case InsnKind::SLL:
            cpu.regs.write(d.rd, rs1v << (rs2v & 31u));
            cpu.pc = next_pc;
            return true;
        case InsnKind::SLT:
            cpu.regs.write(d.rd, (static_cast<std::int32_t>(rs1v) < static_cast<std::int32_t>(rs2v)) ? 1u : 0u);
            cpu.pc = next_pc;
            return true;
        case InsnKind::SLTU:
            cpu.regs.write(d.rd, (rs1v < rs2v) ? 1u : 0u);
            cpu.pc = next_pc;
            return true;
        case InsnKind::XOR:
            cpu.regs.write(d.rd, rs1v ^ rs2v);
            cpu.pc = next_pc;
            return true;
        case InsnKind::SRL:
            cpu.regs.write(d.rd, rs1v >> (rs2v & 31u));
            cpu.pc = next_pc;
            return true;
        case InsnKind::SRA:
            cpu.regs.write(d.rd, static_cast<std::uint32_t>(static_cast<std::int32_t>(rs1v) >> (rs2v & 31u)));
            cpu.pc = next_pc;
            return true;
        case InsnKind::OR:
            cpu.regs.write(d.rd, rs1v | rs2v);
            cpu.pc = next_pc;
            return true;
        case InsnKind::AND:
            cpu.regs.write(d.rd, rs1v & rs2v);
            cpu.pc = next_pc;
            return true;

        case InsnKind::FENCE:
            // No-op for now
            cpu.pc = next_pc;
            return true;

        // CSR ops (minimal: no privilege checks yet)
        case InsnKind::CSRRW:
        case InsnKind::CSRRS:
        case InsnKind::CSRRC:
        case InsnKind::CSRRWI:
        case InsnKind::CSRRSI:
        case InsnKind::CSRRCI: {
            const std::uint16_t csr = static_cast<std::uint16_t>(d.imm & 0xFFF);
            std::uint32_t old = 0;
            if (!cpu.csr.read(csr, old)) return false;

            std::uint32_t zimm_or_rs1 = 0;
            if (d.kind == InsnKind::CSRRWI || d.kind == InsnKind::CSRRSI || d.kind == InsnKind::CSRRCI) {
                zimm_or_rs1 = d.rs1; // rs1 field encodes zimm
            } else {
                zimm_or_rs1 = rs1v;
            }

            std::uint32_t newv = old;
            switch (d.kind) {
                case InsnKind::CSRRW:
                case InsnKind::CSRRWI:
                    newv = zimm_or_rs1;
                    break;
                case InsnKind::CSRRS:
                case InsnKind::CSRRSI:
                    if (zimm_or_rs1 != 0) newv = old | zimm_or_rs1;
                    break;
                case InsnKind::CSRRC:
                case InsnKind::CSRRCI:
                    if (zimm_or_rs1 != 0) newv = old & ~zimm_or_rs1;
                    break;
                default:
                    break;
            }

            if (d.rd != 0) cpu.regs.write(d.rd, old);
            if (!cpu.csr.write(csr, newv)) return false;

            cpu.pc = next_pc;
            return true;
        }

        case InsnKind::ECALL:
        case InsnKind::EBREAK:
            // For now: signal “stop” to the simulator.
            return false;
        
        case InsnKind::MRET: {
            constexpr std::uint32_t MSTATUS_MIE   = 1u << 3;
            constexpr std::uint32_t MSTATUS_MPIE  = 1u << 7;
            constexpr std::uint32_t MSTATUS_MPP_MASK = 3u << 11;

            std::uint32_t ms = cpu.csr.mstatus();

            // Extract MPP
            std::uint32_t mpp = (ms & MSTATUS_MPP_MASK) >> 11;

            // MIE <- MPIE
            if (ms & MSTATUS_MPIE) ms |= MSTATUS_MIE;
            else                   ms &= ~MSTATUS_MIE;

            // MPIE <- 1
            ms |= MSTATUS_MPIE;

            // MPP <- 0 (U-mode) after return
            ms &= ~MSTATUS_MPP_MASK;

            cpu.csr.write(0x300, ms); // or set_mstatus
            cpu.priv = static_cast<remu::cpu::PrivMode>(mpp);

            cpu.pc = cpu.csr.mepc();
            return true;
        }

        case InsnKind::SRET: {
            // sstatus bits live in mstatus (view)
            constexpr std::uint32_t MSTATUS_SIE   = 1u << 1;
            constexpr std::uint32_t MSTATUS_SPIE  = 1u << 5;
            constexpr std::uint32_t MSTATUS_SPP   = 1u << 8;

            std::uint32_t ms = cpu.csr.mstatus();

            const bool spp = (ms & MSTATUS_SPP) != 0; // 0=return to U, 1=return to S

            // SIE <- SPIE
            if (ms & MSTATUS_SPIE) ms |= MSTATUS_SIE;
            else                   ms &= ~MSTATUS_SIE;

            // SPIE <- 1, SPP <- 0
            ms |= MSTATUS_SPIE;
            ms &= ~MSTATUS_SPP;

            cpu.csr.write(0x300, ms);
            cpu.priv = spp ? remu::cpu::PrivMode::Supervisor : remu::cpu::PrivMode::User;
            cpu.pc = cpu.csr.sepc();
            return true;
        }

        default:
            return false;
    }
}

} // namespace remu::cpu
