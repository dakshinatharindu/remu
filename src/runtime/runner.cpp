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

    auto size =
        remu::loaders::load_file_into_guest(machine.ram(), args.kernel_path);
    if (!size) {
        log_error("Failed to load kernel into guest RAM: " + size.error());
        return 1;
    }
    log_info("Kernel loaded into guest RAM at 0x8000000 (size: " +
             std::to_string(size.value()) + " bytes)");

    return 0;
}
}  // namespace remu::runtime