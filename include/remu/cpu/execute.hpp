#pragma once

#include <remu/cpu/decode.hpp>
#include <remu/cpu/cpu.hpp>
#include <remu/mem/bus.hpp>

namespace remu::cpu {

// Execute subsets
bool execute_rv32i(const DecodedInsn& d, Cpu& cpu, remu::mem::Bus& bus);
bool execute_rv32m(const DecodedInsn& d, Cpu& cpu, remu::mem::Bus& bus);
bool execute_rv32a(const DecodedInsn& d, Cpu& cpu, remu::mem::Bus& bus);

// Convenience dispatcher (so Sim loop stays clean)
inline bool execute(const DecodedInsn& d, Cpu& cpu, remu::mem::Bus& bus) {
    switch (d.kind) {
        // RV32M
        case InsnKind::MUL:
        case InsnKind::MULH:
        case InsnKind::MULHSU:
        case InsnKind::MULHU:
        case InsnKind::DIV:
        case InsnKind::DIVU:
        case InsnKind::REM:
        case InsnKind::REMU:
            return execute_rv32m(d, cpu, bus);

        // RV32A
        case InsnKind::LR_W:
        case InsnKind::SC_W:
        case InsnKind::AMOSWAP_W:
        case InsnKind::AMOADD_W:
        case InsnKind::AMOXOR_W:
        case InsnKind::AMOAND_W:
        case InsnKind::AMOOR_W:
        case InsnKind::AMOMIN_W:
        case InsnKind::AMOMAX_W:
        case InsnKind::AMOMINU_W:
        case InsnKind::AMOMAXU_W:
            return execute_rv32a(d, cpu, bus);

        // Everything else treated as RV32I/System/CSR
        default:
            return execute_rv32i(d, cpu, bus);
    }
}

} // namespace remu::cpu
