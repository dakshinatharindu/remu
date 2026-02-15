#include <remu/platform/virt.hpp>

namespace remu::platform {

namespace memmap {
// QEMU virt-style base addresses
static constexpr std::uint32_t CLINT_BASE = 0x0200'0000;
static constexpr std::uint32_t CLINT_SIZE = 0x0001'0000;  // 64 KiB window is enough for CLINT regs

static constexpr std::uint32_t PLIC_BASE = 0x0C00'0000;
static constexpr std::uint32_t PLIC_SIZE = 0x0400'0000;  // stub big window (you can refine later)

static constexpr std::uint32_t UART_BASE = 0x1000'0000;
static constexpr std::uint32_t UART_SIZE = 0x0000'0100;  // 256 bytes window typical

static constexpr std::uint32_t RAM_BASE = 0x8000'0000;
}  // namespace memmap

VirtMachine::VirtMachine(std::uint32_t mem_size_bytes)
    : ram_base_(memmap::RAM_BASE),
      mem_size_bytes_(mem_size_bytes),
      ram_(ram_base_, mem_size_bytes_),
      bus_()
// Devices: you can pass params if your device constructors need them
//   uart_(),
//   clint_(),
//   plic_()
{
    map_devices_();
}

void VirtMachine::map_devices_() {
    // 1) RAM
    bus_.map_ram(ram_base_, mem_size_bytes_, ram_);

    // 2) UART (ns16550-like)
    // bus_.map_mmio(memmap::UART_BASE, memmap::UART_SIZE, uart_);

    // // 3) CLINT (mtime/mtimecmp/msip)
    // bus_.map_mmio(memmap::CLINT_BASE, memmap::CLINT_SIZE, clint_);

    // // 4) PLIC (stub for now)
    // bus_.map_mmio(memmap::PLIC_BASE, memmap::PLIC_SIZE, plic_);
}

void VirtMachine::tick(std::uint64_t cycles) {
    // Advance timers, update pending interrupts, etc.
    // Your Sim loop can call machine.tick(1) per instruction (start),
    // later you can do cycle-accurate or batch ticks.
    // clint_.tick(cycles);

    // UART might not need ticking unless you simulate RX/TX timing
    // uart_.tick(cycles);

    // PLIC stub likely no-op
    // plic_.tick(cycles);
}

}  // namespace remu::platform
