#include <remu/platform/console_input.hpp>

#include <csignal>
#include <cstdlib>
#include <thread>

#include <termios.h>
#include <unistd.h>

#include <remu/common/log.hpp>

namespace remu::platform {

namespace {

constexpr unsigned char kQuitKey = 0x1D; // Ctrl-]

termios g_orig_termios{};
bool g_have_orig_termios = false;

void restore_terminal() {
    if (g_have_orig_termios) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
        g_have_orig_termios = false;
    }
}

void handle_terminating_signal(int sig) {
    restore_terminal();
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

void enable_raw_mode() {
    if (!isatty(STDIN_FILENO)) return;
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) != 0) return;

    g_have_orig_termios = true;
    std::atexit(restore_terminal);
    std::signal(SIGTERM, handle_terminating_signal);
    std::signal(SIGHUP, handle_terminating_signal);

    termios raw = g_orig_termios;
    // Disable canonical mode/echo/signal-generation so keystrokes go straight
    // to the guest UART instead of being line-buffered/echoed/intercepted by
    // the host tty driver.
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO | ISIG | IEXTEN));
    raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL | BRKINT | INPCK | ISTRIP));
    raw.c_oflag &= static_cast<tcflag_t>(~(OPOST));
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void reader_loop(remu::devices::UartNs16550* uart) {
    unsigned char byte = 0;
    while (true) {
        const ssize_t n = ::read(STDIN_FILENO, &byte, 1);
        if (n <= 0) break; // EOF or error on stdin

        if (byte == kQuitKey) {
            restore_terminal();
            std::exit(0);
        }

        uart->inject_rx_byte(byte);
    }
}

} // namespace

void start_console_input(remu::devices::UartNs16550& uart) {
    remu::common::log_info("Console ready. Press Ctrl-] to quit remu.");

    enable_raw_mode();
    std::thread(reader_loop, &uart).detach();
}

} // namespace remu::platform
