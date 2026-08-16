# qemu-virt kernel scenario test index

Scenarios live in `apps/examples/hello/hello_main.c`. A **PASS** requires a matching build, image refresh, literal root-level `./run_qemu.sh` boot to `TASH>>`, `hello` output, and clean QEMU termination.

| TEST-ID | Source / trigger | Expected outcome | `dramboot_flat` | `dramboot_flat_smp` | `dramboot_elf` | `dramboot_elf_smp` | State |
|---|---|---|---|---|---|---|---|
| KSC-001 | `hello_main.c`: task lifecycle; worker posts a semaphore, returns a token, and creator timed-waits then joins. | `KSC-001: PASS task create -> wake -> exit -> join` | PASS | PASS | PASS | PASS | pass |
| KSC-002 | `hello_main.c`: two workers make 32 mutex-protected updates after a common semaphore release. | `KSC-002: PASS mutex handoff counter=64` | PASS | PASS | PASS | PASS | pass |
| KSC-003 | `hello_main.c`: worker changes a condition-variable predicate under its mutex and signals; creator predicate-waits with a two-second deadline. | `KSC-003: PASS condition predicate=1` | PASS | PASS | PASS | PASS | pass |
| KSC-004 | `hello_main.c`: worker with affinity covering all CPUs in `CONFIG_SMP_NCPUS` sets and reads a pthread TLS key, signals a semaphore, and is joined; all waits have a two-second deadline and cleanup destroys the key, attribute, and semaphore. | `KSC-004: PASS thread-specific value=0 mask=...` and affinity evidence. | PASS | PASS | PASS | PASS | pass |
| KSC-005 | `hello_main.c`: two all-active-CPU-affined workers concurrently call `pthread_once`; each completion is semaphore-timed and both are joined, while the single initializer count is checked. | `KSC-005: PASS pthread_once initializer count=1 mask=...` and affinity evidence. | PASS | PASS | PASS | PASS | pass |
| KSC-006 | `hello_main.c`: two all-active-CPU-affined workers acquire a shared read lock, publish acquisition, then are released and joined; creator requires both acquisitions before either release. | `KSC-006: PASS rwlock concurrent readers=2 mask=...` and affinity evidence. | PASS | PASS | PASS | PASS | pass |
| KSC-007 | `hello_main.c`: two workers with affinity covering all CPUs in `CONFIG_SMP_NCPUS` are released through a start gate into a two-party pthread barrier; each completion is semaphore-timed, both are joined, and exactly one serial-thread return is required. | `KSC-007: PASS barrier serial count=1 mask=...` and affinity evidence. | PASS | FAIL (2026-08-16 rerun) | PASS | PASS | regression |
| KSC-008 | `hello_main.c`: creator holds a mutex while one all-active-CPU-affined worker calls `pthread_mutex_trylock()`, reports its return through a timed semaphore wait, and is joined before cleanup. | `KSC-008: PASS mutex trylock status=16 mask=...` and affinity evidence. | PASS | PASS | PASS | PASS | pass |
| KSC-009 | `hello_main.c`: two all-active-CPU-affined workers publish that they are condition-waiting; creator sets a predicate and broadcasts, then requires both timed completions and joins. | `KSC-009: PASS condition broadcast woken=2 mask=...` and affinity evidence. | PASS | PASS | PASS | PASS | pass |
| KSC-010 | `hello_main.c`: creator holds a read lock while one all-active-CPU-affined worker calls `pthread_rwlock_trywrlock()`, reports its return through a timed semaphore wait, and is joined before cleanup. | `KSC-010: PASS rwlock trywrite status=16 mask=...` and affinity evidence. | PASS | PASS | PASS | PASS | pass |
| KSC-011 | `hello_main.c`: creator holds a write lock while one all-active-CPU-affined worker calls `pthread_rwlock_tryrdlock()`, reports its return through a timed semaphore wait, and is joined before cleanup. | `KSC-011: PASS rwlock tryread status=16 mask=...` and affinity evidence. | PASS | PASS | PASS | PASS | pass |
| KSC-012 | `hello_main.c`: an initially empty semaphore is waited with an absolute two-second deadline and then destroyed. | `KSC-012: PASS semaphore timeout errno=110` | PASS | PASS | PASS | PASS | pass |
| KSC-013 | `hello_main.c`: an all-active-CPU-affined worker recursively locks and unlocks a `PTHREAD_MUTEX_RECURSIVE` mutex twice, reports completion through a two-second timed semaphore wait, and is joined before mutex/attribute cleanup. | `KSC-013: PASS recursive mutex status=0 mask=...` and affinity evidence. | PASS | PASS | PASS | PASS | pass |
| KSC-014 | `hello_main.c`: creator locks an unsignaled condition variable's mutex, uses `pthread_cond_timedwait()` with an absolute two-second deadline, and destroys the condition variable and mutex after the expected timeout. | `KSC-014: PASS condition timeout status=110` | PASS | PASS | PASS | PASS | pass |
| KSC-015 | `hello_main.c`: an all-active-CPU-affined worker is created detached, posts completion, and returns; creator uses a two-second semaphore deadline and cleans up the thread attribute and semaphore. | `KSC-015: PASS detached completion mask=...` and affinity evidence. | BLOCKED (link) | BLOCKED (link) | BLOCKED (post-link) | BLOCKED (post-link) | blocked |
| KSC-016 | `hello_main.c`: an all-active-CPU-affined worker verifies an empty semaphore's `sem_trywait()` returns `EAGAIN`; creator uses a two-second completion deadline, joins it, then verifies a posted token is acquired nonblockingly. | `KSC-016: PASS semaphore trywait empty=11 posted=0 mask=...` and affinity evidence. | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 post-link) | BLOCKED (KSC-015 post-link) | blocked |
| KSC-017 | `hello_main.c`: an all-active-CPU-affined worker waits behind a start gate while its creator requires `pthread_tryjoin_np()` to return `EBUSY`, then releases, deadline-waits, and normally joins it. | `KSC-017: PASS pthread tryjoin busy=16 mask=...` and affinity evidence. | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 post-link) | BLOCKED (KSC-015 post-link) | blocked |
| KSC-018 | `hello_main.c`: an initially empty semaphore is posted once, its value is read, its token is acquired with a two-second absolute timed wait, and its value is read again before destruction. | `KSC-018: PASS semaphore value before=1 after=0` | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 post-link) | BLOCKED (KSC-015 post-link) | blocked |
| KSC-019 | `hello_main.c`: an initially empty semaphore is posted twice, must report values 2, 1, and 0 around two two-second absolute timed waits, and is then destroyed. | `KSC-019: PASS semaphore values initial=2 middle=1 final=0` | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 post-link) | BLOCKED (KSC-015 post-link) | blocked |
| KSC-020 | `hello_main.c`: creator holds a mutex while one all-active-CPU-affined worker publishes its blocking acquisition attempt; creator uses two-second deadlines to observe the attempt and post-release completion, then joins it. | `KSC-020: PASS mutex blocking handoff status=0 mask=...` and affinity evidence. | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 post-link) | BLOCKED (KSC-015 post-link) | blocked |
| KSC-021 | `hello_main.c`: one all-active-CPU-affined worker compares its `pthread_self()` identity with itself, posts bounded completion, and is joined before attribute and semaphore cleanup. | `KSC-021: PASS pthread self identity status=1 mask=...` and affinity evidence. | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 post-link) | BLOCKED (KSC-015 post-link) | blocked |
| KSC-022 | `hello_main.c`: one all-active-CPU-affined worker publishes its pending wait on an empty semaphore; creator observes it with a deadline, posts exactly one wake token, deadline-waits completion, and joins. | `KSC-022: PASS semaphore wake status=0 mask=...` and affinity evidence. | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 post-link) | BLOCKED (KSC-015 post-link) | blocked |
| KSC-023 | `hello_main.c`: signal a condition variable before any waiter exists, then require a later wait to expire under its mutex. | `KSC-023: PASS condition signal is not retained status=110` | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 link) | BLOCKED (KSC-015 post-link) | PENDING | pending |

## 2026-08-16 completion evidence for KSC-001 through KSC-003

The prior flat and flat-SMP boots reached `TASH>>`; `hello` printed PASS for KSC-001, KSC-002, and KSC-003 and `KSC: harness PASS (0 failed)`. The ELF boot had the same captured output and exited with `QEMU: Terminated`.

The final required ELF-SMP execution this cycle used the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matched image refresh completed with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

Literal root-level `./run_qemu.sh` reached `TASH>>`; after entering `hello`, it captured:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after this job sent Ctrl-A x. This closes the four-configuration verification gate for KSC-001 through KSC-003. KSC-004 was added only afterward and has not yet been built or booted; its four configuration results remain pending.

## 2026-08-16 KSC-004 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`.  Invoking `hello` produced:

```text
KSC-004: START thread-specific data (timeout=2 s)
KSC-004: worker affinity mask=0x1 cpus=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x.  KSC-004 therefore passes `dramboot_flat`; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending and must be completed before adding another scenario.

## 2026-08-16 KSC-004 `dramboot_flat_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: START thread-specific data (timeout=2 s)
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-004 therefore passes `dramboot_flat_smp`; `dramboot_elf` and `dramboot_elf_smp` remain pending and must be completed before adding another scenario.

## 2026-08-16 KSC-004 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: START thread-specific data (timeout=2 s)
KSC-004: worker affinity mask=0x1 cpus=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-004 therefore passes `dramboot_elf`; only `dramboot_elf_smp` remains pending, so no new scenario may be added yet.

## 2026-08-16 KSC-004 `dramboot_elf_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching ELF-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: START thread-specific data (timeout=2 s)
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-004 passes all four required configurations. KSC-005 was added only after that gate closed.

## 2026-08-16 KSC-005 `dramboot_elf_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: START pthread_once concurrency (timeout=2 s)
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-005 passes `dramboot_elf_smp`; `dramboot_flat`, `dramboot_flat_smp`, and `dramboot_elf` remain pending.

## 2026-08-16 KSC-005 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching flat artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: START pthread_once concurrency (timeout=2 s)
KSC-005: worker affinity mask=0x1 cpus=1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-005 therefore passes `dramboot_flat`; `dramboot_flat_smp` and `dramboot_elf` remain pending, so no new scenario may be added yet.

## 2026-08-16 KSC-005 `dramboot_flat_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching flat-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: START pthread_once concurrency (timeout=2 s)
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-005 therefore passes `dramboot_flat_smp`; only `dramboot_elf` remains pending, so no new scenario may be added yet.

## 2026-08-16 KSC-005 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0x1 cpus=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: START pthread_once concurrency (timeout=2 s)
KSC-005: worker affinity mask=0x1 cpus=1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-005 now passes all four required configurations; a subsequent cycle may add exactly one new isolated scenario after inspecting the harness for an unrepresented reachable behavior.

## 2026-08-16 KSC-006 added; verification pending

KSC-006 covers POSIX read/write-lock reader sharing, which was not represented by the preceding task, mutex, condition, TLS, or once scenarios. Two workers use an affinity mask constructed from every CPU in `CONFIG_SMP_NCPUS`; each must acquire the read lock and post its acquisition before the creator permits either to release. The creator uses a two-second `sem_timedwait()` deadline, releases every created worker on both success and failure paths, joins workers, and destroys the lock and semaphores. No configuration has been built or booted with KSC-006 yet; all four outcomes are pending.

## 2026-08-16 KSC-006 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching flat artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC-006: START rwlock reader sharing (timeout=2 s)
KSC-006: worker affinity mask=0x1 cpus=1
KSC-006: PASS rwlock concurrent readers=2 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-006 passes `dramboot_flat`; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending, so no new scenario may be added yet.

## 2026-08-16 KSC-006 `dramboot_flat_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching flat-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC-006: START rwlock reader sharing (timeout=2 s)
KSC-006: worker affinity mask=0xf cpus=4
KSC-006: PASS rwlock concurrent readers=2 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-006 passes `dramboot_flat_smp`; `dramboot_elf` and `dramboot_elf_smp` remain pending, so no new scenario may be added yet.

## 2026-08-16 KSC-006 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0x1 cpus=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: worker affinity mask=0x1 cpus=1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC-006: START rwlock reader sharing (timeout=2 s)
KSC-006: worker affinity mask=0x1 cpus=1
KSC-006: PASS rwlock concurrent readers=2 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-006 passes `dramboot_elf`; only `dramboot_elf_smp` remains pending, so no new scenario may be added yet.

## 2026-08-16 KSC-006 `dramboot_elf_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching ELF-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC-006: START rwlock reader sharing (timeout=2 s)
KSC-006: worker affinity mask=0xf cpus=4
KSC-006: PASS rwlock concurrent readers=2 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-006 now passes all four required configurations. A later cycle may inspect the harness and add exactly one new isolated scenario.

## 2026-08-16 KSC-007 added; verification pending

KSC-007 covers POSIX pthread barrier generation and serial-thread election, which was not represented by the preceding lifecycle, mutex, condition, TLS, once, or reader-sharing scenarios. Two workers use an affinity mask constructed from every CPU in `CONFIG_SMP_NCPUS`. A start semaphore holds both workers until creation succeeds, then each enters a two-party `pthread_barrier_wait()`; the creator requires two semaphore-timed completions, joins both workers, and validates exactly one `PTHREAD_BARRIER_SERIAL_THREAD` result and one normal zero result. Setup failures cancel and join workers before destroying the barrier, attributes, and semaphores. `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending.

## 2026-08-16 KSC-007 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching flat artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC-006: PASS rwlock concurrent readers=2 mask=0x1
KSC-007: START barrier serial election (timeout=2 s)
KSC-007: worker affinity mask=0x1 cpus=1
KSC-007: PASS barrier serial count=1 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-007 passes `dramboot_flat`; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending, so no new scenario may be added yet.

## 2026-08-16 KSC-007 `dramboot_flat_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching flat-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC-006: worker affinity mask=0xf cpus=4
KSC-006: PASS rwlock concurrent readers=2 mask=0xf
KSC-007: START barrier serial election (timeout=2 s)
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: PASS barrier serial count=1 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-007 passes `dramboot_flat_smp`; `dramboot_elf` and `dramboot_elf_smp` remain pending, so no new scenario may be added yet.

## 2026-08-16 KSC-007 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0x1 cpus=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: worker affinity mask=0x1 cpus=1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC-006: worker affinity mask=0x1 cpus=1
KSC-006: PASS rwlock concurrent readers=2 mask=0x1
KSC-007: START barrier serial election (timeout=2 s)
KSC-007: worker affinity mask=0x1 cpus=1
KSC-007: PASS barrier serial count=1 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-007 passes `dramboot_elf`; only `dramboot_elf_smp` remains pending, so no new scenario may be added yet.

## 2026-08-16 KSC-007 `dramboot_elf_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching ELF-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC-006: worker affinity mask=0xf cpus=4
KSC-006: PASS rwlock concurrent readers=2 mask=0xf
KSC-007: START barrier serial election (timeout=2 s)
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: PASS barrier serial count=1 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-007 now passes all four required configurations. A subsequent cycle may inspect the harness and add exactly one new isolated scenario.

## 2026-08-16 KSC-008 added; verification pending

KSC-008 covers the non-blocking mutex exclusion path, which is distinct from KSC-002's blocking mutex handoff. The creator holds a fresh mutex while an all-active-CPU-affined worker calls `pthread_mutex_trylock()`; the worker must report `EBUSY` through a semaphore observed with a two-second deadline. The creator joins the worker before releasing and destroying the mutex; failure paths cancel and join any created worker. At creation, all four outcomes were pending.

## 2026-08-16 KSC-008 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching flat artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC-006: PASS rwlock concurrent readers=2 mask=0x1
KSC-007: PASS barrier serial count=1 mask=0x1
KSC-008: START mutex trylock exclusion (timeout=2 s)
KSC-008: worker affinity mask=0x1 cpus=1
KSC-008: PASS mutex trylock status=16 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-008 passes `dramboot_flat`; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending, so no new scenario may be added yet.

## 2026-08-16 KSC-008 `dramboot_flat_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching flat-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC-006: worker affinity mask=0xf cpus=4
KSC-006: PASS rwlock concurrent readers=2 mask=0xf
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: PASS barrier serial count=1 mask=0xf
KSC-008: START mutex trylock exclusion (timeout=2 s)
KSC-008: worker affinity mask=0xf cpus=4
KSC-008: PASS mutex trylock status=16 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-008 passes `dramboot_flat_smp`; `dramboot_elf` and `dramboot_elf_smp` remain pending, so no new scenario may be added yet.

## 2026-08-16 KSC-008 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0x1 cpus=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: worker affinity mask=0x1 cpus=1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC-006: worker affinity mask=0x1 cpus=1
KSC-006: PASS rwlock concurrent readers=2 mask=0x1
KSC-007: worker affinity mask=0x1 cpus=1
KSC-007: PASS barrier serial count=1 mask=0x1
KSC-008: START mutex trylock exclusion (timeout=2 s)
KSC-008: worker affinity mask=0x1 cpus=1
KSC-008: PASS mutex trylock status=16 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-008 passes `dramboot_elf`; only `dramboot_elf_smp` remains pending, so no new scenario may be added yet.

## 2026-08-16 KSC-008 `dramboot_elf_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching ELF-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC-006: worker affinity mask=0xf cpus=4
KSC-006: PASS rwlock concurrent readers=2 mask=0xf
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: PASS barrier serial count=1 mask=0xf
KSC-008: START mutex trylock exclusion (timeout=2 s)
KSC-008: worker affinity mask=0xf cpus=4
KSC-008: PASS mutex trylock status=16 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-008 passes all four required configurations. A subsequent cycle may inspect the harness and add exactly one new isolated scenario.

## 2026-08-16 KSC-009 added; verification pending

KSC-009 covers condition-variable broadcast wakeup, distinct from KSC-003's single-waiter signal path. Two workers receive an affinity mask containing every CPU derived from `CONFIG_SMP_NCPUS`, acquire the shared mutex, and publish readiness before condition-timed-waiting on a predicate. The creator uses two-second timed semaphore waits to require both waiters, sets the predicate under the mutex, broadcasts, then requires both completion posts and joins both workers. Failure paths still set the predicate and broadcast before joining, and all synchronization objects are destroyed afterward. At creation, all four verification outcomes were pending.

## 2026-08-16 KSC-009 `dramboot_elf_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching ELF-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC-006: worker affinity mask=0xf cpus=4
KSC-006: PASS rwlock concurrent readers=2 mask=0xf
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: PASS barrier serial count=1 mask=0xf
KSC-008: worker affinity mask=0xf cpus=4
KSC-008: PASS mutex trylock status=16 mask=0xf
KSC-009: START condition broadcast (timeout=2 s)
KSC-009: worker affinity mask=0xf cpus=4
KSC-009: PASS condition broadcast woken=2 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-009 passes `dramboot_elf_smp`; `dramboot_flat`, `dramboot_flat_smp`, and `dramboot_elf` remain pending, so no new scenario may be added yet.

## 2026-08-16 KSC-009 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching flat artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0x1 cpus=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: worker affinity mask=0x1 cpus=1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC-006: worker affinity mask=0x1 cpus=1
KSC-006: PASS rwlock concurrent readers=2 mask=0x1
KSC-007: worker affinity mask=0x1 cpus=1
KSC-007: PASS barrier serial count=1 mask=0x1
KSC-008: worker affinity mask=0x1 cpus=1
KSC-008: PASS mutex trylock status=16 mask=0x1
KSC-009: START condition broadcast (timeout=2 s)
KSC-009: worker affinity mask=0x1 cpus=1
KSC-009: PASS condition broadcast woken=2 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-009 passes `dramboot_flat`; `dramboot_flat_smp` and `dramboot_elf` remain pending, so no new scenario may be added yet.

## 2026-08-16 KSC-009 `dramboot_flat_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching flat-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC-006: worker affinity mask=0xf cpus=4
KSC-006: PASS rwlock concurrent readers=2 mask=0xf
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: PASS barrier serial count=1 mask=0xf
KSC-008: worker affinity mask=0xf cpus=4
KSC-008: PASS mutex trylock status=16 mask=0xf
KSC-009: START condition broadcast (timeout=2 s)
KSC-009: worker affinity mask=0xf cpus=4
KSC-009: PASS condition broadcast woken=2 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-009 passes `dramboot_flat_smp`; only `dramboot_elf` remains pending, so no new scenario may be added yet.

## 2026-08-16 KSC-009 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0x1 cpus=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: worker affinity mask=0x1 cpus=1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC-006: worker affinity mask=0x1 cpus=1
KSC-006: PASS rwlock concurrent readers=2 mask=0x1
KSC-007: worker affinity mask=0x1 cpus=1
KSC-007: PASS barrier serial count=1 mask=0x1
KSC-008: worker affinity mask=0x1 cpus=1
KSC-008: PASS mutex trylock status=16 mask=0x1
KSC-009: START condition broadcast (timeout=2 s)
KSC-009: worker affinity mask=0x1 cpus=1
KSC-009: PASS condition broadcast woken=2 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-009 now passes all four required configurations. A subsequent cycle may inspect the harness and add exactly one new isolated scenario.

## 2026-08-16 KSC-010 added; verification pending

KSC-010 covers nonblocking reader/writer-lock exclusion, distinct from KSC-006's concurrent-reader path and KSC-008's mutex-specific trylock path. The creator holds a fresh read lock while one worker with an affinity mask constructed from every CPU in `CONFIG_SMP_NCPUS` calls `pthread_rwlock_trywrlock()`. The worker must report `EBUSY` through a two-second `sem_timedwait()` deadline, be joined, and then the creator releases and destroys the lock. All setup, affinity, thread, wait, join, unlock, and cleanup return values are checked; a failed wait cancels and joins the worker before cleanup. All four configuration results are pending.

## 2026-08-16 KSC-010 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching flat artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-010: START rwlock writer exclusion (timeout=2 s)
KSC-010: worker affinity mask=0x1 cpus=1
KSC-010: PASS rwlock trywrite status=16 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-010 passes `dramboot_flat`; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending, so no new scenario may be added yet.

## 2026-08-16 KSC-010 `dramboot_flat_smp` evidence; KSC-007 regression

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching flat-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` captured:

```text
KSC-007: START barrier serial election (timeout=2 s)
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: FAIL barrier completion errno=110
KSC-007: FAIL barrier result[0]=-1
KSC-007: FAIL barrier result[1]=-1
KSC-007: FAIL serial count=0
KSC-007: FAIL barrier serial count=0 mask=0xf
KSC-010: START rwlock writer exclusion (timeout=2 s)
KSC-010: worker affinity mask=0xf cpus=4
KSC-010: PASS rwlock trywrite status=16 mask=0xf
KSC: harness FAIL (1 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-010 passes `dramboot_flat_smp` under the individual four-configuration gate: its selected configuration built, refreshed, booted, reached TASH, printed its TEST-ID PASS and affinity evidence, and terminated cleanly. The same reachable run newly failed KSC-007 after its two-second completion deadline; this is recorded as a `dramboot_flat_smp` regression and prevents treating the aggregate harness run as clean. KSC-010 `dramboot_elf` and `dramboot_elf_smp` remain pending; no new scenario was added.

## 2026-08-16 KSC-010 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-010: START rwlock writer exclusion (timeout=2 s)
KSC-010: worker affinity mask=0x1 cpus=1
KSC-010: PASS rwlock trywrite status=16 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-010 passes `dramboot_elf`; `dramboot_elf_smp` remains pending, so no new scenario may be added.

## 2026-08-16 KSC-010 `dramboot_elf_smp` evidence; KSC-007 regression reproduced

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching ELF-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-007: START barrier serial election (timeout=2 s)
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: FAIL barrier completion errno=110
KSC-007: FAIL barrier result[0]=-1
KSC-007: FAIL barrier result[1]=-1
KSC-007: FAIL serial count=0
KSC-007: FAIL barrier serial count=0 mask=0xf
KSC-010: START rwlock writer exclusion (timeout=2 s)
KSC-010: worker affinity mask=0xf cpus=4
KSC-010: PASS rwlock trywrite status=16 mask=0xf
KSC: harness FAIL (1 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-010 passes `dramboot_elf_smp` and therefore all four required configurations. The existing reachable KSC-007 barrier timeout also reproduced under ELF-SMP; KSC-007 remains an explicit regression rather than an aggregate clean pass.

## 2026-08-16 KSC-011 added; verification pending

KSC-011 covers the reverse nonblocking reader/writer-lock exclusion from KSC-010: while the creator holds a write lock, one worker with affinity covering every CPU in `CONFIG_SMP_NCPUS` calls `pthread_rwlock_tryrdlock()` and must observe `EBUSY`. The worker posts its result through a two-second `sem_timedwait()` deadline; all setup, affinity, thread, wait, join, unlock, attribute-destroy, semaphore-destroy, and rwlock-destroy results are checked. A failed wait cancels and joins the worker before cleanup. This behavior is reachable through `hello` and is distinct from KSC-006 reader sharing and KSC-010's writer rejection. All four configuration outcomes are pending.

## 2026-08-16 KSC-011 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching flat artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-011: START rwlock reader exclusion (timeout=2 s)
KSC-011: worker affinity mask=0x1 cpus=1
KSC-011: PASS rwlock tryread status=16 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-011 passes `dramboot_flat`; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending, so no new scenario was added.

## 2026-08-16 KSC-011 `dramboot_flat_smp` evidence; KSC-007 regression reproduced

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching flat-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-007: START barrier serial election (timeout=2 s)
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: FAIL barrier completion errno=110
KSC-007: FAIL barrier result[0]=-1
KSC-007: FAIL barrier result[1]=-1
KSC-007: FAIL serial count=0
KSC-007: FAIL barrier serial count=0 mask=0xf
KSC-011: START rwlock reader exclusion (timeout=2 s)
KSC-011: worker affinity mask=0xf cpus=4
KSC-011: PASS rwlock tryread status=16 mask=0xf
KSC: harness FAIL (1 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-011 passes `dramboot_flat_smp` under its individual four-configuration gate: the selected configuration built and refreshed, the literal run booted to TASH, and KSC-011 printed its PASS and all-active-CPU affinity evidence before clean termination. The existing reachable KSC-007 barrier timeout reproduced; KSC-011 `dramboot_elf` and `dramboot_elf_smp` remain pending, so no scenario was added.

## 2026-08-16 KSC-011 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-011: START rwlock reader exclusion (timeout=2 s)
KSC-011: worker affinity mask=0x1 cpus=1
KSC-011: PASS rwlock tryread status=16 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-011 passes `dramboot_elf`; only `dramboot_elf_smp` remains pending, so no new scenario was added.

## 2026-08-16 KSC-011 `dramboot_elf_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching ELF-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-011: START rwlock reader exclusion (timeout=2 s)
KSC-011: worker affinity mask=0xf cpus=4
KSC-011: PASS rwlock tryread status=16 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-011 passes `dramboot_elf_smp` and closes its four-configuration gate. KSC-007 did not reproduce its documented barrier regression in this run. After all prior TEST-IDs had complete outcomes, KSC-012 was added.

## 2026-08-16 KSC-012 added; verification pending

KSC-012 covers the semaphore timed-wait expiry path, which is not represented by the prior lifecycle, handoff, condition, TLS, once, reader/writer-lock, barrier, or trylock scenarios. `hello` initializes an empty semaphore, derives an absolute two-second deadline with `clock_gettime(CLOCK_REALTIME)`, requires `sem_timedwait()` to fail with `ETIMEDOUT`, and destroys the semaphore while checking every operation. It uses no worker, so it remains compatible with single-core and SMP configurations. It was added with all four outcomes pending; no KSC-013 may be added until every KSC-012 result is recorded.

## 2026-08-16 KSC-012 `dramboot_elf_smp` evidence; KSC-007 regression reproduced

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching ELF-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-007: FAIL barrier completion errno=110
KSC-007: FAIL barrier serial count=0 mask=0xf
KSC-012: START semaphore timeout (timeout=2 s)
KSC-012: PASS semaphore timeout errno=110
KSC: harness FAIL (1 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-012 passes `dramboot_elf_smp`: build, refresh, literal boot, TEST-ID PASS, and clean termination all completed. The established reachable KSC-007 barrier timeout reproduced, so the aggregate harness remained failed; it is separately recorded as KSC-007's ELF-SMP regression. `dramboot_flat`, `dramboot_flat_smp`, and `dramboot_elf` remain pending, so no new scenario was added.

## 2026-08-16 KSC-012 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching flat artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-012: START semaphore timeout (timeout=2 s)
KSC-012: PASS semaphore timeout errno=110
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-012 passes `dramboot_flat`: build, matching image refresh, literal boot, TEST-ID PASS, and clean termination all completed. `dramboot_flat_smp` and `dramboot_elf` remain pending, so no new scenario was added.

## 2026-08-16 KSC-012 `dramboot_flat_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching flat-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-004: worker affinity mask=0xf cpus=4
KSC-004: PASS thread-specific value=0 mask=0xf
KSC-005: worker affinity mask=0xf cpus=4
KSC-005: PASS pthread_once initializer count=1 mask=0xf
KSC-006: worker affinity mask=0xf cpus=4
KSC-006: PASS rwlock concurrent readers=2 mask=0xf
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: PASS barrier serial count=1 mask=0xf
KSC-008: worker affinity mask=0xf cpus=4
KSC-008: PASS mutex trylock status=16 mask=0xf
KSC-009: worker affinity mask=0xf cpus=4
KSC-009: PASS condition broadcast woken=2 mask=0xf
KSC-010: worker affinity mask=0xf cpus=4
KSC-010: PASS rwlock trywrite status=16 mask=0xf
KSC-011: worker affinity mask=0xf cpus=4
KSC-011: PASS rwlock tryread status=16 mask=0xf
KSC-012: START semaphore timeout (timeout=2 s)
KSC-012: PASS semaphore timeout errno=110
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-012 passes `dramboot_flat_smp`: build, matching image refresh, literal boot, TEST-ID PASS, and clean termination all completed. Only `dramboot_elf` remains pending, so no new scenario was added.

## 2026-08-16 KSC-012 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: worker affinity mask=0x1 cpus=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: worker affinity mask=0x1 cpus=1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC-006: worker affinity mask=0x1 cpus=1
KSC-006: PASS rwlock concurrent readers=2 mask=0x1
KSC-007: worker affinity mask=0x1 cpus=1
KSC-007: PASS barrier serial count=1 mask=0x1
KSC-008: worker affinity mask=0x1 cpus=1
KSC-008: PASS mutex trylock status=16 mask=0x1
KSC-009: worker affinity mask=0x1 cpus=1
KSC-009: PASS condition broadcast woken=2 mask=0x1
KSC-010: worker affinity mask=0x1 cpus=1
KSC-010: PASS rwlock trywrite status=16 mask=0x1
KSC-011: worker affinity mask=0x1 cpus=1
KSC-011: PASS rwlock tryread status=16 mask=0x1
KSC-012: START semaphore timeout (timeout=2 s)
KSC-012: PASS semaphore timeout errno=110
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-012 passes `dramboot_elf`; all four required configurations now have actual passing results. The next cycle may inspect the harness and add exactly one new isolated scenario.

## 2026-08-16 KSC-013 added; verification pending

KSC-013 covers recursive POSIX mutex ownership, unrepresented by the preceding lifecycle, normal mutex, condition, TLS, once, rwlock, barrier, and semaphore scenarios. A worker with an affinity mask constructed from every CPU in `CONFIG_SMP_NCPUS` locks a `PTHREAD_MUTEX_RECURSIVE` mutex twice and unlocks it twice. Its completion is required through a two-second `sem_timedwait()` deadline, then the worker is joined; every attribute, mutex, semaphore, affinity, and cleanup result is checked. It remains compatible with one CPU and all four configuration outcomes are pending. No KSC-014 may be added until every KSC-013 result is recorded.

## 2026-08-16 KSC-013 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching flat artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-013: START recursive mutex ownership (timeout=2 s)
KSC-013: worker affinity mask=0x1 cpus=1
KSC-013: PASS recursive mutex status=0 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-013 passes `dramboot_flat`; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending, so no new scenario may be added.

## 2026-08-16 KSC-013 `dramboot_flat_smp` evidence; KSC-007 regression reproduced

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching flat-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-007: START barrier serial election (timeout=2 s)
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: FAIL barrier completion errno=110
KSC-007: FAIL barrier result[0]=-1
KSC-007: FAIL barrier result[1]=-1
KSC-007: FAIL serial count=0
KSC-007: FAIL barrier serial count=0 mask=0xf
KSC-013: START recursive mutex ownership (timeout=2 s)
KSC-013: worker affinity mask=0xf cpus=4
KSC-013: PASS recursive mutex status=0 mask=0xf
KSC: harness FAIL (1 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-013 passes `dramboot_flat_smp`: the selected configuration built and refreshed, the literal root-level run booted to TASH, KSC-013 printed its TEST-ID PASS and all-active-CPU affinity evidence, and QEMU terminated cleanly. The established reachable KSC-007 barrier timeout reproduced, so the aggregate harness failed; KSC-013 `dramboot_elf` and `dramboot_elf_smp` remain pending and no new scenario was added.

## 2026-08-16 KSC-013 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-013: START recursive mutex ownership (timeout=2 s)
KSC-013: worker affinity mask=0x1 cpus=1
KSC-013: PASS recursive mutex status=0 mask=0x1
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-013 passes `dramboot_elf`; the required build, matching image refresh, literal root-level boot, command execution, TEST-ID PASS output, and clean termination were all observed. Only `dramboot_elf_smp` remains pending, so no new scenario was added.

## 2026-08-16 KSC-013 `dramboot_elf_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching ELF-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-013: START recursive mutex ownership (timeout=2 s)
KSC-013: worker affinity mask=0xf cpus=4
KSC-013: PASS recursive mutex status=0 mask=0xf
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-013 now passes all four required configurations. After completing every pending result, KSC-014 was added as the one new isolated scenario for this cycle.

## 2026-08-16 KSC-014 added; verification pending

KSC-014 covers the condition-variable timeout path, distinct from KSC-003's predicate-and-signal wakeup, KSC-009's broadcast wakeup, and KSC-012's semaphore timeout. `hello` initializes a mutex and condition variable, locks the mutex, derives an absolute two-second `CLOCK_REALTIME` deadline, and requires an unsignaled `pthread_cond_timedwait()` to return `ETIMEDOUT`; it then unlocks and destroys both synchronization objects while checking every return value. It uses no worker, so it remains compatible with single-core and SMP configurations. All four configuration outcomes are pending; no KSC-015 may be added until every KSC-014 result is recorded.

## 2026-08-16 KSC-014 `dramboot_flat` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The matching flat artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-014: START condition timeout (timeout=2 s)
KSC-014: PASS condition timeout status=110
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after this job sent Ctrl-A x. KSC-014 passes `dramboot_flat`: the required build, matching image refresh, literal boot, TEST-ID PASS output, and clean termination were observed. `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending, so no new scenario was added.

## 2026-08-16 KSC-014 `dramboot_flat_smp` evidence; KSC-007 regression reproduced

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The matching flat-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-007: START barrier serial election (timeout=2 s)
KSC-007: worker affinity mask=0xf cpus=4
KSC-007: FAIL barrier completion errno=110
KSC-007: FAIL barrier result[0]=-1
KSC-007: FAIL barrier result[1]=-1
KSC-007: FAIL serial count=0
KSC-007: FAIL barrier serial count=0 mask=0xf
KSC-014: START condition timeout (timeout=2 s)
KSC-014: PASS condition timeout status=110
KSC: harness FAIL (1 failed)
QEMU: Terminated
```

QEMU exited 0 after this job sent Ctrl-A x. KSC-014 passes `dramboot_flat_smp`: the selected configuration built and refreshed, the literal root-level boot reached TASH, the TEST-ID emitted the expected PASS output, and QEMU terminated cleanly. The established reachable KSC-007 barrier timeout reproduced, so the aggregate harness failed. `dramboot_elf` and `dramboot_elf_smp` remain pending, so no new scenario was added.

## 2026-08-16 KSC-014 `dramboot_elf` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The matching ELF artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced:

```text
KSC-001: PASS task create -> wake -> exit -> join
KSC-002: PASS mutex handoff counter=64
KSC-003: PASS condition predicate=1
KSC-004: PASS thread-specific value=0 mask=0x1
KSC-005: PASS pthread_once initializer count=1 mask=0x1
KSC-006: PASS rwlock concurrent readers=2 mask=0x1
KSC-007: PASS barrier serial count=1 mask=0x1
KSC-008: PASS mutex trylock status=16 mask=0x1
KSC-009: PASS condition broadcast woken=2 mask=0x1
KSC-010: PASS rwlock trywrite status=16 mask=0x1
KSC-011: PASS rwlock tryread status=16 mask=0x1
KSC-012: PASS semaphore timeout errno=110
KSC-013: PASS recursive mutex status=0 mask=0x1
KSC-014: START condition timeout (timeout=2 s)
KSC-014: PASS condition timeout status=110
KSC: harness PASS (0 failed)
QEMU: Terminated
```

QEMU exited 0 after Ctrl-A x. KSC-014 passes `dramboot_elf`: required build, matching image refresh, literal boot to TASH, TEST-ID PASS output, and clean termination were all observed. Only `dramboot_elf_smp` remains pending, so no new scenario was added.

## 2026-08-16 KSC-014 `dramboot_elf_smp` evidence

The required configuration/build succeeded with:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The matching ELF-SMP artifact refresh succeeded with:

```sh
printf '0\n' | TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all
```

A literal root-level `./run_qemu.sh` boot reached `TASH>>`. Invoking `hello` produced the expected KSC-014 result and a clean aggregate harness result:

```text
KSC-014: START condition timeout (timeout=2 s)
KSC-014: PASS condition timeout status=110
KSC: harness PASS (0 failed)
QEMU: Terminated
```

The SMP affinity evidence from the same run was `mask=0xf cpus=4` for KSC-004 through KSC-011 and KSC-013. QEMU exited 0 after Ctrl-A x. KSC-014 now passes all four required configurations; all existing TEST-IDs have recorded four-configuration outcomes (KSC-007 retains its recorded `dramboot_flat_smp` regression), so a later cycle may add exactly one new isolated scenario after inspecting the harness.

## 2026-08-16 KSC-015 added; verification pending

KSC-015 covers detached pthread lifecycle completion, an unrepresented reachable behavior after the prior join-based lifecycle, synchronization, locking, TLS, and timeout scenarios. Its worker receives an affinity mask containing every CPU in `CONFIG_SMP_NCPUS`, prints that mask as evidence, posts one completion semaphore, and returns detached. The creator bounds the completion observation with a two-second absolute deadline and cleans up its semaphore and thread attributes. No configuration has yet built, refreshed, booted, or invoked this new scenario; all four outcomes are pending.

## 2026-08-16 KSC-015 `dramboot_flat` build blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

Configuration and compilation proceeded, but the final link failed while resolving KSC-015:

```text
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1768: undefined reference to `pthread_attr_setdetachstate'
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

The wrapper returned exit status 0 despite these linker errors. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible, so this result is recorded as an explicit `dramboot_flat` blocker rather than PASS. The other three configurations remain pending; no new scenario was added.

## 2026-08-16 KSC-015 `dramboot_flat_smp` build blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

Configuration and compilation proceeded, but the final link failed at the same unsupported detached-thread attribute API:

```text
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1768: undefined reference to `pthread_attr_setdetachstate'
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

The `dbuild.sh` wrapper again returned exit status 0 despite the linker failure. Consequently no matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination could occur. This is an explicit `dramboot_flat_smp` blocker, not PASS; `dramboot_elf` and `dramboot_elf_smp` remain pending, and no new scenario was added.

## 2026-08-16 KSC-015 `dramboot_elf` post-link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

Compilation and the kernel `tinyara` link proceeded, but ELF packaging rejected the unresolved KSC-015 detached-thread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1768
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

The `dbuild.sh` wrapper returned status 0 despite this post-link failure; the existing `tinyara` artifact was therefore not evidence of a successful matching application image. No image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was attempted. This is an explicit `dramboot_elf` blocker rather than PASS. Only `dramboot_elf_smp` remains pending, and no new scenario was added.

## 2026-08-16 KSC-015 `dramboot_elf_smp` post-link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

Compilation and the kernel `tinyara` link completed, but ELF packaging rejected the same unresolved detached-thread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1768
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

The `dbuild.sh` wrapper returned status 0 despite the post-link failure. The retained `tinyara` image is not a successful matching KSC-015 application image, so no image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was attempted. This is an explicit `dramboot_elf_smp` blocker rather than PASS. KSC-015 now has all four required configuration outcomes recorded as blockers; no new scenario was added this cycle.

## 2026-08-16 KSC-016 added; verification pending

KSC-016 covers the semaphore nonblocking acquisition state transition, which is not represented by the prior semaphore timed-expiry or thread synchronization scenarios. A worker with affinity covering every CPU in `CONFIG_SMP_NCPUS` calls `sem_trywait()` on an empty semaphore and records the required `EAGAIN`; its completion is observed through a two-second absolute `sem_timedwait()` deadline and it is joined. The creator then posts exactly one token and requires `sem_trywait()` to consume it, before destroying both semaphores and the thread attribute. No configuration has yet built, refreshed, booted, or invoked this scenario; all four outcomes are pending.

## 2026-08-16 KSC-016 `dramboot_elf_smp` post-link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

KSC-016 compiled as part of the `hello` application, but the ELF packaging stage stopped before a matching application image could be produced because the retained KSC-015 scenario still references the unavailable detached-thread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1774
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

The wrapper returned status 0 despite this packaging failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible; this is a `dramboot_elf_smp` blocker inherited from KSC-015 rather than a KSC-016 PASS. `dramboot_flat`, `dramboot_flat_smp`, and `dramboot_elf` remain pending.

## 2026-08-16 KSC-016 `dramboot_flat` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

Configuration and compilation completed through `hello_main.c`, but the final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1774: undefined reference to `pthread_attr_setdetachstate'
Makefile:207: recipe for target '../build/output/bin/tinyara' failed
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned status 0 despite this linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit `dramboot_flat` blocker inherited from KSC-015, not a KSC-016 PASS. `dramboot_flat_smp` and `dramboot_elf` remain pending; no new scenario was added because KSC-016 still lacks complete configuration outcomes.

## 2026-08-16 KSC-016 `dramboot_flat_smp` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

Configuration and compilation completed through `hello_main.c`, but the final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1774: undefined reference to `pthread_attr_setdetachstate'
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned status 0 despite this linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit `dramboot_flat_smp` blocker inherited from KSC-015, not a KSC-016 PASS. Only `dramboot_elf` remains pending; no new scenario was added because KSC-016 still lacks complete configuration outcomes.

## 2026-08-16 KSC-016 `dramboot_elf` post-link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

Compilation and the kernel `tinyara` link completed, but the ELF packaging stage could not produce a matching KSC-016 application image because the retained KSC-015 detached-thread scenario still references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1774
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

The `dbuild.sh` wrapper returned status 0 despite this packaging failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit `dramboot_elf` blocker inherited from KSC-015, not a KSC-016 PASS. KSC-016 now has all four required configuration outcomes recorded as blockers; no new scenario was added this cycle.

## 2026-08-16 KSC-017 added; verification pending

KSC-017 covers the reachable nonblocking pthread join busy path, which is not represented by the prior lifecycle, synchronization, lock, timeout, TLS, barrier, or semaphore state-transition scenarios. One worker receives an affinity mask containing every CPU in `CONFIG_SMP_NCPUS` and remains blocked at a start gate. Its creator verifies `pthread_tryjoin_np()` returns `EBUSY`, releases the worker, observes completion with a two-second absolute semaphore deadline, then normally joins and validates the worker token. All attribute and semaphore resources are destroyed; if release fails, the worker is cancelled and joined before destruction. No configuration has yet built, refreshed, booted, or invoked this scenario; all four outcomes are pending.

## 2026-08-16 KSC-017 `dramboot_flat` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

KSC-017 compiled successfully as `hello_main.c`, but the final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1778: undefined reference to `pthread_attr_setdetachstate'
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned status 0 despite this linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit `dramboot_flat` blocker inherited from KSC-015, not a KSC-017 PASS; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending.

## 2026-08-16 KSC-017 `dramboot_flat_smp` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

Configuration and compilation completed through `hello_main.c`, but the final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1778: undefined reference to `pthread_attr_setdetachstate'
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned status 0 despite this linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit `dramboot_flat_smp` blocker inherited from KSC-015, not a KSC-017 PASS; `dramboot_elf` and `dramboot_elf_smp` remain pending. No new scenario was added because KSC-017 still lacks complete configuration outcomes.

## 2026-08-16 KSC-017 `dramboot_elf` post-link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

Configuration and compilation, including `hello_main.c`, completed. Kernel `tinyara` also linked, but final ELF application packaging could not produce the matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate	/root/tizenrt/apps/examples/hello/hello_main.c:1778
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

`dbuild.sh` returned status 0 despite this post-link packaging failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit `dramboot_elf` blocker inherited from KSC-015, not a KSC-017 PASS; only `dramboot_elf_smp` remained pending. No new scenario was added because the four-configuration gate was not yet closed.

## 2026-08-16 KSC-017 `dramboot_elf_smp` post-link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

Configuration and compilation, including `hello_main.c`, completed. Kernel `tinyara` also linked, but final ELF application packaging could not produce the matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1778
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

The `dbuild.sh` wrapper returned status 0 despite this post-link packaging failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit `dramboot_elf_smp` blocker inherited from KSC-015, not a KSC-017 PASS. KSC-017 now has all four required configuration outcomes recorded; no scenario was added in this cycle because verification was pending at its start.

## 2026-08-16 KSC-018 added; verification pending

KSC-018 covers semaphore value accounting, an unrepresented reachable behavior distinct from KSC-012's empty-semaphore timed expiry and KSC-016's nonblocking acquisition. `hello` initializes an empty semaphore, posts exactly one token, requires `sem_getvalue()` to report one, consumes that token through a two-second absolute `sem_timedwait()` deadline, then requires `sem_getvalue()` to report zero before destroying the semaphore. Every operation is checked and the scenario has no worker, so it is compatible with single-core and SMP configurations. All prior TEST-IDs have recorded outcomes (including KSC-015 through KSC-017 explicit inherited blockers), and KSC-018 is the exactly one new isolated scenario for this cycle; all four KSC-018 outcomes are pending.

## 2026-08-16 KSC-018 `dramboot_flat` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

`hello_main.c`, including KSC-018, compiled successfully, and the TizenRT libc build included `bin/sem_getvalue.o`. The final kernel link nevertheless could not produce a matching image because the retained KSC-015 scenario references the unavailable detached-thread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1780: undefined reference to `pthread_attr_setdetachstate'
Makefile:207: recipe for target '../build/output/bin/tinyara' failed
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

The wrapper returned status 0 despite the final link failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit KSC-018 `dramboot_flat` blocker inherited from KSC-015, not a PASS; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending.

## 2026-08-16 KSC-018 `dramboot_flat_smp` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The configuration selected SMP and compiled `hello_main.c`, including KSC-018, but the final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1780: undefined reference to `pthread_attr_setdetachstate'
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

The `dbuild.sh` wrapper returned status 0 despite this final linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit KSC-018 `dramboot_flat_smp` blocker inherited from KSC-015, not a PASS; `dramboot_elf` and `dramboot_elf_smp` remain pending. No new scenario was added because the four-configuration gate remains open.

## 2026-08-16 KSC-018 `dramboot_elf` post-link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The configuration and compilation completed through `hello_main.c`, including KSC-018, and the kernel `tinyara` link completed. Final ELF application packaging could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1780
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

The `dbuild.sh` wrapper returned status 0 despite this post-link failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit KSC-018 `dramboot_elf` blocker inherited from KSC-015, not a PASS; only `dramboot_elf_smp` remains pending. No new scenario was added because the four-configuration gate remains open.

## 2026-08-16 KSC-018 `dramboot_elf_smp` post-link blocker

The final pending KSC-018 configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

Compilation completed through `hello_main.c`, including KSC-018, and the kernel `tinyara` link completed. Final ELF application packaging did not produce a matching image because retained KSC-015 still references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1780
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

`dbuild.sh` returned status 0 despite the packaging failure. Therefore no matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit KSC-018 `dramboot_elf_smp` blocker inherited from KSC-015, not a PASS. KSC-018 now has all four required outcomes recorded.

## 2026-08-16 KSC-019 added; verification pending

After every prior TEST-ID had a recorded result or explicit blocker in all four configurations, KSC-019 was added as this cycle's exactly one new isolated scenario. It covers multi-token semaphore accounting, a reachable behavior distinct from KSC-018's one-token post/acquire transition: it posts twice, verifies values 2, 1, and 0 around two finite absolute timed waits, then destroys the semaphore while checking every result. It has no worker and therefore remains compatible with single-core and SMP configurations. All four KSC-019 outcomes are pending; the next cycle must first attempt its pending configurations. The current inherited KSC-015 post-link blocker will prevent a matching image unless that retained scenario is made linkable.

## 2026-08-16 KSC-019 `dramboot_flat` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

Configuration and compilation completed through `hello_main.c`, including KSC-019, but the final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1782: undefined reference to `pthread_attr_setdetachstate'
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned status 0 despite this linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit KSC-019 `dramboot_flat` blocker inherited from KSC-015, not a PASS; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending. No new scenario was added because KSC-019's four-configuration gate remains open.

## 2026-08-16 KSC-019 `dramboot_flat_smp` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

Configuration selected the SMP qemu-virt build and compilation completed through `hello_main.c`, including KSC-019, but the final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1782: undefined reference to `pthread_attr_setdetachstate'
Makefile:207: recipe for target '../build/output/bin/tinyara' failed
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

The `dbuild.sh` wrapper returned status 0 despite the final linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-019 `dramboot_flat_smp` blocker inherited from KSC-015, not a PASS; `dramboot_elf` and `dramboot_elf_smp` remain pending. No new scenario was added because KSC-019's four-configuration gate remains open.

## 2026-08-16 KSC-019 `dramboot_elf` post-link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

Configuration and compilation completed through `hello_main.c`, including KSC-019, and the kernel `tinyara` link completed. Final ELF application packaging could not produce a matching image because the retained KSC-015 detached-thread scenario still references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1782
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

The `dbuild.sh` wrapper returned status 0 despite this post-link packaging failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit KSC-019 `dramboot_elf` blocker inherited from KSC-015, not a PASS; only `dramboot_elf_smp` remains pending. No new scenario was added because KSC-019's four-configuration gate remains open.

## 2026-08-16 KSC-019 `dramboot_elf_smp` post-link blocker

The final pending KSC-019 configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

Compilation completed through `hello_main.c`, including KSC-019, and the kernel `tinyara` link completed. Final ELF application packaging did not produce a matching image because retained KSC-015 still references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1782
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

`dbuild.sh` returned status 0 despite the packaging failure. Therefore no matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit KSC-019 `dramboot_elf_smp` blocker inherited from KSC-015, not a PASS. KSC-019 now has all four required outcomes recorded as blockers; no new scenario was added because verification was pending at this cycle's start.

## 2026-08-16 KSC-020 added; verification pending

After every prior TEST-ID had a PASS, failure, or explicit blocker recorded in all four required configurations, KSC-020 was added as this cycle's exactly one new isolated scenario. It covers the blocking mutex ownership-handoff path, distinct from KSC-002's concurrent incrementing, KSC-008's nonblocking `EBUSY` result, and KSC-013's recursive ownership. The creator holds a mutex while one worker, configured with an affinity mask containing every CPU in `CONFIG_SMP_NCPUS`, publishes its imminent acquisition attempt. The creator uses separate two-second absolute semaphore deadlines to observe that attempt and the worker completion after releasing the mutex, joins the worker, validates its successful lock/unlock status, and destroys all synchronization objects. It prints affinity-mask evidence and is compatible with single-core paths. All four KSC-020 outcomes are pending; the next cycle must attempt them before adding another scenario. The retained KSC-015 unresolved `pthread_attr_setdetachstate` reference is expected to block image production unless it is made linkable.

## 2026-08-16 KSC-020 `dramboot_flat` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

Configuration and compilation completed through `hello_main.c`, including KSC-020 (`CC: hello_main.c` and `AR: hello_main.o`). The final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1788: undefined reference to `pthread_attr_setdetachstate'
Makefile:207: recipe for target '../build/output/bin/tinyara' failed
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned status 0 despite this linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-020 `dramboot_flat` blocker inherited from KSC-015, not a PASS; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending and must be attempted before adding another scenario.

## 2026-08-16 KSC-020 `dramboot_flat_smp` link blocker

The next pending configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

Configuration selected the SMP qemu-virt build and compilation completed through `hello_main.c`, including KSC-020 (`CC: hello_main.c` and `AR: hello_main.o`). The final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario still references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1788: undefined reference to `pthread_attr_setdetachstate'
Makefile:207: recipe for target '../build/output/bin/tinyara' failed
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

The `dbuild.sh` wrapper returned status 0 despite this linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-020 `dramboot_flat_smp` blocker inherited from KSC-015, not a PASS; `dramboot_elf` and `dramboot_elf_smp` remain pending and must be attempted before adding another scenario.

## 2026-08-16 KSC-020 `dramboot_elf` post-link blocker

The next pending configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

Configuration and compilation completed through `hello_main.c`, including KSC-020, and the kernel `tinyara` link completed. Final ELF application packaging could not produce a matching image because retained KSC-015 still references an unavailable detached-thread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1788
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

`dbuild.sh` returned status 0 despite the packaging failure. Therefore no matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit KSC-020 `dramboot_elf` blocker inherited from KSC-015, not a PASS; only `dramboot_elf_smp` remains pending and must be attempted before adding another scenario.

## 2026-08-16 KSC-020 `dramboot_elf_smp` post-link blocker

The final pending KSC-020 configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

Configuration selected qemu-virt ELF-SMP and compilation completed through `hello_main.c`, including KSC-020. The kernel `tinyara` link and `tinyara.bin` copy completed, but the final application packaging could not produce a matching image because retained KSC-015 still references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1788
make: *** [post] Error 1
Makefile.unix:551: recipe for target 'post' failed
```

`dbuild.sh` returned status 0 despite the packaging failure. Therefore no matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit KSC-020 `dramboot_elf_smp` blocker inherited from KSC-015, not a PASS. KSC-020 now has all four required configuration outcomes recorded; no new scenario was added because verification was pending at this cycle's start.

## 2026-08-16 KSC-021 added; verification pending

After every earlier TEST-ID had a PASS, failure, or explicit blocker recorded in each required configuration, KSC-021 was added as this cycle's exactly one isolated scenario. It covers pthread thread-identity comparison, which was not represented by the prior lifecycle, synchronization, TLS, or semaphore scenarios. Its one worker has an affinity mask built from every CPU in `CONFIG_SMP_NCPUS`, verifies `pthread_equal(pthread_self(), pthread_self())` is nonzero, posts completion, and is joined. Creator observation uses a two-second absolute semaphore deadline; all attribute and semaphore cleanup return values are validated, and the mask is printed so single-core and SMP evidence are explicit. All four KSC-021 outcomes are pending. The retained KSC-015 unresolved `pthread_attr_setdetachstate` reference is expected to block image production until it is resolved.

## 2026-08-16 KSC-021 `dramboot_flat` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The configuration and compilation completed through the new source (`CC: hello_main.c` and `AR: hello_main.o`), so KSC-021 itself compiled. The final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1792: undefined reference to `pthread_attr_setdetachstate'
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned status 0 despite the linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-021 `dramboot_flat` blocker inherited from KSC-015, not a PASS; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending and must be attempted before another scenario can be added.

## 2026-08-16 KSC-021 `dramboot_flat_smp` link blocker

The next pending configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The SMP configuration compiled the retained harness, including KSC-021 (`CC: hello_main.c` and `AR: hello_main.o`), but the final kernel link could not produce a matching image because retained KSC-015 still references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1792: undefined reference to `pthread_attr_setdetachstate'
Makefile:207: recipe for target '../build/output/bin/tinyara' failed
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned status 0 despite the final linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-021 `dramboot_flat_smp` blocker inherited from KSC-015, not a PASS; `dramboot_elf` and `dramboot_elf_smp` remain pending, so no new scenario was added.

## 2026-08-16 KSC-021 `dramboot_elf` post-link blocker

The next pending configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The configuration and compilation completed through the retained harness, and the kernel `tinyara` link completed (`LD: tinyara`, followed by `CP: tinyara.bin`). Final ELF application packaging could not produce a matching image because retained KSC-015 still references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1792
make: *** [post] Error 1
Makefile.unix:551: recipe for target 'post' failed
```

`dbuild.sh` returned status 0 despite the post-link packaging failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-021 `dramboot_elf` blocker inherited from KSC-015, not a PASS; only `dramboot_elf_smp` remains pending, so no new scenario was added.

## 2026-08-16 KSC-021 `dramboot_elf_smp` post-link blocker

The final pending KSC-021 configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The configuration compiled the retained harness, including KSC-021, and the kernel `tinyara` link and `tinyara.bin` copy completed. Final ELF application packaging could not produce a matching image because retained KSC-015 still references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1792
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

`dbuild.sh` returned exit status 0 despite the packaging failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-021 `dramboot_elf_smp` blocker inherited from KSC-015, not a PASS. KSC-021 now has all four required configuration outcomes recorded as blockers. No new scenario was added because verification was pending at this cycle's start.

## 2026-08-16 KSC-022 added; verification pending

All earlier TEST-IDs now have four recorded PASS, failure, or explicit blocker outcomes, so KSC-022 is this cycle's exactly one new isolated scenario. It covers a creator-to-worker semaphore wake handoff, distinct from KSC-012's empty-semaphore timeout, KSC-016's nonblocking acquisition, KSC-018/KSC-019 accounting, and earlier worker-to-creator completion posts. One worker is assigned an affinity mask containing every CPU derived from `CONFIG_SMP_NCPUS`; it first publishes readiness, waits on an initially empty release semaphore, then posts completion after exactly one creator wake token. The creator uses two-second absolute deadlines for readiness and completion, joins the worker, validates successful `sem_wait()`, and checks all attribute and semaphore cleanup operations. All four KSC-022 outcomes are pending. The retained KSC-015 unresolved `pthread_attr_setdetachstate` reference is expected to block matching image production until resolved.

## 2026-08-16 KSC-022 `dramboot_flat` link blocker

The first pending KSC-022 configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

Configuration and compilation reached the final kernel link, but no matching flat image was produced because retained KSC-015 references an unavailable pthread API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1798: undefined reference to `pthread_attr_setdetachstate'
Makefile:207: recipe for target '../build/output/bin/tinyara' failed
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned status 0 despite the linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-022 `dramboot_flat` blocker inherited from KSC-015, not a PASS; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending, so no new scenario was added.

## 2026-08-16 KSC-022 `dramboot_flat_smp` link blocker

The next pending KSC-022 configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The SMP configuration compiled `hello_main.c`, including KSC-022 (`CC: hello_main.c` and `AR: hello_main.o`), but the final kernel link could not produce a matching image because the retained KSC-015 detached-thread scenario references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1798: undefined reference to `pthread_attr_setdetachstate'
Makefile:207: recipe for target '../build/output/bin/tinyara' failed
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned status 0 despite this final linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-022 `dramboot_flat_smp` blocker inherited from KSC-015, not a PASS; `dramboot_elf` and `dramboot_elf_smp` remain pending, so no new scenario was added.

## 2026-08-16 KSC-022 `dramboot_elf` post-link blocker

The next pending KSC-022 configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

Configuration and compilation completed through `hello_main.c`, including KSC-022, and the kernel `tinyara` link and `tinyara.bin` copy completed. Final ELF application packaging could not produce a matching image because retained KSC-015 still references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1798
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

`dbuild.sh` returned status 0 despite the post-link packaging failure. Therefore no matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or QEMU termination was possible. This is an explicit KSC-022 `dramboot_elf` blocker inherited from KSC-015, not a PASS; only `dramboot_elf_smp` remains pending, so no new scenario was added.

## 2026-08-16 KSC-022 `dramboot_elf_smp` post-link blocker

The final pending KSC-022 configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf_smp && ./dbuild.sh
```

The build compiled the retained harness, linked `tinyara`, and copied `tinyara.bin`, but final ELF application packaging could not produce a matching image because retained KSC-015 references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1798
make: *** [post] Error 1
Makefile.unix:551: recipe for target 'post' failed
```

`dbuild.sh` returned exit status 0 despite the post-link packaging failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-022 `dramboot_elf_smp` blocker inherited from KSC-015, not a PASS. KSC-022 now has all four required outcomes recorded; no new scenario was added because verification was pending at this cycle's start.

## 2026-08-16 KSC-023 added; verification pending

All earlier TEST-IDs have four recorded PASS, failure, or explicit blocker outcomes, so KSC-023 is this cycle's exactly one new isolated scenario. It covers the condition-variable lost-signal behavior not represented by KSC-003's predicate signal, KSC-009's broadcast, or KSC-014's unsignaled timeout: `hello` signals a newly initialized condition variable while it has no waiters, then locks its mutex and requires a later `pthread_cond_timedwait()` using a two-second absolute `CLOCK_REALTIME` deadline to return `ETIMEDOUT`. Every initialization, signal, lock, timed wait, unlock, and destruction return value is checked. The scenario has no worker and remains compatible with single-core and SMP configurations. All four KSC-023 outcomes are pending; no KSC-024 may be added until each is recorded. The retained KSC-015 unresolved `pthread_attr_setdetachstate` reference is expected to prevent matching image production.

## 2026-08-16 KSC-023 `dramboot_flat` link blocker

The required configuration/build was attempted with the exact command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The new source compiled successfully (`CC: hello_main.c` and `AR: hello_main.o`), including KSC-023. The final kernel link could not produce a matching image because retained KSC-015 still references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1801: undefined reference to `pthread_attr_setdetachstate'
Makefile:207: recipe for target '../build/output/bin/tinyara' failed
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned exit status 0 despite this linker failure. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-023 `dramboot_flat` blocker inherited from KSC-015, not a PASS; `dramboot_flat_smp`, `dramboot_elf`, and `dramboot_elf_smp` remain pending, so no new scenario was added.

## 2026-08-16 KSC-023 `dramboot_flat_smp` link blocker

The next pending KSC-023 configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat_smp && ./dbuild.sh
```

The SMP configuration compiled the retained harness, including `CC: hello_main.c` and `AR: hello_main.o`, but the final kernel link could not produce a matching image because retained KSC-015 still references an unavailable pthread attribute API:

```text
LD: tinyara
arm-none-eabi-ld: /root/tizenrt/os/../build/output/libraries/libapps.a(hello_main.o): in function `ksc015_detached_thread':
/root/tizenrt/apps/examples/hello/hello_main.c:1801: undefined reference to `pthread_attr_setdetachstate'
make[1]: *** [../build/output/bin/tinyara] Error 1
make: *** [pass2] Error 2
```

`dbuild.sh` returned exit status 0 despite the linker failure, and the job confirmed `TINYARA_PRESENT=0`. No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-023 `dramboot_flat_smp` blocker inherited from KSC-015, not a PASS; `dramboot_elf` and `dramboot_elf_smp` remain pending, so no new scenario was added.

## 2026-08-16 KSC-023 `dramboot_elf` post-link blocker

The next pending KSC-023 configuration was attempted with the exact required command:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_elf && ./dbuild.sh
```

The ELF configuration compiled the retained harness, linked `tinyara`, and copied `tinyara.bin`, but final application packaging could not produce a matching KSC-023 image because retained KSC-015 still references an unavailable pthread attribute API:

```text
Preparing final ../build/output/bin/app1 binary
Verify ../build/output/bin/common
Undefined Symbols in ../build/output/bin/common
         U pthread_attr_setdetachstate    /root/tizenrt/apps/examples/hello/hello_main.c:1801
Makefile.unix:551: recipe for target 'post' failed
make: *** [post] Error 1
```

`dbuild.sh` returned exit status 0 despite this post-link packaging failure (the job observed `TINYARA_PRESENT=1`, which is not evidence of a matching application image). No matching image refresh, literal root-level `./run_qemu.sh` boot, `TASH>>`, `hello` invocation, TEST-ID output, or clean QEMU termination was possible. This is an explicit KSC-023 `dramboot_elf` blocker inherited from KSC-015, not a PASS; only `dramboot_elf_smp` remains pending, and no new scenario was added.
