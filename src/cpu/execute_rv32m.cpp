#include <remu/cpu/decode.hpp>
#include <remu/cpu/cpu.hpp>
#include <remu/mem/bus.hpp>

#include <cstdint>
#include <limits>

namespace remu::cpu {

bool execute_rv32m(const DecodedInsn& d, Cpu& cpu, remu::mem::Bus&) {
    const std::uint32_t pc = cpu.pc;
    const std::uint32_t rs1u = cpu.regs.read(d.rs1);
    const std::uint32_t rs2u = cpu.regs.read(d.rs2);
    const std::int32_t  rs1s = static_cast<std::int32_t>(rs1u);
    const std::int32_t  rs2s = static_cast<std::int32_t>(rs2u);

    const std::uint32_t next_pc = pc + d.length;

    switch (d.kind) {
        case InsnKind::MUL: {
            std::uint64_t prod = static_cast<std::uint64_t>(rs1u) * static_cast<std::uint64_t>(rs2u);
            cpu.regs.write(d.rd, static_cast<std::uint32_t>(prod));
            cpu.pc = next_pc;
            return true;
        }
        case InsnKind::MULH: {
            std::int64_t prod = static_cast<std::int64_t>(rs1s) * static_cast<std::int64_t>(rs2s);
            cpu.regs.write(d.rd, static_cast<std::uint32_t>(static_cast<std::uint64_t>(prod) >> 32));
            cpu.pc = next_pc;
            return true;
        }
        case InsnKind::MULHSU: {
            std::int64_t  a = static_cast<std::int64_t>(rs1s);
            std::uint64_t b = static_cast<std::uint64_t>(rs2u);
            std::int64_t prod = a * static_cast<std::int64_t>(b);
            cpu.regs.write(d.rd, static_cast<std::uint32_t>(static_cast<std::uint64_t>(prod) >> 32));
            cpu.pc = next_pc;
            return true;
        }
        case InsnKind::MULHU: {
            std::uint64_t prod = static_cast<std::uint64_t>(rs1u) * static_cast<std::uint64_t>(rs2u);
            cpu.regs.write(d.rd, static_cast<std::uint32_t>(prod >> 32));
            cpu.pc = next_pc;
            return true;
        }

        case InsnKind::DIV: {
            if (rs2u == 0) {
                cpu.regs.write(d.rd, 0xFFFF'FFFFu);
            } else if (rs1s == std::numeric_limits<std::int32_t>::min() && rs2s == -1) {
                cpu.regs.write(d.rd, static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::min()));
            } else {
                cpu.regs.write(d.rd, static_cast<std::uint32_t>(rs1s / rs2s));
            }
            cpu.pc = next_pc;
            return true;
        }

        case InsnKind::DIVU: {
            if (rs2u == 0) {
                cpu.regs.write(d.rd, 0xFFFF'FFFFu);
            } else {
                cpu.regs.write(d.rd, rs1u / rs2u);
            }
            cpu.pc = next_pc;
            return true;
        }

        case InsnKind::REM: {
            if (rs2u == 0) {
                cpu.regs.write(d.rd, rs1u);
            } else if (rs1s == std::numeric_limits<std::int32_t>::min() && rs2s == -1) {
                cpu.regs.write(d.rd, 0);
            } else {
                cpu.regs.write(d.rd, static_cast<std::uint32_t>(rs1s % rs2s));
            }
            cpu.pc = next_pc;
            return true;
        }

        case InsnKind::REMU: {
            if (rs2u == 0) {
                cpu.regs.write(d.rd, rs1u);
            } else {
                cpu.regs.write(d.rd, rs1u % rs2u);
            }
            cpu.pc = next_pc;
            return true;
        }

        default:
            return false;
    }
}

} // namespace remu::cpu
