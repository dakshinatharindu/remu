#include <remu/devices/clint.hpp>

namespace remu::devices {

namespace {
constexpr std::uint32_t MSIP0_OFF      = 0x0000;
constexpr std::uint32_t MTIMECMP0_OFF  = 0x4000; // low 32
constexpr std::uint32_t MTIMECMP0H_OFF = 0x4004; // high 32
constexpr std::uint32_t MTIME_OFF      = 0xBFF8; // low 32
constexpr std::uint32_t MTIMEH_OFF     = 0xBFFC; // high 32
} // namespace

Clint::Clint() = default;

void Clint::tick(std::uint64_t cycles) {
    std::lock_guard<std::mutex> lock(mu_);
    mtime_ += cycles;
}

bool Clint::msip_pending() const {
    std::lock_guard<std::mutex> lock(mu_);
    return (msip0_ & 0x1u) != 0;
}

bool Clint::mtip_pending() const {
    std::lock_guard<std::mutex> lock(mu_);
    return mtime_ >= mtimecmp0_;
}

std::uint64_t Clint::mtime() const {
    std::lock_guard<std::mutex> lock(mu_);
    return mtime_;
}

std::uint64_t Clint::mtimecmp() const {
    std::lock_guard<std::mutex> lock(mu_);
    return mtimecmp0_;
}

bool Clint::read(std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t& out) {
    std::lock_guard<std::mutex> lock(mu_);
    out = 0;

    // CLINT is usually accessed as 32-bit words on RV32
    if (width_bytes != 4) return false;

    const std::uint32_t off = off_(addr);

    switch (off) {
        case MSIP0_OFF:
            out = msip0_;
            return true;

        case MTIMECMP0_OFF:
            out = static_cast<std::uint32_t>(mtimecmp0_ & 0xFFFF'FFFFull);
            return true;

        case MTIMECMP0H_OFF:
            out = static_cast<std::uint32_t>((mtimecmp0_ >> 32) & 0xFFFF'FFFFull);
            return true;

        case MTIME_OFF:
            out = static_cast<std::uint32_t>(mtime_ & 0xFFFF'FFFFull);
            return true;

        case MTIMEH_OFF:
            out = static_cast<std::uint32_t>((mtime_ >> 32) & 0xFFFF'FFFFull);
            return true;

        default:
            out = 0;
            return true; // unmapped reads return 0 in many simple models
    }
}

bool Clint::write(std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t val) {
    std::lock_guard<std::mutex> lock(mu_);

    if (width_bytes != 4) return false;

    const std::uint32_t off = off_(addr);

    switch (off) {
        case MSIP0_OFF:
            msip0_ = (val & 0x1u);
            return true;

        case MTIMECMP0_OFF: {
            // Typical safe programming pattern is write high then low (or vice versa).
            // We'll just update the low half.
            const std::uint64_t hi = (mtimecmp0_ & 0xFFFF'FFFF'0000'0000ull);
            mtimecmp0_ = hi | static_cast<std::uint64_t>(val);
            return true;
        }

        case MTIMECMP0H_OFF: {
            const std::uint64_t lo = (mtimecmp0_ & 0x0000'0000'FFFF'FFFFull);
            mtimecmp0_ = (static_cast<std::uint64_t>(val) << 32) | lo;
            return true;
        }

        case MTIME_OFF: {
            const std::uint64_t hi = (mtime_ & 0xFFFF'FFFF'0000'0000ull);
            mtime_ = hi | static_cast<std::uint64_t>(val);
            return true;
        }

        case MTIMEH_OFF: {
            const std::uint64_t lo = (mtime_ & 0x0000'0000'FFFF'FFFFull);
            mtime_ = (static_cast<std::uint64_t>(val) << 32) | lo;
            return true;
        }

        default:
            return true;
    }
}

} // namespace rvemu::devices
