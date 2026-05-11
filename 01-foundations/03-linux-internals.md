# Linux Internals for Security Research

Understanding how Linux manages processes, memory, and files is essential context for every exploit technique you will learn.

---

## Processes

A process is a running instance of a program. The kernel manages processes and enforces isolation between them.

```
Program (file on disk)  --exec-->  Process (running in memory)
```

### Process Properties

```bash
ps aux                  # list all running processes
ps aux | grep firefox   # find a specific process
cat /proc/self/status   # info about the current process
cat /proc/<pid>/maps    # memory map of a process (virtual address space)
```

### Process States

```
Running   -> Currently executing on a CPU core
Sleeping  -> Waiting for I/O or an event
Stopped   -> Paused (e.g., by SIGSTOP or a debugger)
Zombie    -> Exited but not yet waited on by parent
```

---

## Virtual Memory

Each process gets its own virtual address space. The kernel + MMU translate virtual addresses to physical RAM.

```
Virtual Address Space (per process):
+-----------------------------------+  0xFFFFFFFFFFFFFFFF
|          Kernel Space             |  <- shared, not accessible to users
+-----------------------------------+  0xFFFF800000000000
|                                   |
|          (unmapped gap)           |
|                                   |
+-----------------------------------+  depends on ASLR
|            Stack                  |  grows down
|     [argc, argv, envp,            |
|      return addrs, locals]        |
|                                   |
|            Heap                   |  grows up (malloc)
|                                   |
|       Libraries (.so files)       |  e.g., libc.so
|                                   |
|       Program binary              |  .text, .data, .bss
+-----------------------------------+  0x0000000000000000
```

```bash
# See a process's memory map live
cat /proc/self/maps

# Example output:
# 55a3c3a00000-55a3c3a01000 r--p 00000000 fd:01 1234  /bin/ls
# ^start addr  ^end addr    ^perms           ^file
```

### Memory Permissions

Each memory region has permissions:

| Flag | Meaning |
|---|---|
| `r` | Readable |
| `w` | Writable |
| `x` | Executable |
| `p` | Private (copy-on-write) |
| `s` | Shared |

NX (No eXecute) works by marking the stack and heap as `rw-` (not executable).

---

## File Descriptors

In Linux, almost everything is treated as a file: regular files, sockets, pipes, devices.

A file descriptor (fd) is a small integer your process uses to reference an open file.

```
fd 0  ->  stdin   (standard input)
fd 1  ->  stdout  (standard output)
fd 2  ->  stderr  (standard error)
fd 3+ ->  opened by your program
```

```c
int fd = open("file.txt", O_RDONLY);  // returns 3 (or higher)
read(fd, buf, 100);
close(fd);
```

---

## Syscalls

Syscalls are the interface between userspace (your program) and the kernel.

```
User Program
    |
    |  int 0x80 / syscall instruction
    v
  Kernel
    |
    |  does the work (read file, allocate memory, etc.)
    v
Returns result to userspace in RAX
```

```bash
# Trace syscalls made by a program
strace ./program
strace -e trace=read,write,open ./program   # filter specific syscalls

# Example output:
# execve("./program", ["./program"], ...) = 0
# open("/etc/passwd", O_RDONLY)  = 3
# read(3, "root:x:0:0:...", 4096) = 1234
# write(1, "output\n", 7) = 7
```

---

## Signals

Signals are asynchronous notifications sent to a process.

| Signal | Number | Default Action | Common Cause |
|---|---|---|---|
| SIGSEGV | 11 | Crash (core dump) | Invalid memory access |
| SIGBUS | 7 | Crash | Misaligned memory access |
| SIGFPE | 8 | Crash | Division by zero |
| SIGKILL | 9 | Terminate (unblockable) | `kill -9` |
| SIGTERM | 15 | Terminate | `kill` (default) |
| SIGINT | 2 | Terminate | Ctrl+C |
| SIGTRAP | 5 | Trap | Debugger breakpoint |
| SIGSTOP | 19 | Stop (unblockable) | Debugger pause |

When a program crashes with `Segmentation fault`, it received `SIGSEGV` — usually from accessing an invalid memory address (classic sign of a bug or overflow).

```bash
kill -SIGSEGV <pid>    # send signal to process
kill -l                 # list all signals
```

---

## ELF Binary Format

Linux executables use the ELF (Executable and Linkable Format) structure.

```
ELF File:
+------------------+
|   ELF Header     |  magic bytes, architecture, entry point address
+------------------+
|  Program Headers |  tells the loader how to map the file into memory
+------------------+
|   .text          |  executable code
|   .rodata        |  read-only data (string literals)
|   .data          |  initialized global/static variables
|   .bss           |  uninitialized globals (zeroed at startup)
|   .plt           |  Procedure Linkage Table (lazy linking stubs)
|   .got           |  Global Offset Table (resolved addresses)
|   .got.plt       |  GOT entries for PLT
+------------------+
|  Section Headers |  metadata about sections (for linkers/debuggers)
+------------------+
```

```bash
# Inspect an ELF binary
file ./binary                  # confirm it's ELF, architecture
readelf -h ./binary            # ELF header
readelf -S ./binary            # section headers
readelf -l ./binary            # program headers (segments)
readelf --syms ./binary        # symbol table
objdump -d ./binary            # disassemble .text
```

### PLT and GOT (Lazy Binding)

#### The problem

When you compile a program that calls `printf`, the compiler does not know where `printf` will be in memory at runtime. `printf` lives inside `libc.so`, a shared library that can be loaded at a different address on every run (ASLR). The address is only known at runtime — not at compile time.

Two data structures work together to solve this.

#### GOT — Global Offset Table

A table of **addresses** stored in a writable section of the binary (`.got.plt`). There is one entry per external function the binary uses. Think of it as a phone book that starts out empty and gets filled in as calls are made.

#### PLT — Procedure Linkage Table

A table of small **code stubs** stored in an executable section (`.plt`). There is one stub per external function. Each stub does one thing: jump to whatever address is currently in the matching GOT entry.

```
.plt (executable, code)        .got.plt (writable, data)
+----------------------+        +---------------------+
|  printf@plt:         |        |  printf@got:        |
|    jmp [printf@got]  | -----> |  <address>          |
+----------------------+        +---------------------+
```

#### First call — lazy resolution

On the very first call to `printf`, the GOT entry has not been filled in yet. It points back into the PLT resolver instead.

```
Step 1:  call printf
           │
           ▼
Step 2:  printf@plt  (PLT stub)
           │  jmp [printf@got]
           │  GOT entry currently = address of PLT resolver
           ▼
Step 3:  PLT resolver
           │  pushes printf's index onto the stack
           │  calls the dynamic linker (ld-linux.so)
           ▼
Step 4:  Dynamic linker
           │  searches libc for the real printf address
           │  writes that address into printf@got  ← GOT is now patched
           ▼
Step 5:  Jumps to the real printf in libc
```

#### Subsequent calls — direct jump

The GOT entry now holds the real address. The PLT stub jumps straight there — the dynamic linker is never involved again.

```
call printf
    │
    ▼
printf@plt
    │  jmp [printf@got]
    │  GOT entry = real printf in libc  ✓
    ▼
printf() in libc  (direct, no linker overhead)
```

#### Why this matters for exploitation

The GOT is a **writable** section in memory (at least with Partial RELRO). If an attacker can write an arbitrary value to a GOT entry, they redirect every future call to that function to an address of their choosing.

```
Normal:    got['printf'] = 0x7f...  (real printf in libc)
           → printf("hello") prints "hello"

Exploited: got['printf'] = system()
           → printf("/bin/sh") calls system("/bin/sh") → shell
```

This is why Full RELRO exists: it makes the GOT read-only after startup, preventing this class of attack.

---

## Dynamic Linking

```bash
ldd ./binary                   # list shared libraries a binary depends on
ldd ./binary | grep libc       # find libc path

# Find function offsets in libc
nm -D /lib/x86_64-linux-gnu/libc.so.6 | grep " puts"
readelf -s /lib/x86_64-linux-gnu/libc.so.6 | grep printf
```

---

## Permissions and Privilege

Every process runs as a user. The kernel enforces access control via UIDs and GIDs.

```bash
id                      # show current uid, gid, groups
whoami                  # current username
cat /etc/passwd         # user accounts
cat /etc/shadow         # password hashes (root only)

ls -la /path            # show file permissions
chmod 755 file          # rwxr-xr-x
chmod u+s binary        # set SUID bit
```

### SUID (Set User ID)

If a binary has the SUID bit set, it runs with the file owner's privileges (often root), regardless of who executes it. This is a major escalation target.

```bash
find / -perm -4000 2>/dev/null    # find SUID binaries
```

---

## Useful Commands for Security Work

```bash
# Examine a binary before running it
file ./binary
strings ./binary          # extract readable strings
hexdump -C ./binary | head -20
checksec --file=./binary  # show security mitigations

# Runtime inspection
strace ./binary           # syscall trace
ltrace ./binary           # library call trace
gdb ./binary              # debugger

# Memory and process inspection
cat /proc/<pid>/maps      # memory map
cat /proc/<pid>/cmdline   # command line arguments
cat /proc/<pid>/environ   # environment variables
```

---

## Exercises

1. Run `cat /proc/self/maps` and identify which regions are the stack, heap, binary, and libc.
2. Run `strace /bin/ls` and count how many `read` and `write` syscalls it makes.
3. Find all SUID binaries on your system. Look up each one — is any of them known to be exploitable?
4. Compile a simple C program and run `readelf -S` on it. Find the `.text`, `.data`, `.bss`, `.plt`, and `.got` sections.
5. Use `ldd` to find the path to `libc`. Use `nm -D` to find the offset of `system` inside it.

---

## References

| Topic | Source |
|---|---|
| Processes, virtual memory, `/proc` filesystem | *The Linux Programming Interface* — Michael Kerrisk, Ch. 6 (Processes) and Ch. 49 (Memory Mappings) |
| File descriptors, `open`/`read`/`write` syscalls | *The Linux Programming Interface* — Michael Kerrisk, Ch. 4–5 (File I/O) |
| Signals (`SIGSEGV`, `SIGKILL`, etc.) | *The Linux Programming Interface* — Michael Kerrisk, Ch. 20–22 (Signals) |
| ELF format (header, sections, segments) | [ELF-64 Object File Format spec](https://uclibc.org/docs/elf-64-gen.pdf) |
| PLT/GOT and dynamic linking | *Hacking: The Art of Exploitation* — Jon Erickson, Ch. 0x400 (Networking) |
| Linux syscalls (numbers, ABI) | [Linux man-pages: `man 2 syscall`](https://man7.org/linux/man-pages/man2/syscall.2.html) |
| SUID, UIDs, permissions model | *The Linux Programming Interface* — Michael Kerrisk, Ch. 9 (Process Credentials) |
| Kernel internals (scheduler, memory management) | *Linux Kernel Development* — Robert Love, 3rd ed. |
