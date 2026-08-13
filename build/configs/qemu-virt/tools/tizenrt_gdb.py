"""TizenRT OS-awareness commands for the qemu-virt GDB session.

This file is intentionally qemu-virt tooling; it does not modify the common
TizenRT build scripts or kernel sources.
"""
import gdb


_STATE_NAMES = {
    0: "INVALID",
    1: "PENDING",
    2: "READY",
    3: "ASSIGNED",
    4: "RUNNING",
    5: "INACTIVE",
    6: "WAIT_SEM",
    7: "WAIT_FIN",
    8: "WAIT_SIG",
    9: "WAIT_MQEMPTY",
    10: "WAIT_MQFULL",
    11: "WAIT_PAGEFILL",
}

_QUEUE_STATES = (
    ("g_readytorun", "READY/RUNNING"),
    ("g_pendingtasks", "PENDING"),
    ("g_inactivetasks", "INACTIVE"),
    ("g_waitingforsemaphore", "WAIT_SEM"),
    ("g_waitingforfin", "WAIT_FIN"),
    ("g_waitingforsignal", "WAIT_SIG"),
    ("g_waitingformqnotempty", "WAIT_MQEMPTY"),
    ("g_waitingformqnotfull", "WAIT_MQFULL"),
)


def _eval(name):
    return gdb.parse_and_eval(name)


def _int(value, default=0):
    try:
        return int(value)
    except (TypeError, gdb.error):
        return default


def _field(value, name, default=None):
    try:
        return value[name]
    except (KeyError, gdb.error, TypeError):
        return default


def _ptr(value):
    if value is None:
        return 0
    try:
        return int(value)
    except (TypeError, gdb.error):
        return 0


def _name(tcb):
    value = _field(tcb, "name")
    if value is None:
        return "<unnamed>"
    try:
        return value.string("utf-8", "replace")
    except (UnicodeError, gdb.error):
        return str(value).strip('"')


def _state(tcb):
    number = _int(_field(tcb, "task_state"), -1)
    return _STATE_NAMES.get(number, "STATE_%d" % number)


def _cpu(tcb):
    value = _field(tcb, "cpu")
    return str(_int(value)) if value is not None else "-"


def _task_info(tcb, source):
    if not tcb:
        return None
    return {
        "address": tcb,
        "source": source,
        "pid": _int(_field(tcb, "pid"), -1),
        "name": _name(tcb),
        "state": _state(tcb),
        "priority": _int(_field(tcb, "sched_priority"), -1),
        "cpu": _cpu(tcb),
        "stack_size": _int(_field(tcb, "adj_stack_size"), 0),
        "stack_base": _ptr(_field(tcb, "stack_base_ptr")),
        "stack_ptr": _ptr(_field(tcb, "adj_stack_ptr")),
    }


def _queue_tasks(symbol, source):
    try:
        queue = _eval(symbol)
        current = queue["head"]
    except (gdb.error, KeyError, TypeError):
        return []

    result = []
    seen = set()
    while _ptr(current):
        address = _ptr(current)
        if address in seen:
            break
        seen.add(address)
        try:
            tcb = current.cast(gdb.lookup_type("struct tcb_s").pointer())
        except gdb.error:
            break
        info = _task_info(tcb, source)
        if info:
            result.append(info)
        current = _field(tcb, "flink")
    return result


def _running_tasks():
    result = []
    try:
        running = _eval("g_running_tasks")
        # CONFIG_SMP_NCPUS is normally one in dramboot_elf; stop at the
        # first null entry because the array length is not needed here.
        for cpu in range(32):
            try:
                tcb = running[cpu]
            except gdb.error:
                break
            if not _ptr(tcb):
                if cpu:
                    break
                continue
            info = _task_info(tcb, "RUNNING")
            if info:
                info["cpu"] = str(cpu)
                result.append(info)
    except gdb.error:
        pass
    return result


def _all_tasks():
    result = []
    seen = set()
    for info in _running_tasks():
        key = info["pid"] if info["pid"] >= 0 else _ptr(info["address"])
        seen.add(key)
        result.append(info)
    for symbol, source in _QUEUE_STATES:
        for info in _queue_tasks(symbol, source):
            key = info["pid"] if info["pid"] >= 0 else _ptr(info["address"])
            if key not in seen:
                seen.add(key)
                result.append(info)
    return sorted(result, key=lambda item: item["pid"])


def _stack_used(info):
    """Estimate used bytes from TizenRT's stack coloration pattern."""
    base = info["stack_base"]
    end = info["stack_ptr"]
    if not base or not end or end <= base:
        return None
    try:
        memory = gdb.selected_inferior().read_memory(base, end - base)
        raw = bytes(memory)
    except (gdb.error, ValueError):
        return None
    used_from_base = 0
    for offset in range(0, len(raw) - 3, 4):
        if raw[offset:offset + 4] != b"\xef\xbe\xad\xde":
            used_from_base = len(raw) - offset
            break
    else:
        used_from_base = 0
    return min(info["stack_size"], used_from_base)


def _print_task(info, with_stack=False):
    stack = "-"
    if with_stack:
        used = _stack_used(info)
        stack = "%s/%s" % (used, info["stack_size"]) if used is not None else "?/%s" % info["stack_size"]
    print("%-5s %-16s %-13s %-4s %-4s %-14s %s" % (
        info["pid"], info["name"][:16], info["state"],
        info["priority"], info["cpu"], stack, info["source"]))


class TizenRTPrefix(gdb.Command):
    """TizenRT qemu-virt OS-awareness command prefix."""
    def __init__(self):
        super(TizenRTPrefix, self).__init__("tizenrt", gdb.COMMAND_USER, prefix=True)


class TizenRTTasks(gdb.Command):
    """Show TizenRT tasks: tizenrt tasks [all|running]."""
    def __init__(self):
        super(TizenRTTasks, self).__init__("tizenrt tasks", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        mode = argument.strip().lower()
        tasks = _running_tasks() if mode == "running" else _all_tasks()
        print("PID   NAME             STATE         PRI  CPU  SOURCE")
        print("----- ---------------- ------------- ---- ---- ----------------")
        for info in tasks:
            _print_task(info)
        print("%d task(s)" % len(tasks))


class TizenRTCurrent(gdb.Command):
    """Show the TizenRT task currently running on each CPU."""
    def __init__(self):
        super(TizenRTCurrent, self).__init__("tizenrt current", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        tasks = _running_tasks()
        if not tasks:
            print("TizenRT running-task symbols are unavailable")
            return
        print("PID   NAME             STATE         PRI  CPU  SOURCE")
        print("----- ---------------- ------------- ---- ---- ----------------")
        for info in tasks:
            _print_task(info)


class TizenRTTask(gdb.Command):
    """Show one TizenRT task: tizenrt task PID."""
    def __init__(self):
        super(TizenRTTask, self).__init__("tizenrt task", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        try:
            wanted = int(argument.strip(), 0)
        except ValueError:
            print("usage: tizenrt task PID")
            return
        for info in _all_tasks():
            if info["pid"] == wanted:
                _print_task(info)
                print("TCB: %s" % info["address"])
                print("stack size: %d bytes" % info["stack_size"])
                return
        print("TizenRT task %d not found" % wanted)


def _sem_value(sem):
    return _int(_field(sem, "semcount"), 0)


def _sem_flags(sem):
    return "0x%02x" % (_int(_field(sem, "flags"), 0) & 0xff)


def _holder_tasks(sem):
    """Return holder TCBs when SAVE_SEM_HOLDER is enabled."""
    holders = []
    try:
        holder = _field(sem, "hhead")
        if holder is not None:
            seen = set()
            while _ptr(holder) and _ptr(holder) not in seen:
                seen.add(_ptr(holder))
                tcb = _field(holder, "htcb")
                if _ptr(tcb):
                    holders.append(tcb)
                holder = _field(holder, "flink")
            return holders
    except (gdb.error, TypeError):
        pass
    try:
        holder = _field(sem, "holder")
        tcb = _field(holder, "htcb")
        if _ptr(tcb):
            holders.append(tcb)
    except (gdb.error, TypeError):
        pass
    return holders


def _waiting_tasks():
    return _queue_tasks("g_waitingforsemaphore", "WAIT_SEM")


class TizenRTWaiters(gdb.Command):
    """Show tasks blocked on semaphores: tizenrt waiters."""
    def __init__(self):
        super(TizenRTWaiters, self).__init__("tizenrt waiters", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        tasks = _waiting_tasks()
        print("PID   NAME             SEMAPHORE       COUNT  FLAGS  HOLDER")
        print("----- ---------------- --------------- ------ ------ ----------------")
        for info in tasks:
            sem = _field(info["address"], "waitsem")
            if not _ptr(sem):
                continue
            holders = _holder_tasks(sem)
            holder_names = ",".join(_name(tcb) for tcb in holders) or "-"
            print("%-5s %-16s 0x%08x      %-6s %-6s %s" % (
                info["pid"], info["name"][:16], _ptr(sem),
                _sem_value(sem), _sem_flags(sem), holder_names[:32]))
        print("%d semaphore waiter(s)" % len(tasks))


class TizenRTHeld(gdb.Command):
    """Show semaphores held by tasks: tizenrt held."""
    def __init__(self):
        super(TizenRTHeld, self).__init__("tizenrt held", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        tasks = _all_tasks()
        printed = 0
        for info in tasks:
            holdsem = _field(info["address"], "holdsem")
            seen = set()
            while _ptr(holdsem) and _ptr(holdsem) not in seen:
                seen.add(_ptr(holdsem))
                sem = _field(holdsem, "sem")
                if _ptr(sem):
                    print("PID %-5s %-16s SEM 0x%08x COUNT %-4s FLAGS %s" % (
                        info["pid"], info["name"][:16], _ptr(sem),
                        _sem_value(sem), _sem_flags(sem)))
                    printed += 1
                holdsem = _field(holdsem, "tlink")
        print("%d held semaphore(s)" % printed)


class TizenRTStack(gdb.Command):
    """Show stack usage: tizenrt stack [PID]."""
    def __init__(self):
        super(TizenRTStack, self).__init__("tizenrt stack", gdb.COMMAND_USER)

    def invoke(self, argument, from_tty):
        argument = argument.strip()
        tasks = _all_tasks()
        if argument:
            try:
                wanted = int(argument, 0)
            except ValueError:
                print("usage: tizenrt stack [PID]")
                return
            tasks = [item for item in tasks if item["pid"] == wanted]
        print("PID   NAME             USED/TOTAL      BASE        SP")
        print("----- ---------------- --------------- ---------- ----------")
        for info in tasks:
            used = _stack_used(info)
            used_text = "%s/%s" % (used, info["stack_size"]) if used is not None else "?/%s" % info["stack_size"]
            print("%-5s %-16s %-15s 0x%08x 0x%08x" % (
                info["pid"], info["name"][:16], used_text,
                info["stack_base"], info["stack_ptr"]))


TizenRTPrefix()
TizenRTTasks()
TizenRTCurrent()
TizenRTTask()
TizenRTStack()
TizenRTWaiters()
TizenRTHeld()
print("TizenRT qemu-virt OS awareness loaded (tasks, current, task, stack, waiters, held)")
