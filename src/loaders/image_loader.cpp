#include <cerrno>
#include <cstdio>
#include <cstring>
#include <remu/loaders/image_loader.hpp>

namespace remu::loaders {

remu::common::Result<std::vector<std::uint8_t>> read_file_bytes(
    const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        return remu::common::Result<std::vector<std::uint8_t>>::err(
            "fopen failed: " + std::string(std::strerror(errno)));
    }

    if (std::fseek(f, 0, SEEK_END) != 0) {
        std::fclose(f);
        return remu::common::Result<std::vector<std::uint8_t>>::err(
            "fseek failed");
    }

    long size = std::ftell(f);
    if (size < 0) {
        std::fclose(f);
        return remu::common::Result<std::vector<std::uint8_t>>::err(
            "ftell failed");
    }
    std::rewind(f);

    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    if (!data.empty()) {
        std::size_t got = std::fread(data.data(), 1, data.size(), f);
        if (got != data.size()) {
            std::fclose(f);
            return remu::common::Result<std::vector<std::uint8_t>>::err(
                "fread short read");
        }
    }

    std::fclose(f);
    return remu::common::Result<std::vector<std::uint8_t>>::ok(std::move(data));
}

remu::common::Result<void> load_blob(remu::mem::Bus& bus,
                                     std::uint32_t guest_paddr,
                                     std::span<const std::uint8_t> bytes) {
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        auto res =
            bus.write8(guest_paddr + static_cast<std::uint32_t>(i), bytes[i]);
        if (!res) {
            return remu::common::Result<void>::err(
                "bus write8 failed while loading blob");
        }
    }
    return remu::common::Result<void>::ok();
}

}  // namespace remu::loaders
