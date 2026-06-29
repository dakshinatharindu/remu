# remu

A RISC-V emulator written in C++20, capable of booting a Linux kernel. It implements a RV32IMA hart running in machine mode, emulating the QEMU `virt` machine layout with enough peripheral fidelity for Linux early bring-up.

## Features

- **RV32IMA** — base integer (I), multiply/divide (M), and atomic (A) extensions
- **Machine-mode CSRs** — `mstatus`, `mtvec`, `mepc`, `mcause`, `mip`, `mie`, `mhartid`, cycle/instret counters
- **Trap handling** — synchronous exceptions (illegal instruction, misaligned access, ecall) and M-mode timer/software/external interrupts
- **NS16550 UART** — early `printk` output over stdout
- **CLINT** — `mtime`, `mtimecmp`, `msip` for timer and software interrupts
- **PLIC** — priority/pending/enable/claim registers for external interrupt routing
- **Device tree** — DTB blob loaded and passed to the kernel via `a1`

---

## Building

### Prerequisites

- CMake >= 3.20
- A C++20-capable compiler (GCC 11+ or Clang 13+)

### Steps

```bash
cmake -B build
cmake --build build -j$(nproc)
```

The binary is produced at `build/apps/remu/remu`.

### Build options

| Option | Default | Description |
|---|---|---|
| `REMU_ENABLE_ASAN` | OFF | Enable AddressSanitizer |
| `REMU_ENABLE_UBSAN` | OFF | Enable UndefinedBehaviorSanitizer |
| `REMU_ENABLE_TSAN` | OFF | Enable ThreadSanitizer |
| `REMU_ENABLE_TRACE` | OFF | Enable per-instruction trace hooks |
| `REMU_ENABLE_LOG` | ON | Enable runtime logging |

Pass options at configure time:

```bash
cmake -B build -DREMU_ENABLE_ASAN=ON -DREMU_ENABLE_TRACE=ON
cmake --build build -j$(nproc)
```

---

## Running a Linux kernel

```bash
./build/apps/remu/remu -k <kernel_image> [-d <dtb>] [-m <mem_size>]
```

| Flag | Description |
|---|---|
| `-k <path>` | Path to the kernel image (required) |
| `-d <path>` | Path to a DTB file (default: `resources/dtb/virt.dtb`) |
| `-m <size>` | RAM size — bytes, or with suffix K/M/G (default: `128M`) |

### Example

```bash
./build/apps/remu/remu -k Image -d resources/dtb/virt.dtb -m 128M
```

The kernel image should be a raw binary (e.g. `Image` for arm64-style flat image, or a raw ELF loaded at `0x80000000`). The DTB is placed just above the top of RAM and its address is passed to the kernel in register `a1`, following the Linux RISC-V boot convention.

---

## Project structure

```
remu/
├── apps/remu/          # Entry point (main.cpp, argument parsing)
├── include/remu/
│   ├── common/         # Logging, Result type
│   ├── cpu/            # CPU state, registers, CSRs, decoder, exceptions
│   ├── devices/        # UART, CLINT, PLIC
│   ├── loaders/        # Kernel/DTB image loading
│   ├── mem/            # Bus, Memory, MMIO region abstraction
│   ├── platform/       # VirtMachine (wires everything together)
│   └── runtime/        # Sim loop, runner, CLI arguments
├── src/                # Implementations (mirrors include/ layout)
├── resources/dtb/      # Pre-built DTB blobs and source DTS
└── cmake/              # Build options, warnings, sanitizer flags
```

---

## Architecture

### Memory map

Follows the QEMU `virt` layout:

| Region | Base address | Size |
|---|---|---|
| PLIC | `0x0C000000` | 64 MiB |
| UART (NS16550) | `0x10000000` | 256 B |
| CLINT | `0x11000000` | 64 KiB |
| RAM | `0x80000000` | configurable |
| DTB | `RAM_BASE + RAM_SIZE` | 2 MiB |

### Components

**`cpu/`** — the hart

- `Cpu` holds architectural state: `pc`, privilege mode, `RegFile` (x0–x31), `CsrFile`, and the LR/SC reservation register for atomics.
- `decode_rv32()` decodes a 32-bit word into a `DecodedInsn` (kind, format, rd/rs1/rs2, immediate).
- Execute is split by extension: `execute_rv32i`, `execute_rv32m`, `execute_rv32a`.
- `trap.cpp` handles exception and interrupt delivery into M-mode, updating `mepc`, `mcause`, `mtval`, and `mstatus.MIE/MPIE`.

**`mem/`** — address space

- `Bus` holds a flat list of `Region`s (each is either a RAM slice or an MMIO device). Reads and writes walk the list to find the matching region.
- `Memory` is a plain byte buffer with a base address — used for both RAM and the DTB window.
- `MmioDevice` is the interface all peripherals implement (`read` / `write` with byte-width dispatch).

**`devices/`** — peripherals

- `UartNs16550` — NS16550A-compatible UART. TX writes go to stdout immediately. LSR keeps THRE/TEMT set so the kernel never stalls waiting for the transmit buffer. RX bytes can be injected via `inject_rx_byte()`.
- `Clint` — tracks `mtime` (incremented every `tick()`), `mtimecmp`, and `msip`. Asserts `MTIP`/`MSIP` bits into `mip` via `VirtMachine::tick()`.
- `Plic` — supports up to 64 IRQ lines. Implements priority, pending, enable, threshold, claim, and complete registers for hart0 in M-mode. Asserts `MEIP` when a qualifying interrupt is pending.

**`platform/`** — machine assembly

- `VirtMachine` owns all components (RAM, DTB memory, bus, UART, CLINT, PLIC) and wires them onto the bus at their fixed base addresses. Its `tick()` method propagates CLINT and PLIC interrupt state into the CPU's `mip` register each cycle.

**`runtime/`** — simulation loop

- `Sim` is a pure interpreter: each call to `step()` fetches a 32-bit instruction from the bus, decodes it, executes it, handles any pending exception or interrupt, and calls `VirtMachine::tick()`. `run()` loops until a stop condition (illegal instruction, bus fault, instruction limit).
- `runner.cpp` sets up `VirtMachine`, loads the kernel and DTB images, sets `a0`/`a1` per the Linux boot protocol, and starts `Sim::run()`.

### Boot flow

1. `main()` parses CLI flags into `Arguments`.
2. `runner::run()` constructs a `VirtMachine` and a `Cpu`.
3. The kernel image is loaded into RAM at `0x80000000`; the DTB blob is loaded at `RAM_BASE + RAM_SIZE`.
4. `cpu.set_boot_args(hartid=0, dtb_ptr)` sets `a0 = 0`, `a1 = dtb_base`.
5. `cpu.reset(0x80000000)` sets `pc` and starts in M-mode.
6. `Sim::run()` executes the fetch–decode–execute–trap loop indefinitely until the kernel halts or an unrecoverable fault occurs.
7. Each iteration calls `VirtMachine::tick()` to advance `mtime` and update interrupt pending bits.
