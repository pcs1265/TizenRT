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

## Monitor cycle — 2026-08-16 01:00 UTC

- **Fetched and diffed:** `git fetch --prune upstream origin` succeeded. `upstream/master` remains `93cde68110a26df205ac4f0f536cff70699f1bc6`; `git log ef1531ce..upstream/master` is empty and a path-scoped diff over scheduler, pthread, semaphore, mutex, condition, cancellation, SMP, task, qemu-virt, and public-kernel headers has zero changed paths. The upstream tip is the unrelated LittleFS `system_file` change.
- **Deduplication/result:** historical waiter, cancellation, semaphore-holder, priority-restoration, task/MQ, waitpid, and bitmask fixes were rechecked. BUG-20260814-002 remains rejected: `pthread_sem_take()` retries `sem_wait()` on `EINTR` (`pthread_initialize.c:140-150`), so its alleged interrupted-return path is unreachable. No root cause distinct from retained BUG-20260814-001 was found. BUG-001 remains **unreproduced static candidate**, and no TizenRT fix is merged in `upstream/master` to mark it fixed.
- **Actual QEMU/GDB observation:** both helper scripts passed `python3 -m py_compile`. A live `./run_qemu.sh` boot reached S1 boot, kernel CRC pass, virtio-blk, SMARTFS `/mnt`, `/dev/virtblk0`, and `TASH>>`. `gdb-multiarch` connected to `:1234`, sourced `auto_symbol_loader.py` and `tizenrt_gdb.py`, and invoked `tizenrt current/tasks/stack/waiters/held`. There is no matching `build/output/bin/tinyara`; auto-loading therefore raised its existing `NoneType.string` error and OS awareness observed 0 symbol-backed tasks, waiters, and held semaphores (raw `pc=0x40114c00`, `sp=0x4015122c`, `lr=0x40104a31`). `arm-none-eabi-gcc` is also absent, blocking an isolated BUG-001 hello_main/helloxx_main build/run.
- **NuttX comparison:** current official Apache NuttX `pthread_cond_wait()` atomically counts waiters and uses `nxsem_wait_uninterruptible()`, while signal uses atomic compare/exchange before post. This supports the already rejected BUG-002 analysis/design distinction, but is not a TizenRT fix.

**Cycle judgment: 새 버그 없음.** No new BUG-ID, per-BUG repro artifact, push, or PR; this ledger update is local to `qemu-virt_bughunt`.
