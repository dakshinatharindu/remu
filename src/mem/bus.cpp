#include <remu/mem/bus.hpp>
#include <remu/mem/memory.hpp>   // your Memory class (direct RAM)

namespace remu::mem {

void Bus::map_ram(std::uint32_t base, std::uint32_t size, Memory& ram) {
    regions_.push_back(Region::make_ram(base, size, &ram));
}

void Bus::map_mmio(std::uint32_t base, std::uint32_t size, MmioDevice& dev) {
    regions_.push_back(Region::make_mmio(base, size, &dev));
}

Region* Bus::find_region_(std::uint32_t addr, std::uint32_t len) {
    for (auto& r : regions_) {
        if (r.contains(addr, len)) return &r;
    }
    return nullptr;
}

const Region* Bus::find_region_(std::uint32_t addr, std::uint32_t len) const {
    for (const auto& r : regions_) {
        if (r.contains(addr, len)) return &r;
    }
    return nullptr;
}

bool Bus::mmio_read_(MmioDevice& dev, std::uint32_t addr, std::uint32_t width, std::uint32_t& out) {
    return dev.read(addr, width, out);
}

bool Bus::mmio_write_(MmioDevice& dev, std::uint32_t addr, std::uint32_t width, std::uint32_t val) {
    return dev.write(addr, width, val);
}

// ---------------- Reads ----------------

bool Bus::read8(std::uint32_t addr, std::uint8_t& out) {
    Region* r = find_region_(addr, 1);
    if (!r) return false;

    if (r->kind == Region::Kind::Ram) {
        return r->ram->read8(addr, out);
    } else {
        std::uint32_t tmp = 0;
        if (!mmio_read_(*r->mmio, addr, 1, tmp)) return false;
        out = static_cast<std::uint8_t>(tmp & 0xFFu);
        return true;
    }
}

bool Bus::read16(std::uint32_t addr, std::uint16_t& out) {
    Region* r = find_region_(addr, 2);
    if (!r) return false;

    if (r->kind == Region::Kind::Ram) {
        return r->ram->read16(addr, out);
    } else {
        std::uint32_t tmp = 0;
        if (!mmio_read_(*r->mmio, addr, 2, tmp)) return false;
        out = static_cast<std::uint16_t>(tmp & 0xFFFFu);
        return true;
    }
}

bool Bus::read32(std::uint32_t addr, std::uint32_t& out) {
    Region* r = find_region_(addr, 4);
    if (!r) return false;

    if (r->kind == Region::Kind::Ram) {
        return r->ram->read32(addr, out);
    } else {
        std::uint32_t tmp = 0;
        if (!mmio_read_(*r->mmio, addr, 4, tmp)) return false;
        out = tmp;
        return true;
    }
}

// ---------------- Writes ----------------

bool Bus::write8(std::uint32_t addr, std::uint8_t val) {
    Region* r = find_region_(addr, 1);
    if (!r) return false;

    if (r->kind == Region::Kind::Ram) {
        return r->ram->write8(addr, val);
    } else {
        return mmio_write_(*r->mmio, addr, 1, static_cast<std::uint32_t>(val));
    }
}

bool Bus::write16(std::uint32_t addr, std::uint16_t val) {
    Region* r = find_region_(addr, 2);
    if (!r) return false;

    if (r->kind == Region::Kind::Ram) {
        return r->ram->write16(addr, val);
    } else {
        return mmio_write_(*r->mmio, addr, 2, static_cast<std::uint32_t>(val));
    }
}

bool Bus::write32(std::uint32_t addr, std::uint32_t val) {
    Region* r = find_region_(addr, 4);
    if (!r) return false;

    if (r->kind == Region::Kind::Ram) {
        return r->ram->write32(addr, val);
    } else {
        return mmio_write_(*r->mmio, addr, 4, val);
    }
}

} // namespace remu::mem
