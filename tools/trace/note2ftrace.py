#!/usr/bin/env python3
"""Convert qemu-virt TizenRT binary scheduler notes to ftrace text."""

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO, Dict, Iterator, Optional, TextIO, Tuple


NOTE_START = 0
NOTE_STOP = 1
NOTE_SUSPEND = 2
NOTE_RESUME = 3

COMMON_SIZE = 16
TSTATE_TASK_RUNNING = 4
RAW_MAGIC = b"TZNOTE01"
QEMU_TIMER_FREQUENCY = 62_500_000


class NoteFormatError(ValueError):
    """Raised when an input stream does not contain valid note records."""


@dataclass
class CommonNote:
    length: int
    note_type: int
    priority: int
    cpu: int
    pid: int
    timestamp: int


@dataclass
class CpuContext:
    current_pid: int = -1
    current_priority: int = 255
    current_state: int = TSTATE_TASK_RUNNING


def plausible_record(data: bytes, offset: int) -> bool:
    if offset + COMMON_SIZE > len(data):
        return False

    length = data[offset]
    note_type = data[offset + 1]
    if length < COMMON_SIZE or offset + length > len(data):
        return False

    if note_type == NOTE_START:
        return length > COMMON_SIZE
    if note_type in (NOTE_STOP, NOTE_RESUME):
        return length == COMMON_SIZE
    if note_type == NOTE_SUSPEND:
        return length > COMMON_SIZE

    return False


def find_record_start(data: bytes) -> int:
    for candidate in range(len(data)):
        offset = candidate
        valid = 0

        while valid < 4 and plausible_record(data, offset):
            offset += data[offset]
            valid += 1
            if offset == len(data):
                return candidate

        if valid == 4:
            return candidate

    raise NoteFormatError("no scheduler note records found")


def read_records(stream: BinaryIO) -> Iterator[Tuple[int, bytes]]:
    data = stream.read()
    if not data:
        return

    offset = find_record_start(data)
    if offset:
        print(
            f"note2ftrace: skipped {offset} leading byte(s) before "
            "the first complete record",
            file=sys.stderr,
        )

    while offset < len(data):
        if not plausible_record(data, offset):
            length = data[offset]
            note_type = data[offset + 1] if offset + 1 < len(data) else -1
            raise NoteFormatError(
                f"invalid record at offset 0x{offset:x}: "
                f"length={length}, type={note_type}"
            )

        length = data[offset]
        yield offset, data[offset : offset + length]
        offset += length


def parse_common(record: bytes) -> CommonNote:
    length, note_type, priority, cpu, pid = struct.unpack_from("<BBBBh", record)
    timestamp = struct.unpack_from("<Q", record, 8)[0]
    return CommonNote(length, note_type, priority, cpu, pid, timestamp)


class FtraceConverter:
    def __init__(
        self,
        frequency: int,
        cpu_count: int = 1,
        task_names: Optional[Dict[int, str]] = None,
    ):
        if frequency <= 0:
            raise ValueError("timer frequency must be greater than zero")
        if cpu_count <= 0:
            raise ValueError("CPU count must be greater than zero")

        self.frequency = frequency
        self.cpu_count = cpu_count
        self.contexts: Dict[int, CpuContext] = {}
        self.task_names = dict(task_names or {})

    def task_name(self, pid: int) -> str:
        if 0 <= pid < self.cpu_count:
            return "Idle Task"
        return self.task_names.get(pid, "<noname>")

    def display_pid(self, pid: int) -> int:
        return 0 if 0 <= pid < self.cpu_count else pid

    def header(self, note: CommonNote) -> str:
        seconds, ticks = divmod(note.timestamp, self.frequency)
        nanoseconds = ticks * 1_000_000_000 // self.frequency
        name = self.task_name(note.pid)
        return (
            f"{name:>8}-{self.display_pid(note.pid):<3} "
            f"[{note.cpu}] {seconds:3}.{nanoseconds:09}: "
        )

    def convert(self, record: bytes) -> str:
        note = parse_common(record)
        context = self.contexts.setdefault(note.cpu, CpuContext())

        if note.note_type == NOTE_START:
            raw_name = record[COMMON_SIZE:note.length].split(b"\0", 1)[0]
            self.task_names[note.pid] = raw_name.decode(
                "utf-8", errors="replace"
            ) or "<noname>"

        if context.current_pid < 0:
            context.current_pid = note.pid

        if note.note_type == NOTE_START:
            return (
                self.header(note)
                + "sched_wakeup_new: "
                + f"comm={self.task_name(note.pid)} "
                + f"pid={self.display_pid(note.pid)} "
                + f"target_cpu={note.cpu}\n"
            )

        if note.note_type == NOTE_STOP:
            context.current_state = 0
            return ""

        if note.note_type == NOTE_SUSPEND:
            if note.length <= COMMON_SIZE:
                raise NoteFormatError("NOTE_SUSPEND record has no state field")
            context.current_state = record[COMMON_SIZE]
            return ""

        if note.note_type == NOTE_RESUME:
            state = (
                "X"
                if context.current_state == 0
                else "R"
                if context.current_state <= TSTATE_TASK_RUNNING
                else "S"
            )
            current_pid = context.current_pid
            text = (
                self.header(note)
                + "sched_switch: "
                + f"prev_comm={self.task_name(current_pid)} "
                + f"prev_pid={self.display_pid(current_pid)} "
                + f"prev_prio={context.current_priority} "
                + f"prev_state={state} ==> "
                + f"next_comm={self.task_name(note.pid)} "
                + f"next_pid={self.display_pid(note.pid)} "
                + f"next_prio={note.priority}\n"
            )
            context.current_pid = note.pid
            context.current_priority = note.priority
            return text

        raise NoteFormatError(f"unsupported note type {note.note_type}")


def read_metadata(
    source: BinaryIO,
) -> Tuple[Optional[int], Dict[int, str]]:
    magic = source.read(len(RAW_MAGIC))
    if magic != RAW_MAGIC:
        source.seek(0)
        return None, {}

    header = source.read(8)
    if len(header) != 8:
        raise NoteFormatError("truncated binary trace header")

    frequency, count, name_size = struct.unpack("<IHH", header)
    if frequency == 0 or name_size == 0 or name_size > 256:
        raise NoteFormatError("invalid binary trace header")

    names: Dict[int, str] = {}
    for _ in range(count):
        entry = source.read(2 + name_size)
        if len(entry) != 2 + name_size:
            raise NoteFormatError("truncated task-name table")

        pid = struct.unpack_from("<h", entry)[0]
        name = entry[2:].split(b"\0", 1)[0].decode(
            "utf-8", errors="replace"
        )
        if name:
            names[pid] = name

    return frequency, names


def convert_file(
    source: BinaryIO,
    destination: TextIO,
    frequency: Optional[int],
    cpu_count: int,
) -> None:
    stored_frequency, task_names = read_metadata(source)
    frequency = frequency or stored_frequency or QEMU_TIMER_FREQUENCY
    converter = FtraceConverter(frequency, cpu_count, task_names)
    destination.write("# tracer: nop\n#\n")

    for offset, record in read_records(source):
        try:
            destination.write(converter.convert(record))
        except NoteFormatError as error:
            raise NoteFormatError(f"{error} at offset 0x{offset:x}") from error


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Convert raw scheduler notes from qemu-virt/dramboot_elf "
            "to Trace Compass-compatible ftrace text."
        )
    )
    parser.add_argument("input", type=Path, help="raw file from trace dump -b")
    parser.add_argument(
        "-o", "--output", type=Path, help="output file (default: stdout)"
    )
    parser.add_argument(
        "--frequency",
        type=int,
        help=(
            "override the timer frequency stored in the trace "
            "(legacy default: 62500000)"
        ),
    )
    parser.add_argument(
        "--cpus", type=int, default=1, help="number of CPUs (default: 1)"
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    try:
        with args.input.open("rb") as source:
            if args.output:
                with args.output.open("w", encoding="utf-8", newline="\n") as out:
                    convert_file(source, out, args.frequency, args.cpus)
            else:
                convert_file(source, sys.stdout, args.frequency, args.cpus)
    except (OSError, NoteFormatError, ValueError) as error:
        print(f"note2ftrace: {error}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
