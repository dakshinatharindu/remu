#pragma once

#include <cstdint>
#include <array>
#include <mutex>

#include <remu/mem/region.hpp> // rvemu::mem::MmioDevice

namespace remu::devices {

// Minimal single-hart PLIC (QEMU virt style address layout).
// - Interrupt IDs: 1..N (0 means "no interrupt").
// - Supports: priority, pending, enable (hart0), threshold (hart0), claim/complete (hart0).
class Plic final : public remu::mem::MmioDevice {
public:
    // Keep this small at first. QEMU virt uses many IDs, but Linux UART is usually one ID.
    // You can increase later.
    static constexpr std::uint32_t kMaxIrq = 64; // IDs 1..64 supported

    Plic();

    // MmioDevice interface
    bool read (std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t& out) override;
    bool write(std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t  val) override;

    // Device-facing API
    void raise_irq(std::uint32_t irq_id); // set pending
    void clear_irq(std::uint32_t irq_id); // clear pending

    // Query: should MEIP be asserted for hart0?
    bool has_pending_for_hart0() const;

private:
    // Address decode helpers (offset within PLIC window)
    static std::uint32_t off_(std::uint32_t addr) {
        // PLIC base is usually aligned; mask a big window
        return (addr & 0x00FF'FFFFu); // 16 MiB window is plenty for virt
    }

    // Compute best IRQ to claim for hart0 (0 if none)
    std::uint32_t pick_best_irq_() const;

private:
    mutable std::mutex mu_;

    // priority[irq] for irq 0..kMaxIrq (irq 0 unused)
    std::array<std::uint32_t, kMaxIrq + 1> priority_{};

    // pending bitset for irq 0..kMaxIrq (we store as 64-bit)
    std::uint64_t pending_ = 0;

    // enable bitset for hart0 (same encoding as pending)
    std::uint64_t enable0_ = 0;

    // threshold for hart0
    std::uint32_t threshold0_ = 0;

    // "in service" is optional; we keep it minimal.
};

} // namespace rvemu::devices
