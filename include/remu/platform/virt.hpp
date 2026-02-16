#pragma once

#include <cstdint>
#include <remu/mem/bus.hpp>
#include <remu/mem/memory.hpp>
#include <remu/cpu/cpu.hpp>

#include <remu/devices/uart.hpp>
#include <remu/devices/clint.hpp>
#include <remu/devices/plic.hpp>

namespace remu::platform {

class VirtMachine {
   public:
    explicit VirtMachine(std::uint32_t mem_size_bytes);

    // Access bus for CPU + loaders
    remu::mem::Bus& bus() { return bus_; }
    const remu::mem::Bus& bus() const { return bus_; }

    // Optional: direct RAM accessors (for loaders/debug)
    remu::mem::Memory& ram() { return ram_; }
    const remu::mem::Memory& ram() const { return ram_; }

    // Optional: direct DTB accessors (for loaders/debug)
    remu::mem::Memory& dtb() { return dtb_; }
    const remu::mem::Memory& dtb() const { return dtb_; }

    // Convenience accessors (optional)
    std::uint32_t ram_base() const { return ram_base_; }
    std::uint32_t ram_size() const { return mem_size_bytes_; }
    std::uint32_t dtb_base() const { return dtb_base_; }

    // Optional: tick devices (timers/interrupts). Call this from Sim loop.
    void tick(std::uint64_t cycles, remu::cpu::Cpu& cpu);

   private:
    void map_devices_();

   private:
    std::uint32_t ram_base_;
    std::uint32_t mem_size_bytes_;
    std::uint32_t dtb_base_; // optional, for future use

    // Owned components
    remu::mem::Memory ram_;
    remu::mem::Memory dtb_;
    remu::mem::Bus bus_;

    // Minimal compulsory devices for Linux bring-up
    remu::devices::UartNs16550 uart_;
    remu::devices::Clint clint_;
    remu::devices::Plic plic_;
};

}  // namespace remu::platform
