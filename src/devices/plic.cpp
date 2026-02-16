#include <remu/devices/plic.hpp>

namespace remu::devices {

namespace {
// PLIC register layout (standard-ish, matches QEMU virt layout style)

// Priority: 0x0000 + 4*irq_id
constexpr std::uint32_t PRIORITY_BASE = 0x0000;

// Pending: 0x1000 + 4*word
constexpr std::uint32_t PENDING_BASE  = 0x1000;

// Enable (hart0): 0x2000 + 4*word
constexpr std::uint32_t ENABLE_BASE   = 0x2000;

// Context for hart0 M-mode (virt commonly uses context 1 for M-mode hart0)
// Threshold: 0x200000 + 0x1000*context + 0x0
// Claim/Complete: 0x200000 + 0x1000*context + 0x4
constexpr std::uint32_t CONTEXT_BASE  = 0x200000;
constexpr std::uint32_t CONTEXT_STRIDE= 0x1000;
constexpr std::uint32_t CTX_M_HART0   = 1; // commonly used by QEMU virt

constexpr std::uint32_t THRESHOLD_OFF = 0x0;
constexpr std::uint32_t CLAIM_OFF     = 0x4;

inline bool valid_irq(std::uint32_t id) {
    return id >= 1 && id <= Plic::kMaxIrq;
}
} // namespace

Plic::Plic() {
    // Default: all priorities 0, disabled, no pending.
    priority_.fill(0);
    pending_ = 0;
    enable0_ = 0;
    threshold0_ = 0;
}

void Plic::raise_irq(std::uint32_t irq_id) {
    if (!valid_irq(irq_id)) return;
    std::lock_guard<std::mutex> lock(mu_);
    pending_ |= (1ull << irq_id);
}

void Plic::clear_irq(std::uint32_t irq_id) {
    if (!valid_irq(irq_id)) return;
    std::lock_guard<std::mutex> lock(mu_);
    pending_ &= ~(1ull << irq_id);
}

bool Plic::has_pending_for_hart0() const {
    std::lock_guard<std::mutex> lock(mu_);
    return pick_best_irq_() != 0;
}

std::uint32_t Plic::pick_best_irq_() const {
    // Choose enabled & pending IRQ with priority > threshold.
    // If tie, choose lowest ID (simple deterministic).
    std::uint64_t cand = pending_ & enable0_;

    std::uint32_t best_id = 0;
    std::uint32_t best_pri = 0;

    for (std::uint32_t id = 1; id <= kMaxIrq; ++id) {
        if ((cand & (1ull << id)) == 0) continue;

        const std::uint32_t pri = priority_[id];
        if (pri <= threshold0_) continue;

        if (pri > best_pri) {
            best_pri = pri;
            best_id = id;
        } else if (pri == best_pri && best_id != 0 && id < best_id) {
            best_id = id;
        } else if (pri == best_pri && best_id == 0) {
            best_id = id;
        }
    }

    return best_id; // 0 if none
}

bool Plic::read(std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t& out) {
    if (width_bytes != 4) return false;

    std::lock_guard<std::mutex> lock(mu_);
    out = 0;

    const std::uint32_t off = off_(addr);

    // 1) Priority
    if (off >= PRIORITY_BASE && off < PRIORITY_BASE + 4 * (kMaxIrq + 1)) {
        const std::uint32_t id = (off - PRIORITY_BASE) / 4;
        if (id <= kMaxIrq) out = priority_[id];
        return true;
    }

    // 2) Pending (read-only)
    if (off >= PENDING_BASE && off < PENDING_BASE + 8) {
        const std::uint32_t word = (off - PENDING_BASE) / 4; // word 0 or 1 (we only need 64 bits)
        const std::uint32_t lo = static_cast<std::uint32_t>(pending_ & 0xFFFF'FFFFull);
        const std::uint32_t hi = static_cast<std::uint32_t>((pending_ >> 32) & 0xFFFF'FFFFull);
        out = (word == 0) ? lo : hi;
        return true;
    }

    // 3) Enable (hart0)
    if (off >= ENABLE_BASE && off < ENABLE_BASE + 8) {
        const std::uint32_t word = (off - ENABLE_BASE) / 4;
        const std::uint32_t lo = static_cast<std::uint32_t>(enable0_ & 0xFFFF'FFFFull);
        const std::uint32_t hi = static_cast<std::uint32_t>((enable0_ >> 32) & 0xFFFF'FFFFull);
        out = (word == 0) ? lo : hi;
        return true;
    }

    // 4) Threshold / Claim for hart0 M-mode context
    const std::uint32_t ctx_base = CONTEXT_BASE + CTX_M_HART0 * CONTEXT_STRIDE;
    if (off == ctx_base + THRESHOLD_OFF) {
        out = threshold0_;
        return true;
    }
    if (off == ctx_base + CLAIM_OFF) {
        const std::uint32_t id = pick_best_irq_();
        if (id != 0) {
            // Claim clears pending (typical PLIC behavior)
            pending_ &= ~(1ull << id);
        }
        out = id;
        return true;
    }

    // Unhandled reads return 0
    out = 0;
    return true;
}

bool Plic::write(std::uint32_t addr, std::uint32_t width_bytes, std::uint32_t val) {
    if (width_bytes != 4) return false;

    std::lock_guard<std::mutex> lock(mu_);
    const std::uint32_t off = off_(addr);

    // 1) Priority
    if (off >= PRIORITY_BASE && off < PRIORITY_BASE + 4 * (kMaxIrq + 1)) {
        const std::uint32_t id = (off - PRIORITY_BASE) / 4;
        if (id == 0 || id > kMaxIrq) return true;
        priority_[id] = val & 0x7; // keep it small; real PLIC supports more
        return true;
    }

    // 2) Enable (hart0)
    if (off >= ENABLE_BASE && off < ENABLE_BASE + 8) {
        const std::uint32_t word = (off - ENABLE_BASE) / 4;
        if (word == 0) {
            enable0_ = (enable0_ & 0xFFFF'FFFF'0000'0000ull) | static_cast<std::uint64_t>(val);
        } else {
            enable0_ = (enable0_ & 0x0000'0000'FFFF'FFFFull) | (static_cast<std::uint64_t>(val) << 32);
        }
        // Never enable IRQ0
        enable0_ &= ~1ull;
        return true;
    }

    // 3) Threshold / Complete for hart0 M-mode context
    const std::uint32_t ctx_base = CONTEXT_BASE + CTX_M_HART0 * CONTEXT_STRIDE;
    if (off == ctx_base + THRESHOLD_OFF) {
        threshold0_ = val & 0x7;
        return true;
    }
    if (off == ctx_base + CLAIM_OFF) {
        // Complete: writing the claimed ID indicates end of interrupt service.
        // In minimal model, nothing required.
        (void)val;
        return true;
    }

    // Ignore other writes
    return true;
}

} // namespace rvemu::devices
