#include <cerrno>
#include <cstdio>
#include <cstring>
#include <remu/loaders/image_loader.hpp>

namespace remu::loaders {

remu::common::Result<long> load_file_into_guest(remu::mem::Memory& ram,
                                                const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        return remu::common::Result<long>::err(
            "fopen failed: " + std::string(std::strerror(errno)));
    }

    if (std::fseek(f, 0, SEEK_END) != 0) {
        std::fclose(f);
        return remu::common::Result<long>::err("fseek failed");
    }

    long size = std::ftell(f);
    if (size < 0) {
        std::fclose(f);
        return remu::common::Result<long>::err("ftell failed");
    }
    std::rewind(f);

    if (!ram.bytes().empty()) {
        std::size_t got = std::fread(ram.bytes().data(), 1,
                                     static_cast<std::size_t>(size), f);
        if (got != static_cast<std::size_t>(size)) {
            std::fclose(f);
            return remu::common::Result<long>::err(
                "fread short read while loading file into guest");
        }
    }

    std::fclose(f);
    return remu::common::Result<long>::ok(size);
}

}  // namespace remu::loaders
