#include <remu/devices/uart.hpp>

#include <cstdio>
#include <cstring>

namespace remu::devices {

namespace {
// Standard 16550 offsets (byte registers)
constexpr std::uint8_t REG_RBR_THR_DLL = 0x00;
constexpr std::uint8_t REG_IER_DLM     = 0x01;
constexpr std::uint8_t REG_IIR_FCR     = 0x02;
constexpr std::uint8_t REG_LCR         = 0x03;
constexpr std::uint8_t REG_MCR         = 0x04;
constexpr std::uint8_t REG_LSR         = 0x05;
constexpr std::uint8_t REG_MSR         = 0x06;
constexpr std::uint8_t REG_SCR         = 0x07;

// LSR bits
constexpr std::uint8_t LSR_DR   = 1u << 0; // data ready
constexpr std::uint8_t LSR_THRE = 1u << 5; // transmit holding register empty
constexpr std::uint8_t LSR_TEMT = 1u << 6; // transmitter empty
} // namespace

UartNs16550::UartNs16550() {
    // By default, transmitter is empty.
    lsr_ = static_cast<std::uint8_t>(LSR_THRE | LSR_TEMT);
    iir_ = 0x01; // no interrupt pending
}

void UartNs16550::write_tx_(std::uint8_t ch) {
    // Minimal: print to host stdout immediately.
    std::putchar(static_cast<int>(ch));
    std::fflush(stdout);

    // Keep THRE/TEMT set (we model TX as always-ready).
    lsr_ |= static_cast<std::uint8_t>(LSR_THRE | LSR_TEMT);
}

void UartNs16550::inject_rx_byte(std::uint8_t byte) {
    std::lock_guard<std::mutex> lock(mu_);
    rbr_ = byte;
    lsr_ |= LSR_DR;
    // Interrupts not modeled yet; iir_ stays "no interrupt".
}

bool UartNs16550::read(std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t& out) {
    std::lock_guard<std::mutex> lock(mu_);

    out = 0;
    if (width_bytes != 1 && width_bytes != 2 && width_bytes != 4) return false;

    // Little-endian assembly from successive byte reads
    for (std::uint32_t i = 0; i < width_bytes; ++i) {
        std::uint8_t b = 0;
        if (!read8_(addr + i, b)) return false;
        out |= (static_cast<std::uint32_t>(b) << (8u * i));
    }
    return true;
}

bool UartNs16550::write(std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t val) {
    std::lock_guard<std::mutex> lock(mu_);

    if (width_bytes != 1 && width_bytes != 2 && width_bytes != 4) return false;

    // Little-endian disassembly into successive byte writes
    for (std::uint32_t i = 0; i < width_bytes; ++i) {
        const std::uint8_t b = static_cast<std::uint8_t>((val >> (8u * i)) & 0xFFu);
        if (!write8_(addr + i, b)) return false;
    }
    return true;
}

bool UartNs16550::read8_(std::uint32_t addr, std::uint8_t& out) {
    const std::uint8_t off = reg_off_(addr) & 0x07u; // we only implement 0..7

    switch (off) {
        case REG_RBR_THR_DLL:
            if (dlab_()) {
                out = dll_;
            } else {
                out = rbr_;
                // reading RBR clears DR
                lsr_ = static_cast<std::uint8_t>(lsr_ & ~LSR_DR);
            }
            return true;

        case REG_IER_DLM:
            out = dlab_() ? dlm_ : ier_;
            return true;

        case REG_IIR_FCR:
            // read = IIR. (Write is FCR)
            out = iir_;
            return true;

        case REG_LCR:
            out = lcr_;
            return true;

        case REG_MCR:
            out = mcr_;
            return true;

        case REG_LSR:
            // Keep THRE/TEMT set (always ready to transmit in this minimal model)
            lsr_ |= static_cast<std::uint8_t>(LSR_THRE | LSR_TEMT);
            out = lsr_;
            return true;

        case REG_MSR:
            out = msr_;
            return true;

        case REG_SCR:
            out = scr_;
            return true;

        default:
            out = 0;
            return true;
    }
}

bool UartNs16550::write8_(std::uint32_t addr, std::uint8_t val) {
    const std::uint8_t off = reg_off_(addr) & 0x07u;

    switch (off) {
        case REG_RBR_THR_DLL:
            if (dlab_()) {
                dll_ = val;
            } else {
                thr_ = val;
                write_tx_(val);
            }
            return true;

        case REG_IER_DLM:
            if (dlab_()) {
                dlm_ = val;
            } else {
                ier_ = val;
                // Interrupts not implemented; keep iir_ as "no pending".
            }
            return true;

        case REG_IIR_FCR:
            // write = FCR
            fcr_ = val;
            // If FIFOs are "cleared", clear DR bit (minimal behavior)
            if (val & 0x02u) { // clear RX FIFO
                lsr_ = static_cast<std::uint8_t>(lsr_ & ~LSR_DR);
            }
            return true;

        case REG_LCR:
            lcr_ = val;
            return true;

        case REG_MCR:
            mcr_ = val;
            return true;

        case REG_LSR:
            // LSR is read-only in real hardware; ignore writes.
            return true;

        case REG_MSR:
            // MSR is mostly read-only; ignore.
            return true;

        case REG_SCR:
            scr_ = val;
            return true;

        default:
            return true;
    }
}

} // namespace rvemu::devices
