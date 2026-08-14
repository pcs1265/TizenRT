# BUG-20260814-001

## Candidate and status

- **Status:** unreproduced candidate (not confirmed).
- **Candidate:** `up_timer_disable()` and `up_timer_enable()` in `os/arch/arm/src/qemu-virt/qemu_timer.c` are declared `int` but fall off the end without returning a value. The qemu-virt build emits `-Wreturn-type` at both functions. A caller can therefore observe an undefined return value and compiler behavior is undefined.
- **Why new:** `.hermes/kernel-bugs.md` and `.hermes/kernel-bug-repros/` were absent at the start of this run; no prior BUG-ID or same scenario existed.
- **Why not confirmed:** QEMU boot and GDB invocation succeeded, but a deterministic incorrect return value could not be demonstrated. The GDB inferior call returned 0 and the serial path did not expose a stable value. Do not label this reproduced.

## Reproduction files

- `hello_main.patch`: C reproduction calling both timer functions and printing return values.
- `helloxx_main.patch`: C++ reproduction of the same call path.
- `config.txt`: qemu-virt `dramboot_flat` settings used.
- `qemu.log`: boot-to-`TASH>>` serial evidence.
- `gdb.log`: commands, breakpoints, task/stack output, registers, and result.

The patches are deliberately stored, not applied to the final kernel-bughunt tree.

## Git bases

- `upstream/master`: `93cde68110a26df205ac4f0f536cff70699f1bc6`
- `feat/qemu-virt-gdb-awareness`: `bd5069ad3650600fb5b0aab07ca66106362817b2`
- `qemu-virt_bughunt` reproduction commit: `2a33bf614fcf47262f715e5bffe1291ecbb34623`.
- Final tree preserves `build/configs/qemu-virt/tools/tizenrt_gdb.py`, `auto_symbol_loader.py`, and `launch.json`, plus the existing qemu-virt delta.

## Apache NuttX comparison

- **Classification:** `unknown` / `different-design`.
- Apache NuttX master uses its timer/clock architecture APIs and does not provide this exact qemu-virt `up_timer_disable()`/`up_timer_enable()` implementation in the compared official paths. No NuttX fix was treated as evidence that TizenRT is fixed.
- **Potential fix direction:** change both functions to return the result of `arm_arch_timer_enable(...)` (or explicitly return `OK`/the documented error), then add a caller test; this run did not modify kernel code.
