#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <remu/common/log.hpp>
#include <remu/loaders/image_loader.hpp>
#include <remu/platform/virt.hpp>
#include <remu/runtime/arguments.hpp>
#include <remu/runtime/runner.hpp>
#include <string>
#include <string_view>

using remu::common::log_error;
using remu::common::log_info;

namespace {

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " -k <kernel_image> [-m <mem_size>]\n"
              << "  -k <path>     Kernel image path (required)\n"
              << "  -m <size>     Memory size (e.g. 128M, 256M, 1G, or bytes). "
                 "Default: 128M\n"
              << "  -h            Show help\n";
}

std::optional<std::uint64_t> parse_mem_size(std::string_view s) {
    // Accept: "134217728" (bytes) or "128M"/"256m" or "1G"/"1g" or "64K"
    if (s.empty()) return std::nullopt;

    char suffix = '\0';
    std::string number_part;

    // If last char is alpha, treat it as suffix
    if (std::isalpha(static_cast<unsigned char>(s.back()))) {
        suffix = static_cast<char>(
            std::toupper(static_cast<unsigned char>(s.back())));
        number_part = std::string(s.substr(0, s.size() - 1));
        if (number_part.empty()) return std::nullopt;
    } else {
        number_part = std::string(s);
    }

    char* end = nullptr;
    errno = 0;
    unsigned long long base = std::strtoull(number_part.c_str(), &end, 10);
    if (errno != 0 || end == number_part.c_str() || *end != '\0') {
        return std::nullopt;
    }

    std::uint64_t multiplier = 1;
    switch (suffix) {
        case '\0':
            multiplier = 1;
            break;
        case 'K':
            multiplier = 1024ull;
            break;
        case 'M':
            multiplier = 1024ull * 1024;
            break;
        case 'G':
            multiplier = 1024ull * 1024 * 1024;
            break;
        default:
            return std::nullopt;
    }

    return static_cast<std::uint64_t>(base) * multiplier;
}

bool parse_args(int argc, char** argv, remu::runtime::Arguments& out) {
    if (argc <= 1) return false;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        if (std::strcmp(arg, "-h") == 0 || std::strcmp(arg, "--help") == 0) {
            return false;  // will print usage in main
        } else if (std::strcmp(arg, "-k") == 0) {
            if (i + 1 >= argc) {
                log_error("Missing value after -k");
                return false;
            }
            out.kernel_path = argv[++i];
        } else if (std::strcmp(arg, "-m") == 0) {
            if (i + 1 >= argc) {
                log_error("Missing value after -m");
                return false;
            }
            auto parsed = parse_mem_size(argv[++i]);
            if (!parsed || *parsed == 0) {
                log_error(
                    "Invalid memory size for -m (examples: 128M, 1G, "
                    "134217728)");
                return false;
            }
            out.mem_size_bytes = *parsed;
        } else if (std::strcmp(arg, "-d") == 0) {
            if (i + 1 >= argc) {
                log_error("Missing value after -d");
                return false;
            }
            out.dtb_path = argv[++i];
        } else {
            log_error(std::string("Unknown argument: ") + arg);
            return false;
        }
    }

    if (out.kernel_path.empty()) {
        log_error("Kernel image is required. Use -k <path>.");
        return false;
    }

    return true;
}

}  // namespace

int main(int argc, char** argv) {
    remu::common::set_log_level(remu::common::LogLevel::Info);

    remu::runtime::Arguments args;
    if (!parse_args(argc, argv, args)) {
        print_usage(argv[0]);
        return 1;
    }

    log_info(std::string("Kernel: ") + args.kernel_path);
    log_info("Memory bytes: " + std::to_string(args.mem_size_bytes));
    log_info(std::string("DTB: ") + args.dtb_path);

    remu::runtime::run(args);

    return 0;
}
