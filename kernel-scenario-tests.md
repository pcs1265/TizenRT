# qemu-virt kernel scenario test index

This index records executable scenarios in `apps/examples/hello/hello_main.c`. A
scenario is marked **pass** only after its `hello` output has been captured on
both required qemu-virt configurations.

| TEST-ID | Source location | Kernel behavior / trigger | Expected outcome | Single-core (`dramboot_flat`) | SMP (`dramboot_flat_smp`) | State |
|---|---|---|---|---|---|---|
| KSC-001 | `apps/examples/hello/hello_main.c:82-162` | Scheduler/task lifecycle: create a pthread worker, have it post a semaphore, return a unique exit token, then `pthread_join()` it. `sem_timedwait()` uses a 2-second deadline; failure cancels and joins the worker before semaphore cleanup. | `KSC-001: PASS task create -> wake -> exit -> join` and harness PASS. | Build completed on 2026-08-16 02:22 UTC, but **not run**: image refresh was blocked before QEMU boot, so no TASH `hello` evidence exists. | Pending; not built or run in this corrective cycle because the required single-core image-refresh/boot step was blocked. | **pending verification** |

## 2026-08-16 corrective-run note

The requested literal `./dbuild.sh distclean configure qemu-virt/dramboot_flat`
flow is not accepted by this revision of `dbuild.sh`: after `distclean` it
parses `qemu-virt/dramboot_flat` as a numeric board selection and exits with
`division by 0`. Its noninteractive equivalent was used to configure and build
single-core: `./dbuild.sh qemu-virt dramboot_flat && ./dbuild.sh`; the build
completed successfully. The normal `./dbuild.sh download ...` path allocates a
TTY, so the required documented non-TTY equivalent was attempted instead:

```
docker run --rm -i -v /home/pcs1265/TizenRT/.hermes/bughunt-worktree:/root/tizenrt \
  -w /root/tizenrt/os --privileged tizenrt/tizenrt:2.0.0 make download
```

The execution environment denied that privileged Docker invocation before it
ran. Consequently no freshly matched image could be booted, no TASH `hello`
command was invoked, and neither configuration is recorded as passing. Existing
root-level `qemu_flash.bin`, `qemu_blk.bin`, and `run_qemu.sh` are untracked and
were not used as verification artifacts.

## KSC-002 — mutex contention and ownership handoff

| TEST-ID | Source location | Kernel behavior / trigger | Expected outcome | `dramboot_flat` | `dramboot_flat_smp` | `dramboot_elf` | `dramboot_elf_smp` | State |
|---|---|---|---|---|---|---|---|---|
| KSC-002 | `apps/examples/hello/hello_main.c:171-283` | Create two pthreads, release both through a start semaphore, then have each make 32 mutex-protected counter updates. Main observes both completions with one 2-second absolute deadline, joins all created workers, and destroys both semaphores. Failure cleanup cancels then joins every created worker. | `KSC-002: PASS mutex handoff counter=64` and harness PASS. | **Build passed** on 2026-08-16. Image was refreshed with `printf '0\n' \| TOPDIR="$PWD" bash build/configs/qemu-virt/qemu-virt_download.sh all` after `./dbuild.sh download` returned without performing the refresh. Literal `./run_qemu.sh` was started, but QEMU exited before TASH because the pre-existing process PID 2237810 held both image locks: `Failed to get "write" lock ... qemu_blk.bin`. No `hello` output captured. | Not built or booted: image-lock blocker must be cleared first. | Not built or booted: image-lock blocker must be cleared first. | Not built or booted: image-lock blocker must be cleared first. | **pending verification** |

### 2026-08-16 KSC-002 execution blocker

The exact required flat build command completed successfully:

```sh
cd os && ./dbuild.sh distclean configure qemu-virt dramboot_flat && ./dbuild.sh
```

The required root-level invocation `./run_qemu.sh` then failed before boot/TASH:

```text
qemu-system-arm: -device virtio-blk-device,drive=blk0,bus=virtio-mmio-bus.0: Failed to get "write" lock
Is another process using the image [qemu_blk.bin]?
```

`fuser -v qemu_blk.bin qemu_flash.bin` identified a separate pre-existing
`qemu-system-arm` PID 2237810 as the holder. It was not terminated because this
job did not create it. The next run must clear or otherwise isolate that lock,
then build, refresh, invoke `./run_qemu.sh`, send `hello`, capture KSC-001 and
KSC-002 output, and cleanly terminate this job's QEMU for all four modes.
