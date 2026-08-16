# TizenRT kernel bughunt ledger

## Current review — 2026-08-16 UTC

### Valid retained candidate

#### BUG-20260814-001 — static defect, runtime unreproduced

- **Location:** `os/arch/arm/src/qemu-virt/qemu_timer.c:195-204`
- **Defect:** `up_timer_disable()` and `up_timer_enable()` are declared `int` by `os/include/tinyara/arch.h`, whose documented contract requires `OK` or a negated errno, but both functions fall through without a return statement.
- **Reachable checked caller:** `os/pm/pm_idle.c:212-215` returns `up_timer_disable()` directly from `disable_systick()`.
- **Status:** retained as a static defect; no matching qemu-virt cross-build or deterministic runtime return-value observation is available, so it is **not** marked reproduced.
- **Evidence limitation:** `arm-none-eabi-gcc` and a matching bughunt ELF are absent. No fix has been applied, pushed, or proposed.

### Removed invalid candidate

- **BUG-20260814-002 — deleted (invalid premise).** The candidate asserted that an interrupted ordinary `pthread_cond_wait()` could make `pthread_sem_take()` return `EINTR`, leaving `cond->waiters` stale. The actual wrapper in `os/kernel/pthread/pthread_initialize.c:132-157` loops on `sem_wait()` while `errno == EINTR` and returns `OK` only after successfully taking the semaphore. Consequently, the proposed signal-based reproduction cannot reach the claimed `EINTR` failure path. Its artifacts under `.hermes/kernel-bug-repros/BUG-20260814-002/` were removed.

### Scope and upstream state

- `upstream/master`: `93cde68110a26df205ac4f0f536cff70699f1bc6`
- `feat/qemu-virt-gdb-awareness`: `bd5069ad3650600fb5b0aab07ca66106362817b2`
- This ledger belongs only to the isolated `qemu-virt_bughunt` worktree.
- No user worktree or feature-branch source was changed. No push or PR was performed.
