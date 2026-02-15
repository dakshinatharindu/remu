#pragma once

#include <cstdint>
#include <mutex>

#include <remu/mem/region.hpp> // for remu::mem::MmioDevice

namespace remu::devices {

// Minimal NS16550-compatible UART (enough for early printk)
// - Only MMIO register behavior needed for basic TX.
// - No real interrupts yet.
class UartNs16550 final : public remu::mem::MmioDevice {
public:
    UartNs16550();

    // MmioDevice interface
    bool read (std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t& out) override;
    bool write(std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t  val) override;

    // Optional: allow test/dev code to inject a received byte (sets DR bit)
    void inject_rx_byte(std::uint8_t byte);

private:
    // 8-bit accessors (NS16550 registers are byte-based)
    bool read8_(std::uint32_t addr, std::uint8_t& out);
    bool write8_(std::uint32_t addr, std::uint8_t val);

    // Map absolute address to register offset within 0x100 window
    static std::uint8_t reg_off_(std::uint32_t addr) {
        return static_cast<std::uint8_t>(addr & 0xFFu);
    }

    // Register helpers for DLAB mode (LCR[7])
    bool dlab_() const { return (lcr_ & 0x80u) != 0; }

    void write_tx_(std::uint8_t ch);

private:
    std::mutex mu_;

    // Registers (minimal)
    std::uint8_t rbr_{0}; // receive buffer (read @ 0 when DLAB=0)
    std::uint8_t thr_{0}; // transmit holding (write @ 0 when DLAB=0)
    std::uint8_t ier_{0}; // interrupt enable (offset 1, DLAB=0)

    std::uint8_t iir_{0x01}; // interrupt identification (bit0=1 => no interrupt pending)
    std::uint8_t fcr_{0};    // fifo control (write offset 2)
    std::uint8_t lcr_{0};    // line control (offset 3)
    std::uint8_t mcr_{0};    // modem control (offset 4)
    std::uint8_t lsr_{0x60}; // line status (THRE|TEMT set by default)
    std::uint8_t msr_{0};    // modem status (offset 6)
    std::uint8_t scr_{0};    // scratch (offset 7)

    // Divisor latch (when DLAB=1)
    std::uint8_t dll_{0};
    std::uint8_t dlm_{0};
};

} // namespace remu::devices
