# qemu-virt kernel scenario test index

Scenarios live in `apps/examples/hello/hello_main.c`. A **PASS** requires a matching build, image refresh, literal root-level `./run_qemu.sh` boot to `TASH>>`, `hello` output, and clean QEMU termination.

| TEST-ID | Source / trigger | Expected outcome | `dramboot_flat` | `dramboot_flat_smp` | `dramboot_elf` | `dramboot_elf_smp` | State |
|---|---|---|---|---|---|---|---|
| KSC-001 | `hello_main.c`: task lifecycle; worker posts a semaphore, returns a token, and creator timed-waits then joins. | `KSC-001: PASS task create -> wake -> exit -> join` | PASS | PASS | PASS | PASS | pass |
| KSC-002 | `hello_main.c`: two workers make 32 mutex-protected updates after a common semaphore release. | `KSC-002: PASS mutex handoff counter=64` | PASS | PASS | PASS | PASS | pass |
| KSC-003 | `hello_main.c`: worker changes a condition-variable predicate under its mutex and signals; creator predicate-waits with a two-second deadline. | `KSC-003: PASS condition predicate=1` | PASS | PASS | PASS | PASS | pass |
| KSC-004 | `hello_main.c`: worker with affinity covering all CPUs in `CONFIG_SMP_NCPUS` sets and reads a pthread TLS key, signals a semaphore, and is joined; all waits have a two-second deadline and cleanup destroys the key, attribute, and semaphore. | `KSC-004: PASS thread-specific value=0 mask=...` and affinity evidence. | PASS | PASS | PASS | PASS | pass |
| KSC-005 | `hello_main.c`: two all-active-CPU-affined workers concurrently call `pthread_once`; each completion is semaphore-timed and both are joined, while the single initializer count is checked. | `KSC-005: PASS pthread_once initializer count=1 mask=...` and affinity evidence. | PASS | Pending | Pending | PASS | pending verification |

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
