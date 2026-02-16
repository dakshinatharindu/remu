#pragma once

#include <cstdint>

namespace remu::cpu {

enum class InsnKind : std::uint16_t {
    Illegal = 0,

    // RV32I
    LUI,
    AUIPC,
    JAL,
    JALR,

    BEQ,
    BNE,
    BLT,
    BGE,
    BLTU,
    BGEU,

    LB,
    LH,
    LW,
    LBU,
    LHU,
    SB,
    SH,
    SW,

    ADDI,
    SLTI,
    SLTIU,
    XORI,
    ORI,
    ANDI,
    SLLI,
    SRLI,
    SRAI,

    ADD,
    SUB,
    SLL,
    SLT,
    SLTU,
    XOR,
    SRL,
    SRA,
    OR,
    AND,

    FENCE,
    ECALL,
    EBREAK,
    MRET,
    SRET,
    WFI,
    CSRRW,
    CSRRS,
    CSRRC,
    CSRRWI,
    CSRRSI,
    CSRRCI,

    // RV32M
    MUL,
    MULH,
    MULHSU,
    MULHU,
    DIV,
    DIVU,
    REM,
    REMU,

    // RV32A
    LR_W,
    SC_W,
    AMOSWAP_W,
    AMOADD_W,
    AMOXOR_W,
    AMOAND_W,
    AMOOR_W,
    AMOMIN_W,
    AMOMAX_W,
    AMOMINU_W,
    AMOMAXU_W,
};

enum class InsnFormat : std::uint8_t { R, I, S, B, U, J, Other };

struct DecodedInsn {
    InsnKind kind = InsnKind::Illegal;
    InsnFormat fmt = InsnFormat::Other;

    std::uint32_t raw = 0;

    std::uint8_t rd = 0;
    std::uint8_t rs1 = 0;
    std::uint8_t rs2 = 0;

    std::int32_t imm = 0;

    std::uint8_t length = 4;  // bytes (keep for future RV32C)
};

DecodedInsn decode_rv32(std::uint32_t insn);

// Execute helpers (implemented in separate .cpp files)
bool execute_rv32i(const DecodedInsn& d, class Cpu& cpu,
                   class remu_mem_bus_placeholder&);
bool execute_rv32m(const DecodedInsn& d, class Cpu& cpu,
                   class remu_mem_bus_placeholder&);
bool execute_rv32a(const DecodedInsn& d, class Cpu& cpu,
                   class remu_mem_bus_placeholder&);

// NOTE: The bus placeholder above is to avoid including bus.hpp in this header.
// In your project, replace those declarations with a proper execute.hpp
// that includes remu/mem/bus.hpp and declares:
//   bool execute_rv32i(const DecodedInsn&, Cpu&, remu::mem::Bus&);
// etc.

}  // namespace remu::cpu
