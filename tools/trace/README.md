# TizenRT note conversion

`note2ftrace.py` converts raw scheduler notes produced by
`trace dump -b` into the textual ftrace format accepted by Trace Compass.

The initial implementation targets `qemu-virt/dramboot_elf`:

- little-endian ARM32 ABI
- 16-bit `pid_t`
- 64-bit `clock_t`
- task start, stop, suspend, and resume notes

New raw dumps contain a versioned header with the timer frequency and a
PID/task-name snapshot:

```sh
python3 tools/trace/note2ftrace.py \
  trace.bin -o trace.txt
```

Import `trace.txt` into Trace Compass using the ftrace trace type.

Legacy headerless raw files are also accepted. They default to qemu-virt's
62.5 MHz timer, which can be overridden with `--frequency`. For legacy files,
task names are learned only from `NOTE_START` records, and tasks that existed
before recording began can therefore appear as `<noname>`. Idle task names
are reconstructed from the reserved idle PID range.

If the RAM ring buffer starts in the middle of an overwritten record, the
converter skips the incomplete leading bytes and reports the skipped length.
