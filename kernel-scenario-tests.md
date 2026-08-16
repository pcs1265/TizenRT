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
| KSC-014 | `hello_main.c`: creator locks an unsignaled condition variable's mutex, uses `pthread_cond_timedwait()` with an absolute two-second deadline, and destroys the condition variable and mutex after the expected timeout. | `KSC-014: PASS condition timeout status=110` | PASS | PASS | PASS | pending | pending |

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
