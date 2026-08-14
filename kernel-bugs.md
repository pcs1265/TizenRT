# TizenRT kernel bughunt ledger

Run: 2026-08-14 21:00 UTC monitor cycle

## Scope and bases

- `upstream/master`: `93cde68110a26df205ac4f0f536cff70699f1bc6` (`system_file : verify littlefs mount before format and corruption`)
- `feat/qemu-virt-gdb-awareness`: `bd5069ad3650600fb5b0aab07ca66106362817b2`
- bughunt worktree: `qemu-virt_bughunt`, HEAD `0fa2486130a874e7d1e3886902a609e12337735b`
- Apache NuttX official master checked at `36a971567ac706b86fb9e94cceeb3c81083da344`

The monitored change was only the UTC probe hour (`20` -> `21`). `git fetch --prune upstream origin` succeeded and did not advance `upstream/master`; the scheduler/pthread/semaphore/task commits below were therefore re-audited rather than treated as newly arrived commits.

## Existing IDs (deduplicated)

### BUG-20260814-001 — unreproduced candidate

- qemu-virt `up_timer_disable()` / `up_timer_enable()` return-value candidate.
- Exact artifacts are under `.hermes/kernel-bug-repros/BUG-20260814-001/`.
- No duplicate ID or duplicate root cause found in the current bughunt history.

### BUG-20260814-002 — unreproduced candidate

- Ordinary `pthread_cond_wait()` interrupted by a signal, followed by `pthread_cond_signal()`; stale `pthread_cond_s.waiters` accounting can cause an excess post / future wake error.
- Exact artifacts are under `.hermes/kernel-bug-repros/BUG-20260814-002/`.
- This is not duplicated by the timed-wait accounting path: timed wait decrements on its failure path, while ordinary wait does not.

## This cycle's source audit

Reviewed actual diffs and current files for:

- `813daa2fe` condition-variable waiter counter introduction;
- `47b50100f` cancellation decision race change;
- `5cf352dd5` waking waiters while recovering semaphores held by a terminated task;
- `c93078ab0` semaphore holder count/release changes;
- `ed41deb4e` per-task held-semaphore tracking;
- `4860dbdb2` priority-inheritance overflow guard;
- `542d47be3` and `e705013c1` child-status waitpid/waitid fixes;
- related scheduler/task history on `upstream/master`.

No additional BUG-ID was opened. The review found no distinct, sufficiently evidenced root cause beyond BUG-20260814-002. In particular, the ordinary-cond-wait failure path still lacks a waiter decrement at `os/kernel/pthread/pthread_condwait.c:129-146`; the timed-wait path decrements at `pthread_condtimedwait.c:304-307`. The cancellation and semaphore-recovery changes were checked for a separate failure mode but no new candidate was recorded without a reproducible or independently actionable trigger.

## Runtime/tool evidence

Commands executed from the isolated `qemu-virt_bughunt` worktree:

```text
python3 -m py_compile build/configs/qemu-virt/tools/tizenrt_gdb.py build/configs/qemu-virt/tools/auto_symbol_loader.py
# passed

timeout 20s ./run_qemu.sh > /tmp/bughunt-periodic-20260814T21.log 2>&1
# exit 124: QEMU booted and was stopped by the timeout

gdb-multiarch -q -batch -ex 'set architecture arm' -ex 'target remote :1234' \
  -ex 'source build/configs/qemu-virt/tools/auto_symbol_loader.py' \
  -ex 'source build/configs/qemu-virt/tools/tizenrt_gdb.py' -ex 'interrupt' \
  -ex 'tizenrt current' -ex 'tizenrt tasks' -ex 'tizenrt stack' \
  -ex 'tizenrt waiters' -ex 'tizenrt held' -ex 'info registers pc sp lr' \
  -ex 'detach' -ex 'quit' build/output/bin/tinyara
# connected successfully; OS-awareness commands ran
```

Observed GDB state: 8 tasks; `tash` was the sole semaphore waiter (`semcount=-1`); no held semaphores; stopped at `up_idle+8` (`pc=0x4011ce60`). The auto-symbol loader emitted a Python `AttributeError` before the TizenRT commands loaded, but the TizenRT command module loaded and reported the task/stack/waiter/held state. No reproduction app was present in the flashed image. The image booted to `TASH>>` and loaded the existing `common` and `app1` binaries.

Build/reproduction remains blocked: `arm-none-eabi-gcc` is not installed, and no `build/output/bin/tinyara` ELF exists in this worktree for rebuilding. Thus BUG-001 and BUG-002 remain `unreproduced candidate`, not `reproduced`.

## Apache NuttX comparison

The official NuttX master was queried directly. Its condition-wait implementation uses a separate atomic waiter-count design and an uninterruptible semaphore wait, unlike TizenRT's ordinary `pthread_sem_take()` path. This supports the BUG-002 distinction but is not a TizenRT fix. The reviewed semaphore holder/recovery changes likewise were not marked fixed: no corresponding TizenRT fix commit was observed merged into `upstream/master` for either existing candidate.

No source files on the user's worktree or on `feat/qemu-virt-gdb-awareness` were modified. No push or PR was performed.

## Run: 2026-08-14 22:00 UTC monitor cycle

- `git fetch --prune upstream origin`: exit 0; `upstream/master` remained `93cde68110a26df205ac4f0f536cff70699f1bc6`.
- `qemu-virt_bughunt` remained a separate worktree at `d71b7f65c005d47a49549357196b469367f7f338`; the user's worktree and `feat/qemu-virt-gdb-awareness` were not changed.
- The upstream scheduler/pthread/semaphore/mutex/condition/cancellation/SMP/task history was re-audited in actual files and commit diffs. No new upstream commit in those areas arrived since the previous ledger base, so no new BUG-ID was opened.
- The apparent `&&` flag-check defect in the bughunt branch's copies of `sched_waitpid.c`, `sched_waitid.c`, `task_reparent.c`, and `task_setup.c` was checked against `e705013c1`. It is not a new unresolved TizenRT bug: `e705013c1` is an ancestor of `upstream/master`, and upstream/master contains the correct bitwise `&` checks. Per policy it is **fixed** because the fix is merged and present in upstream code.
- `542d47be3` was also verified in upstream/master: the `waitpid()` child PID assignment is present. No additional child-status defect remained after the comparison.

### Runtime evidence

- `python3 -m py_compile build/configs/qemu-virt/tools/tizenrt_gdb.py build/configs/qemu-virt/tools/auto_symbol_loader.py`: passed.
- `timeout 15s ./run_qemu.sh > /tmp/bughunt-20260814-003-qemu.log 2>&1`: exit 124; QEMU booted through S1 boot, kernel handoff, SMARTFS mount, and reached `TASH>>`.
- The actual `run_qemu.sh` QEMU instance was attached with `gdb-multiarch` using the qemu-virt `auto_symbol_loader.py` and `tizenrt_gdb.py` tools. The commands `tizenrt current`, `tizenrt tasks`, `tizenrt stack`, `tizenrt waiters`, `tizenrt held`, and register inspection executed successfully. A matching bughunt ELF was absent, so the first session had no symbols; a user-worktree ELF was then used read-only and produced mismatched/corrupt task data (one bogus task, zero waiters/held), not valid kernel-state evidence. The live QEMU session was terminated after capture.
- Build/reproduction remains blocked: `arm-none-eabi-gcc` is absent and no matching `build/output/bin/tinyara` ELF exists in the bughunt worktree. Existing BUG-001 and BUG-002 therefore remain `unreproduced candidate`; neither is reproduced, rejected, or fixed.

### Apache NuttX comparison

Official Apache NuttX master was queried directly. Its `pthread_cond_wait()` uses an atomic waiter counter plus `nxsem_wait_uninterruptible()`, and broadcast uses atomic compare/exchange decrementing before posts. This avoids the exact TizenRT ordinary-cond-wait EINTR accounting gap documented as BUG-20260814-002. No NuttX behavior was used to mark a TizenRT bug fixed.

**Cycle judgment: 새 버그 없음.** The only newly scrutinized flag-check issue is resolved in merged `upstream/master`; existing candidates remain deduplicated and unreproduced. No push or PR was performed.
