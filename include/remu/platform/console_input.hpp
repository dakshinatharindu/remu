#pragma once

#include <remu/devices/uart.hpp>

namespace remu::platform {

// Puts the host terminal into raw mode (if stdin is a TTY) and starts a
// background thread that forwards every byte typed on stdin into the guest
// UART's receive register, byte by byte, as they arrive.
//
// Raw mode disables host-side line buffering/echo/signal keys so the guest
// console behaves like a real serial terminal (e.g. Ctrl-C reaches the guest
// shell instead of killing the host process). Terminal settings are restored
// automatically on normal process exit or SIGTERM/SIGHUP.
//
// Press Ctrl-] to quit remu directly.
void start_console_input(remu::devices::UartNs16550& uart);

} // namespace remu::platform
