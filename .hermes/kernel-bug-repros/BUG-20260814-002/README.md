# BUG-20260814-002

## Status and exact target

- **Status:** unreproduced candidate (static root cause confirmed; runtime reproduction blocked).
- **Exact reproduction target:** an ordinary `pthread_cond_wait()` interrupted by a signal so `pthread_sem_take()` returns `EINTR`, followed by `pthread_cond_signal()` on the same condition variable.
- **Observed failure mechanism:** `pthread_cond_wait()` increments `cond->waiters` at `os/kernel/pthread/pthread_condwait.c:121` and never decrements it when its documented/error-checked `pthread_sem_take()` path returns `EINTR`. `pthread_cond_signal()` trusts that stale count and calls `pthread_sem_give()`, potentially leaving an excess semaphore post and incorrect future wake behavior.
- **Introducing upstream commit:** `813daa2feb806dfe99743f480b319b4b7b1a2e5d` (`os/kernel: Add new variable to track waiter count in pthread_cond_s structure`).
- **Why new:** no existing BUG-ID or duplicate root cause was present in the bughunt tree; BUG-20260814-001 concerns qemu timer return values.

## Reproduction artifacts

- `hello_main.patch`: C test using a waiter, `pthread_kill(SIGUSR1)`, and a later condition signal.
- `helloxx_main.patch`: C++ equivalent target.
- `config.txt`: qemu-virt flat configuration.
- `qemu.log`: existing QEMU boot reached `TASH>>`; no candidate app was run.
- `gdb.log`: actual OS-awareness session showing tasks, stacks, waiters, held semaphores, and registers.

The patches are intentionally stored, not applied to the bughunt kernel tree.

## Execution result

- `python3 -m py_compile build/configs/qemu-virt/tools/tizenrt_gdb.py build/configs/qemu-virt/tools/auto_symbol_loader.py`: passed.
- QEMU command: `timeout 25s ./run_qemu.sh > /tmp/bughunt-qemu-20260814.log 2>&1`: booted; terminated by timeout after the observation window.
- GDB command used `source tizenrt_gdb.py`, `target remote :1234`, `tizenrt current`, `tasks`, `stack`, `waiters`, `held`, and registers: passed. It reported 6 tasks, 2 semaphore waiters, 0 held semaphores; see `gdb.log`.
- Build command: `make -C os -j2`: blocked before compilation because `arm-none-eabi-gcc` is absent; the existing output tree also contains root-owned generated files.
- Independent worktree attempt: `git worktree add --detach /tmp/TizenRT-BUG-20260814-002 qemu-virt_bughunt` failed to complete checkout because numerous repository files were not writable in the environment.

## Apache NuttX comparison

Official Apache NuttX master (`36bcebb9c4a9d9c3885be9bac1fe92f86772fa6a`) uses an atomic `wait_count` and `nxsem_wait_uninterruptible()` in `libs/libc/pthread/pthread_condwait.c`; its signal/broadcast paths atomically decrement the count. That design does not have this exact EINTR bookkeeping gap. No NuttX behavior was treated as a TizenRT fix.

## Fix direction (not applied)

Decrement `cond->waiters` on every ordinary `pthread_sem_take()` failure before mutex reacquisition, or use an uninterruptible wait with cancellation-safe accounting. Add an atomic/critical-section protocol consistent with the signal and broadcast paths.
