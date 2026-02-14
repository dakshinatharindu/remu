#include <remu/common/log.hpp>
#include <iostream>

int main(int argc, char** argv) {
    using namespace remu::common;

    set_log_level(LogLevel::Debug);

    log_info("Starting REMU...");
    log_debug("Debug logging enabled");
    log_warn("This is a warning example");
    log_error("This is an error example");

    log_info("REMUs exiting.");

    return 0;
}
