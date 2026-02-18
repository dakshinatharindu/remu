#pragma once

namespace remu::cpu {

enum class ExecResult {
    Ok = 0,        // normal instruction executed
    Wfi = 2,           // wait-for-interrupt requested
    TrapRaised = 3,    // synchronous trap was raised (ecall/ebreak/etc)
    Fault = 4          // execution fault (bus error, unimplemented)
};

} // namespace remu::cpu
