#include <remu/common/log.hpp>
#include <remu/loaders/image_loader.hpp>
#include <remu/platform/virt.hpp>
#include <remu/runtime/runner.hpp>

namespace remu::runtime {
int run(const Arguments& args) {
    using remu::common::log_error;
    using remu::common::log_info;

    remu::platform::VirtMachine machine(
        static_cast<uint32_t>(args.mem_size_bytes));

    auto kernel_bytes_res = remu::loaders::read_file_bytes(args.kernel_path);
    if (!kernel_bytes_res) {
        log_error("Failed to read kernel image: " + kernel_bytes_res.error());
        return 1;
    }

    constexpr std::uint32_t KERNEL_LOAD_ADDR =
        0x8000'0000;  // typical virt RAM base
    auto ok = remu::loaders::load_blob(machine.bus(), KERNEL_LOAD_ADDR,
                                       kernel_bytes_res.value());
    if (!ok) {
        log_error("Failed to load kernel into guest RAM");
        return 1;
    }
    log_info("Kernel loaded into guest RAM at 0x8000000");

    // check that the first few bytes of RAM match the kernel image (sanity
    // check)
    for (size_t i = 0;
         i < std::min<size_t>(16, kernel_bytes_res.value().size()); ++i) {
        uint8_t res;
        auto res_opt = machine.bus().read8(
            KERNEL_LOAD_ADDR + static_cast<std::uint32_t>(i), res);
        if (!res_opt) {
            log_error("Failed to read back from guest RAM for verification");
            return 1;
        }
        if (res != kernel_bytes_res.value()[i]) {
            log_error(
                "Verification failed: RAM content does not match kernel image");
            return 1;
        }
    }
    log_info("Kernel image verification passed");

    return 0;
}
}  // namespace remu::runtime