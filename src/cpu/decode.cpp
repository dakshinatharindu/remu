#include <remu/cpu/decode.hpp>

#include <cstdint>

namespace remu::cpu {

namespace {

constexpr std::uint32_t get_bits(std::uint32_t x, int hi, int lo) {
    const std::uint32_t mask = (hi == 31 && lo == 0) ? 0xFFFF'FFFFu
        : ((1u << (hi - lo + 1)) - 1u);
    return (x >> lo) & mask;
}

constexpr std::int32_t sign_extend(std::uint32_t x, int bits) {
    // bits: number of valid low bits in x
    const std::uint32_t m = 1u << (bits - 1);
    const std::uint32_t r = (x ^ m) - m;
    return static_cast<std::int32_t>(r);
}

constexpr std::int32_t imm_i(std::uint32_t insn) {
    return sign_extend(get_bits(insn, 31, 20), 12);
}

constexpr std::int32_t imm_s(std::uint32_t insn) {
    std::uint32_t imm = (get_bits(insn, 31, 25) << 5) | get_bits(insn, 11, 7);
    return sign_extend(imm, 12);
}

constexpr std::int32_t imm_b(std::uint32_t insn) {
    // imm[12|10:5|4:1|11] << 1
    std::uint32_t imm =
        (get_bits(insn, 31, 31) << 12) |
        (get_bits(insn, 7, 7)   << 11) |
        (get_bits(insn, 30, 25) << 5)  |
        (get_bits(insn, 11, 8)  << 1);
    return sign_extend(imm, 13);
}

constexpr std::int32_t imm_u(std::uint32_t insn) {
    return static_cast<std::int32_t>(insn & 0xFFFFF000u);
}

constexpr std::int32_t imm_j(std::uint32_t insn) {
    // imm[20|10:1|11|19:12] << 1
    std::uint32_t imm =
        (get_bits(insn, 31, 31) << 20) |
        (get_bits(insn, 19, 12) << 12) |
        (get_bits(insn, 20, 20) << 11) |
        (get_bits(insn, 30, 21) << 1);
    return sign_extend(imm, 21);
}

} // namespace

DecodedInsn decode_rv32(std::uint32_t insn) {
    DecodedInsn d;
    d.raw = insn;

    const std::uint32_t opcode = get_bits(insn, 6, 0);
    const std::uint32_t rd     = get_bits(insn, 11, 7);
    const std::uint32_t funct3 = get_bits(insn, 14, 12);
    const std::uint32_t rs1    = get_bits(insn, 19, 15);
    const std::uint32_t rs2    = get_bits(insn, 24, 20);
    const std::uint32_t funct7 = get_bits(insn, 31, 25);

    d.rd  = static_cast<std::uint8_t>(rd);
    d.rs1 = static_cast<std::uint8_t>(rs1);
    d.rs2 = static_cast<std::uint8_t>(rs2);

    switch (opcode) {
        case 0x37: // LUI
            d.fmt = InsnFormat::U;
            d.imm = imm_u(insn);
            d.kind = InsnKind::LUI;
            return d;

        case 0x17: // AUIPC
            d.fmt = InsnFormat::U;
            d.imm = imm_u(insn);
            d.kind = InsnKind::AUIPC;
            return d;

        case 0x6F: // JAL
            d.fmt = InsnFormat::J;
            d.imm = imm_j(insn);
            d.kind = InsnKind::JAL;
            return d;

        case 0x67: // JALR
            d.fmt = InsnFormat::I;
            d.imm = imm_i(insn);
            d.kind = InsnKind::JALR;
            return d;

        case 0x63: // BRANCH
            d.fmt = InsnFormat::B;
            d.imm = imm_b(insn);
            switch (funct3) {
                case 0x0: d.kind = InsnKind::BEQ;  return d;
                case 0x1: d.kind = InsnKind::BNE;  return d;
                case 0x4: d.kind = InsnKind::BLT;  return d;
                case 0x5: d.kind = InsnKind::BGE;  return d;
                case 0x6: d.kind = InsnKind::BLTU; return d;
                case 0x7: d.kind = InsnKind::BGEU; return d;
                default:  return d;
            }

        case 0x03: // LOAD
            d.fmt = InsnFormat::I;
            d.imm = imm_i(insn);
            switch (funct3) {
                case 0x0: d.kind = InsnKind::LB;  return d;
                case 0x1: d.kind = InsnKind::LH;  return d;
                case 0x2: d.kind = InsnKind::LW;  return d;
                case 0x4: d.kind = InsnKind::LBU; return d;
                case 0x5: d.kind = InsnKind::LHU; return d;
                default:  return d;
            }

        case 0x23: // STORE
            d.fmt = InsnFormat::S;
            d.imm = imm_s(insn);
            switch (funct3) {
                case 0x0: d.kind = InsnKind::SB; return d;
                case 0x1: d.kind = InsnKind::SH; return d;
                case 0x2: d.kind = InsnKind::SW; return d;
                default:  return d;
            }

        case 0x13: // OP-IMM
            d.fmt = InsnFormat::I;
            d.imm = imm_i(insn);
            switch (funct3) {
                case 0x0: d.kind = InsnKind::ADDI;  return d;
                case 0x2: d.kind = InsnKind::SLTI;  return d;
                case 0x3: d.kind = InsnKind::SLTIU; return d;
                case 0x4: d.kind = InsnKind::XORI;  return d;
                case 0x6: d.kind = InsnKind::ORI;   return d;
                case 0x7: d.kind = InsnKind::ANDI;  return d;
                case 0x1: d.kind = InsnKind::SLLI;  d.imm = static_cast<std::int32_t>(get_bits(insn, 24, 20)); return d;
                case 0x5:
                    if (funct7 == 0x00) { d.kind = InsnKind::SRLI; d.imm = static_cast<std::int32_t>(get_bits(insn, 24, 20)); return d; }
                    if (funct7 == 0x20) { d.kind = InsnKind::SRAI; d.imm = static_cast<std::int32_t>(get_bits(insn, 24, 20)); return d; }
                    return d;
                default:
                    return d;
            }

        case 0x33: // OP (RV32I + RV32M)
            d.fmt = InsnFormat::R;
            if (funct7 == 0x01) { // M extension
                switch (funct3) {
                    case 0x0: d.kind = InsnKind::MUL;    return d;
                    case 0x1: d.kind = InsnKind::MULH;   return d;
                    case 0x2: d.kind = InsnKind::MULHSU; return d;
                    case 0x3: d.kind = InsnKind::MULHU;  return d;
                    case 0x4: d.kind = InsnKind::DIV;    return d;
                    case 0x5: d.kind = InsnKind::DIVU;   return d;
                    case 0x6: d.kind = InsnKind::REM;    return d;
                    case 0x7: d.kind = InsnKind::REMU;   return d;
                    default:  return d;
                }
            } else {
                switch (funct3) {
                    case 0x0:
                        if (funct7 == 0x00) { d.kind = InsnKind::ADD; return d; }
                        if (funct7 == 0x20) { d.kind = InsnKind::SUB; return d; }
                        return d;
                    case 0x1: d.kind = InsnKind::SLL;  return d;
                    case 0x2: d.kind = InsnKind::SLT;  return d;
                    case 0x3: d.kind = InsnKind::SLTU; return d;
                    case 0x4: d.kind = InsnKind::XOR;  return d;
                    case 0x5:
                        if (funct7 == 0x00) { d.kind = InsnKind::SRL; return d; }
                        if (funct7 == 0x20) { d.kind = InsnKind::SRA; return d; }
                        return d;
                    case 0x6: d.kind = InsnKind::OR;   return d;
                    case 0x7: d.kind = InsnKind::AND;  return d;
                    default:  return d;
                }
            }

        case 0x0F: // MISC-MEM
            d.fmt = InsnFormat::I;
            d.kind = InsnKind::FENCE;
            return d;

        case 0x73: { // SYSTEM
            // ECALL/EBREAK if funct3 == 0
            if (funct3 == 0x0) {
                const std::uint32_t imm12 = get_bits(insn, 31, 20);
                if (imm12 == 0x105) {
                    d.kind = InsnKind::WFI;
                    d.fmt = InsnFormat::Other;
                    return d;
                }
                d.fmt = InsnFormat::I;
                d.imm = static_cast<std::int32_t>(imm12);
                if (imm12 == 0x000) d.kind = InsnKind::ECALL;
                else if (imm12 == 0x001) d.kind = InsnKind::EBREAK;
                else if (imm12 == 0x302) d.kind = InsnKind::MRET;
                return d;
            }

            // CSR instructions
            d.fmt = InsnFormat::I;
            d.imm = static_cast<std::int32_t>(get_bits(insn, 31, 20)); // csr addr (12-bit)
            switch (funct3) {
                case 0x1: d.kind = InsnKind::CSRRW;  return d;
                case 0x2: d.kind = InsnKind::CSRRS;  return d;
                case 0x3: d.kind = InsnKind::CSRRC;  return d;
                case 0x5: d.kind = InsnKind::CSRRWI; return d;
                case 0x6: d.kind = InsnKind::CSRRSI; return d;
                case 0x7: d.kind = InsnKind::CSRRCI; return d;
                default:  return d;
            }
        }

        case 0x2F: { // AMO (A extension)
            d.fmt = InsnFormat::R;
            // funct5 in [31:27], aq/rl ignored for now
            const std::uint32_t funct5 = get_bits(insn, 31, 27);
            const std::uint32_t width  = get_bits(insn, 14, 12); // should be 010 for W
            if (width != 0x2) return d;

            switch (funct5) {
                case 0x02: d.kind = InsnKind::LR_W;       return d;
                case 0x03: d.kind = InsnKind::SC_W;       return d;
                case 0x01: d.kind = InsnKind::AMOSWAP_W;  return d;
                case 0x00: d.kind = InsnKind::AMOADD_W;   return d;
                case 0x04: d.kind = InsnKind::AMOXOR_W;   return d;
                case 0x0C: d.kind = InsnKind::AMOAND_W;   return d;
                case 0x08: d.kind = InsnKind::AMOOR_W;    return d;
                case 0x10: d.kind = InsnKind::AMOMIN_W;   return d;
                case 0x14: d.kind = InsnKind::AMOMAX_W;   return d;
                case 0x18: d.kind = InsnKind::AMOMINU_W;  return d;
                case 0x1C: d.kind = InsnKind::AMOMAXU_W;  return d;
                default:   return d;
            }
        }

        default:
            return d;
    }
}

} // namespace rvemu::cpu
