# Memory Layout

A precise understanding of how memory is organized — both at the CPU and OS level — is the foundation of every exploit technique.

---

## The Memory Hierarchy

Memory in a computer is organized in layers, trading speed for size:

```
Fastest, smallest
+------------------+
|   CPU Registers  |  ~16 x 8 bytes  |  < 1 ns  |  inside the CPU
+------------------+
|   L1 Cache       |  ~32 KB         |  ~1 ns   |  on the CPU die
+------------------+
|   L2 Cache       |  ~256 KB        |  ~4 ns   |  on the CPU die
+------------------+
|   L3 Cache       |  ~8 MB          |  ~10 ns  |  shared across cores
+------------------+
|   RAM (DRAM)     |  GBs            |  ~100 ns |  main memory
+------------------+
|   Disk/SSD       |  TBs            |  ms/µs   |  persistent storage
+------------------+
Slowest, largest
```

When exploiting programs, you are operating primarily in RAM, but CPU registers are your immediate workspace.

---

## Virtual vs Physical Memory

Modern operating systems give every process the illusion of having all of memory to itself. This is **virtual memory**.

```
Process A                   Physical RAM
Virtual Space               
+-------------+             +-------------+
| 0x0000      |  ---\       | page frame  |
|    ...      |      \----> | 0x3A000     |
| 0xFFFF      |             |             |
+-------------+             | page frame  |
                     /----> | 0x71000     |
Process B           /       |             |
Virtual Space      /        +-------------+
+-------------+   /
| 0x0000      | -/
|    ...      |
| 0xFFFF      |
+-------------+
```

- Two processes can have the same virtual address, but they map to different physical memory.
- The CPU's **Memory Management Unit (MMU)** translates virtual → physical using **page tables** maintained by the kernel.
- **Pages** are the unit of virtual memory — 4 KB on most systems.

---

## A Process's Virtual Address Space

When a binary is executed, the OS lays out memory like this:

```
0xFFFFFFFFFFFFFFFF
+==================================+
|          KERNEL SPACE            |  not accessible from userspace
+==================================+  0xFFFF800000000000

        (large unmapped gap)

+----------------------------------+  randomized if ASLR enabled
|              STACK               |
|  grows downward (toward 0)       |
|                                  |
|  contains:                       |
|   - local variables              |
|   - function arguments           |
|   - return addresses             |
|   - saved registers              |
|   - stack canaries               |
|                                  |
|              v v v               |
+----------------------------------+

+----------------------------------+  randomized if ASLR enabled
|           LIBRARIES              |
|  (libc.so, libpthread.so, etc.)  |
|  mmap'd into address space       |
+----------------------------------+

+----------------------------------+  ^ ^ ^
|              HEAP                |
|  grows upward (toward 0xFFFF...) |
|                                  |
|  contains:                       |
|   - malloc'd memory              |
|   - freed chunks (free list)     |
|   - heap metadata                |
+----------------------------------+

+----------------------------------+
|     BSS  (.bss)                  |  uninitialized global/static vars
|     DATA (.data)                 |  initialized global/static vars
|     TEXT (.text)                 |  executable code (read-only)
|     RODATA (.rodata)             |  string literals, constants
+----------------------------------+  0x0000000000400000 (no PIE)
                                       randomized if PIE enabled

0x0000000000000000  (NULL — unmapped, causes SIGSEGV on access)
```

---

## The Stack in Detail

The stack is LIFO (Last In, First Out). It grows toward lower addresses on x86.

### Stack Operations

```
PUSH rax:                       POP rax:
  RSP = RSP - 8                   rax = [RSP]
  [RSP] = rax                     RSP = RSP + 8
```

```
Before PUSH:
+---------------+  <- RSP (e.g., 0x7fff00001000)
|               |

After PUSH:
+---------------+  <- 0x7fff00000ff8  (RSP moved DOWN by 8)
|    value      |
+---------------+  <- old RSP
|               |
```

### Stack Frame

Each function call creates a stack frame. Frames are chained via the saved RBP.

```
HIGH ADDRESSES
+================================+
|  ...caller's frame...          |
+================================+
|  argument 7+ (if any)          |  pushed before call
+--------------------------------+
|  return address                |  pushed by CALL instruction
|  (RIP of next instruction)     |
+--------------------------------+  <- RBP (of current frame)
|  saved RBP                     |  (previous frame's RBP)
+--------------------------------+
|  local variable: int x         |  [rbp - 0x4]
+--------------------------------+
|  local variable: int y         |  [rbp - 0x8]
+--------------------------------+
|  char buf[32]                  |  [rbp - 0x28] to [rbp - 0x9]
|  buf[0]  at [rbp - 0x28]       |
|  buf[31] at [rbp - 0x9]        |
+--------------------------------+  <- RSP (current stack top)
LOW ADDRESSES
```

### Why Overflow buf Reaches the Return Address

```
char buf[32] starts at [rbp - 0x28] = rbp - 40
Return address is at   [rbp + 0x8]  = rbp + 8

Distance = 40 (buf size) + 8 (saved RBP size) = 48 bytes

So: writing 48 bytes into buf[32] reaches the return address.
```

```
buf:  [0  1  2  3 ... 31]      <- 32 bytes
      [32 33 34 35 36 37 38 39] <- overwrite saved RBP (8 bytes)
      [40 41 42 43 44 45 46 47] <- overwrite return address (8 bytes)
```

---

## The Heap in Detail

The heap is managed by the allocator (glibc's `ptmalloc`). Memory is divided into **chunks**.

### Heap Chunk Layout

Every allocated block has metadata before the user data:

```
malloc(32) returns a pointer to "user data":

+--------------------+
|  prev_size (8 B)   |  size of previous chunk (if free)
+--------------------+
|  size flags (8 B)  |  size of THIS chunk | flags (P, M, A bits)
+====================+  <- pointer returned to user
|                    |
|   user data        |  32 bytes
|   (32 bytes)       |
|                    |
+--------------------+

When chunk is free, user data area is reused for:
+====================+
|  fd pointer (8 B)  |  forward pointer to next free chunk
+--------------------+
|  bk pointer (8 B)  |  backward pointer to previous free chunk
+--------------------+
|   (unused)         |
+--------------------+
```

### tcache (Thread Cache) — glibc 2.26+

Small freed chunks go into per-thread singly-linked lists (tcache bins) for fast reuse.

```
tcache bin for size 32:

head -> [chunk A] -> [chunk B] -> [chunk C] -> NULL
         fd ->         fd ->         fd ->
```

Corrupting a `fd` pointer lets you make `malloc()` return an arbitrary address — the foundation of tcache poisoning exploits.

---

## Segments in Detail

### .text (Code Segment)

- Contains compiled machine code
- Read-only and executable
- Shared between processes running the same binary

```bash
objdump -d ./binary          # disassemble .text
```

### .data (Initialized Data)

- Global and static variables that have an initial value

```c
int global = 42;             // lives in .data
static char *name = "alice"; // pointer lives in .data
```

### .bss (Block Started by Symbol)

- Global and static variables with no initial value
- Not stored in the file (just a size) — zeroed at startup

```c
int counter;                 // lives in .bss (initialized to 0)
char buffer[1024];           // 1KB in .bss
```

### .rodata (Read-Only Data)

- String literals and compile-time constants

```c
printf("Hello\n");           // "Hello\n" lives in .rodata
```

### .plt / .got (Dynamic Linking)

See `03-linux-internals.md` for full details. In short:
- `.plt` — stubs that jump through the GOT to call external functions
- `.got` — table of resolved external function addresses (writable)

Overwriting a `.got` entry is a common exploit technique.

---

## Address Space Layout Randomization (ASLR)

ASLR randomizes the base addresses of the stack, heap, and libraries each time a program runs.

```bash
# Check ASLR level
cat /proc/sys/kernel/randomize_va_space
# 0 = disabled
# 1 = randomize stack, VDSO, mmap
# 2 = also randomize heap (default on most systems)

# Disable for practice
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

# Observe ASLR in action
ldd /bin/ls   # run twice, notice different addresses
ldd /bin/ls
```

```
Without ASLR:                   With ASLR:
libc loaded at 0x7ffff7a00000   libc loaded at 0x7f3d92100000
libc loaded at 0x7ffff7a00000   libc loaded at 0x7f8a41c00000
(same every time)               (different every time)
```

---

## Position Independent Executable (PIE)

PIE randomizes the base address of the binary itself (not just libraries).

```bash
checksec --file=./binary
# PIE enabled: binary base is randomized
# PIE disabled: binary loaded at fixed address (e.g., 0x400000)
```

Without PIE, code addresses in the binary are always the same — easier to exploit. With PIE, you need to leak an address first to defeat it.

#### What this means in practice

Without PIE, every time you run the binary it is loaded at the same fixed address — for example `0x400000`. That means the address of every function, every ROP gadget, every string is completely predictable. An attacker can hardcode those addresses into an exploit and it will work every run.

With PIE enabled, the OS loads the binary at a **random base address** on every run. The code is compiled to use *relative* addresses — offsets from its current position — rather than hardcoded absolute ones. That is what "position independent" means: the binary works correctly regardless of where in memory it lands.

```
Without PIE (same every run):      With PIE (random every run):
  main() always at 0x401156          main() at 0x555555555156 (run 1)
  win()  always at 0x401136          win()  at 0x7f3a12340136 (run 2)
```

To exploit a PIE binary, an attacker must first **leak** a runtime address from the binary, subtract the known offset of that symbol to recover the random base, then calculate all other addresses from the base. This is why info leaks (format strings, out-of-bounds reads) are so valuable.

---

## Stack Canary

A random value placed between local variables and the saved return address. The function checks it before returning — if it changed, the program aborts.

```
+---------------------------+
|  return address           |
+---------------------------+
|  saved RBP                |
+---------------------------+
|  CANARY VALUE (random)    |  <- checked at function return
+---------------------------+
|  local variables / buf    |
+---------------------------+
```

```bash
checksec --file=./binary
# Canary found: has stack canary
# No canary found: no protection
```

#### How it works

The name comes from "canary in a coal mine" — miners would bring a canary underground; if the canary died, it warned of invisible danger. The stack canary serves the same purpose: it sits between the buffer and the return address, and if anything killed it (i.e., overwrote it), the program raises the alarm.

When a protected function is entered, the compiler inserts code to read a secret random value from a thread-local variable (`__stack_chk_guard`) and write it onto the stack just above the local variables. When the function is about to return, the compiler inserts code to read the canary back from the stack and compare it to `__stack_chk_guard`. If they don't match, the program immediately calls `__stack_chk_fail()` and aborts — before the corrupted return address is ever used.

```
Normal return:              After overflow:
  canary == guard  ✓          canary != guard  ✗
  → function returns          → __stack_chk_fail() → abort
```

The canary is generated once when the program starts and is different on every run, so an attacker cannot simply hardcode it.

**Limitation:** the canary only protects against overflows that *march straight through it*. If an attacker can read the canary value first — for example via a format string vulnerability — they can include the correct canary in their overflow payload and the check passes.

---

## Quick Reference: checksec Output

```bash
$ checksec --file=./binary
[*] './binary'
    Arch:     amd64-64-little
    RELRO:    Partial RELRO
    Stack:    No canary found
    NX:       NX enabled
    PIE:      No PIE (0x400000)
```

| Field | Meaning |
|---|---|
| Arch | CPU architecture |
| RELRO | Relocation Read-Only: Full = GOT is read-only after init |
| Stack | Whether stack canary is present |
| NX | No eXecute: stack/heap not executable |
| PIE | Whether binary base is randomized |

---

## Exercises

1. Write a C program and compile it. Run `cat /proc/<pid>/maps` while it's paused (use `sleep(60)` in the program). Identify every region.
2. Enable and disable ASLR. Run `ldd /bin/ls` three times each way. Record the addresses.
3. Compile a program with and without PIE (`-no-pie`). Compare the load addresses in GDB with `info proc mappings`.
4. Use `checksec` on five different binaries on your system (`/bin/ls`, `/bin/cat`, etc.). Note which have all mitigations enabled.
5. In GDB, set a breakpoint at `main` and print the canary value: `x/gx $rbp-0x8` (adjust offset if needed). Run the program twice — does the canary change?

---

## References

| Topic | Source |
|---|---|
| Memory hierarchy (registers, caches, RAM) | *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy, Ch. 2 (Memory Hierarchy Design) |
| Virtual memory, page tables, MMU | *The Linux Programming Interface* — Michael Kerrisk, Ch. 49 (Memory Mappings) |
| Process address space layout (`/proc/pid/maps`) | *The Linux Programming Interface* — Ch. 6 (Processes) |
| Stack frames, calling conventions | *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy, Appendix B (Instruction Set Principles) |
| Heap internals (ptmalloc, tcache) | [glibc malloc source](https://sourceware.org/git/?p=glibc.git;a=blob;f=malloc/malloc.c) |
| ASLR, stack canaries, NX, PIE | *Hacking: The Art of Exploitation* — Jon Erickson, Ch. 3 (Exploitation) |
