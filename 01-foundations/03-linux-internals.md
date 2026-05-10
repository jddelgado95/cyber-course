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

When a binary calls `printf`, it doesn't know at compile time where `printf` lives in `libc`. The PLT/GOT mechanism resolves this at runtime.

```
call printf
    |
    v
printf@plt:           <- PLT stub
    jmp [printf@got]  <- GOT entry (address written by dynamic linker)
```

- First call: GOT points back into PLT, which calls the dynamic linker to resolve the address, then patches the GOT entry.
- Subsequent calls: GOT entry now holds the real `printf` address in libc.

This is important for exploits: overwriting a GOT entry redirects all future calls to that function.

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
