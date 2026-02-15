#include <remu/cpu/decode.hpp>
#include <remu/cpu/cpu.hpp>
#include <remu/mem/bus.hpp>

#include <cstdint>

namespace remu::cpu {

namespace {

inline std::uint32_t amo_min_s(std::uint32_t a, std::uint32_t b) {
    return (static_cast<std::int32_t>(a) < static_cast<std::int32_t>(b)) ? a : b;
}
inline std::uint32_t amo_max_s(std::uint32_t a, std::uint32_t b) {
    return (static_cast<std::int32_t>(a) > static_cast<std::int32_t>(b)) ? a : b;
}
inline std::uint32_t amo_min_u(std::uint32_t a, std::uint32_t b) { return (a < b) ? a : b; }
inline std::uint32_t amo_max_u(std::uint32_t a, std::uint32_t b) { return (a > b) ? a : b; }

} // namespace

bool execute_rv32a(const DecodedInsn& d, Cpu& cpu, remu::mem::Bus& bus) {
    const std::uint32_t pc = cpu.pc;
    const std::uint32_t rs1u = cpu.regs.read(d.rs1);
    const std::uint32_t rs2u = cpu.regs.read(d.rs2);
    const std::uint32_t addr = rs1u;

    const std::uint32_t next_pc = pc + d.length;

    auto load32 = [&](std::uint32_t& out) -> bool {
        return bus.read32(addr, out);
    };
    auto store32 = [&](std::uint32_t v) -> bool {
        return bus.write32(addr, v);
    };

    switch (d.kind) {
        case InsnKind::LR_W: {
            std::uint32_t old = 0;
            if (!load32(old)) return false;
            cpu.regs.write(d.rd, old);
            cpu.reservation_valid = true;
            cpu.reservation_addr = addr;
            cpu.pc = next_pc;
            return true;
        }

        case InsnKind::SC_W: {
            const bool ok = cpu.reservation_valid && (cpu.reservation_addr == addr);
            if (ok) {
                if (!store32(rs2u)) return false;
                cpu.regs.write(d.rd, 0); // success
            } else {
                cpu.regs.write(d.rd, 1); // failure
            }
            cpu.reservation_valid = false;
            cpu.pc = next_pc;
            return true;
        }

        case InsnKind::AMOSWAP_W:
        case InsnKind::AMOADD_W:
        case InsnKind::AMOXOR_W:
        case InsnKind::AMOAND_W:
        case InsnKind::AMOOR_W:
        case InsnKind::AMOMIN_W:
        case InsnKind::AMOMAX_W:
        case InsnKind::AMOMINU_W:
        case InsnKind::AMOMAXU_W: {
            std::uint32_t old = 0;
            if (!load32(old)) return false;

            std::uint32_t newv = old;
            switch (d.kind) {
                case InsnKind::AMOSWAP_W: newv = rs2u; break;
                case InsnKind::AMOADD_W:  newv = old + rs2u; break;
                case InsnKind::AMOXOR_W:  newv = old ^ rs2u; break;
                case InsnKind::AMOAND_W:  newv = old & rs2u; break;
                case InsnKind::AMOOR_W:   newv = old | rs2u; break;
                case InsnKind::AMOMIN_W:  newv = amo_min_s(old, rs2u); break;
                case InsnKind::AMOMAX_W:  newv = amo_max_s(old, rs2u); break;
                case InsnKind::AMOMINU_W: newv = amo_min_u(old, rs2u); break;
                case InsnKind::AMOMAXU_W: newv = amo_max_u(old, rs2u); break;
                default: break;
            }

            if (!store32(newv)) return false;
            cpu.regs.write(d.rd, old);

            cpu.reservation_valid = false; // a simple model: any AMO breaks reservations
            cpu.pc = next_pc;
            return true;
        }

        default:
            return false;
    }
}

} // namespace rvemu::cpu
