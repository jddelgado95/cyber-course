# Computer Architecture Fundamentals

Before learning assembly or how exploits work, you need to understand what a computer actually is and how it executes a program. This module explains the hardware foundations from the ground up.

---

## What Is a Computer?

A computer is a machine that executes a sequence of instructions stored in memory. Everything — programs, images, text, videos — is represented as numbers (bits) in memory.

At the most fundamental level, a computer has:

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

---

## Bits, Bytes, and Binary

### Bits

A **bit** is the smallest unit of information. It is either `0` or `1`. This maps directly to transistors in hardware: off = 0, on = 1.

### Bytes

A **byte** is 8 bits. One byte can represent 256 different values (2^8 = 256).

```
1 byte:   [ 0 | 1 | 1 | 0 | 0 | 1 | 0 | 1 ]
           bit7                         bit0
           (most significant)           (least significant)
```

### Binary Counting

```
Decimal:  0   1   2   3   4   5   6   7   8   9   10  15  16
Binary:   0   1  10  11 100 101 110 111 1000 1001 1010 1111 10000
Hex:      0   1   2   3   4   5   6   7   8   9    A    F   10
```

### Hexadecimal

Because binary is verbose, we use **hexadecimal** (base 16). One hex digit = 4 bits. Two hex digits = 1 byte.

```
Binary:  1010 1111
Hex:     A    F     = 0xAF
Decimal: 175
```

Prefixes: `0x` means hexadecimal. `0b` means binary.

```
0xFF = 255 decimal  = 11111111 binary
0x41 = 65  decimal  = 01000001 binary = ASCII 'A'
0x00 = 0   decimal  = null byte
```

### Common Sizes

| Name | Size | Example |
|---|---|---|
| bit | 1 bit | `0` or `1` |
| byte | 8 bits | `0xFF` |
| word | 16 bits (2 bytes) | `0xFFFF` |
| dword | 32 bits (4 bytes) | `0xDEADBEEF` |
| qword | 64 bits (8 bytes) | `0xDEADBEEFCAFEBABE` |

---

## How the CPU Works

The CPU runs a continuous loop called the **fetch-decode-execute cycle**:

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

### Instruction Pointer

The CPU always knows what to execute next via the **Instruction Pointer** (called `RIP` in x86-64):

```
Memory:
Address    Content (bytes)
0x401000   55          <- push rbp        <--- RIP points here
0x401001   48 89 e5    <- mov rbp, rsp
0x401004   ...
```

After executing `push rbp`, RIP automatically advances to `0x401001`.

---

## CPU Architecture: Von Neumann Model

Almost all modern computers follow the **Von Neumann architecture**: instructions and data share the same memory.

```
+-----------------------------------------------------+
|                      CPU                           |
|  +----------------+    +------------------------+  |
|  |   Control Unit |    |   Arithmetic Logic     |  |
|  |   (CU)         |    |   Unit (ALU)           |  |
|  |                |    |                        |  |
|  | - fetches      |    | - add, subtract,       |  |
|  |   instructions |    |   multiply, divide     |  |
|  | - manages      |    | - AND, OR, XOR, NOT    |  |
|  |   execution    |    | - compare values       |  |
|  |   flow         |    |                        |  |
|  +----------------+    +------------------------+  |
|                                                     |
|  +--------------------------------------------------+|
|  |              Registers                          ||
|  |  RAX RBX RCX RDX RSI RDI RSP RBP RIP RFLAGS   ||
|  +--------------------------------------------------+|
+-----------------------------------------------------+
            |                   |
            |  (memory bus)     |
            v                   v
+---------------------+   +------------------+
|    Main Memory      |   |    I/O devices   |
|    (RAM)            |   |  (keyboard, etc) |
|  stores code +data  |   +------------------+
+---------------------+
```

### Key Components

**Control Unit (CU):** Fetches instructions from memory and directs the rest of the CPU to execute them. Manages the instruction cycle.

**Arithmetic Logic Unit (ALU):** Performs all mathematical and logical operations. When you write `a + b` in C, the ALU does the actual addition.

**Registers:** Ultra-fast storage directly inside the CPU. Only ~16 general-purpose registers exist. The compiler maps variables to registers when possible.

**Cache:** Fast memory between the CPU and RAM. The CPU checks L1 cache first, then L2, then L3, then RAM.

---

## Instruction Set Architecture (ISA)

An ISA defines:
- What instructions the CPU understands
- What registers exist
- How memory is addressed

The ISA you will study is **x86-64** (also called AMD64 or Intel 64). It is the dominant ISA for desktop, laptop, and server computers.

```
ISA Families:
x86-64    -> Intel/AMD desktops, laptops, servers
ARM64     -> iPhones, Android, Apple Silicon Macs, Raspberry Pi
RISC-V    -> emerging open standard
MIPS      -> older embedded systems, routers
```

---

## Endianness

When a multi-byte value is stored in memory, which byte comes first?

**Little-endian** (x86-64 uses this): least significant byte first.

**Big-endian** (network byte order): most significant byte first.

```
Value: 0xDEADBEEF  (32-bit / 4 bytes)

Little-endian (x86):       Big-endian:
Address: [0] [1] [2] [3]   Address: [0] [1] [2] [3]
Bytes:   EF  BE  AD  DE    Bytes:   DE  AD  BE  EF
         ^least sig byte            ^most sig byte
```

This matters when reading raw memory in a debugger or constructing payloads.

```python
# Python struct — pack/unpack for exploit scripts
import struct
struct.pack("<I", 0xDEADBEEF)   # little-endian 32-bit: b'\xef\xbe\xad\xde'
struct.pack(">I", 0xDEADBEEF)   # big-endian 32-bit:    b'\xde\xad\xbe\xef'
```

---

## How a Program Runs: From Source to Execution

```
Source Code (C)
      |
      | gcc (compiler + assembler + linker)
      v
  ELF Binary (on disk)
      |
      | execve() syscall
      v
  OS Loader
      |
      | maps binary into virtual memory
      | sets up stack (argc, argv, envp)
      | loads shared libraries (libc)
      | jumps to _start (CRT startup code)
      v
  _start  (C runtime)
      |
      | calls main()
      v
  main()  (your code)
      |
      | returns exit code
      v
  _exit()  -> exit syscall -> OS cleans up process
```

### What the OS Does

1. Creates a new process (new virtual address space)
2. Loads the ELF binary's segments into virtual memory
3. Sets up the initial stack with arguments and environment
4. Loads required shared libraries (e.g., libc)
5. Transfers execution to the program's entry point

---

## The Stack and Heap at the Hardware Level

### Stack

The stack is a region of memory where the CPU automatically tracks function calls and local variables using the `RSP` (stack pointer) register.

```
CPU Instructions <-> Stack Operations:

PUSH rax:            RSP = RSP - 8
                     Memory[RSP] = rax

POP rax:             rax = Memory[RSP]
                     RSP = RSP + 8

CALL func:           RSP = RSP - 8
                     Memory[RSP] = RIP + instruction_length  (return addr)
                     RIP = func

RET:                 RIP = Memory[RSP]
                     RSP = RSP + 8
```

The stack is managed automatically by these hardware instructions. This is why overwriting the return address (which lives on the stack) redirects execution — `RET` just loads `RIP` from wherever `RSP` points.

### Heap

The heap has no special hardware support. It is just a region of memory managed by software (the allocator like `malloc`/`free`).

---

## Interrupts and Exceptions

The CPU can be interrupted at any time to handle external events:

```
Hardware Interrupt:  Timer fires, keyboard pressed, network packet arrives
                     -> CPU pauses, saves state, runs interrupt handler

Software Interrupt:  Program calls "syscall" or "int 0x80"
                     -> CPU switches to kernel mode, runs kernel code

Exception:           CPU detects an error: divide by zero, invalid address
                     -> CPU raises exception (SIGSEGV, SIGFPE, etc.)
```

When a segfault happens, the CPU generated a **page fault exception** because the program accessed an unmapped memory address. The kernel's exception handler sent SIGSEGV to the process.

---

## Registers: Quick Reference

```
General Purpose (64-bit):
  RAX  - Accumulator, holds return values
  RBX  - Base register (callee-saved)
  RCX  - Counter (used in loops, 4th argument)
  RDX  - Data (used in multiply/divide, 3rd argument)
  RSI  - Source index (2nd argument)
  RDI  - Destination index (1st argument)
  R8   - 5th argument
  R9   - 6th argument
  R10, R11 - Temporary (caller-saved)
  R12, R13, R14, R15 - Callee-saved

Special Purpose:
  RSP  - Stack Pointer (top of stack)
  RBP  - Base Pointer (bottom of current stack frame)
  RIP  - Instruction Pointer (next instruction to execute)

Flags Register:
  RFLAGS - ZF (zero), SF (sign), CF (carry), OF (overflow), etc.
```

---

## Summary: What Makes Exploitation Possible

The Von Neumann architecture stores code and data in the same memory. This has a fundamental consequence:

> If you can write to memory, you can overwrite code (or control flow data like return addresses) and change what the CPU executes.

```
Normal execution:
  buf (data) ... saved RBP (data) ... return addr (pointer to CODE)
                                      ^-- CPU executes whatever is here

After overflow:
  AAAA...AAAA ... AAAAAAAA ... 0xdeadbeef
                                ^-- CPU tries to execute at 0xdeadbeef
                                    (attacker controls this)
```

This is the root of nearly every software vulnerability you will study.

---

## Exercises

1. Convert these values manually, then verify with Python:
   - `0xFF` to decimal and binary
   - `255` to hex and binary
   - `0b11001010` to hex and decimal

2. Look at a hex dump (`hexdump -C /bin/ls | head -4`). The first 4 bytes are the ELF magic: `7f 45 4c 46`. What is `0x45`, `0x4c`, `0x46` in ASCII?

3. In Python: `struct.pack("<Q", 0x4141414141414141)`. What does this produce? What would you see in GDB if this overwrote a return address?

4. Run a simple program in GDB. Use `info registers` to see all registers. What is `RIP` pointing to? What is at that address (`x/5i $rip`)?

5. Open `/proc/self/maps` (run `cat /proc/self/maps` in your shell). The first column is the address range. How large is the stack region? The heap?

---

## References

| Topic | Source |
|---|---|
| Von Neumann architecture, CPU components (CU, ALU, registers) | *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy, Ch. 1 (Fundamentals of Quantitative Design) |
| Instruction sets, fetch-decode-execute cycle | *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy, Appendix B (Instruction Set Principles) |
| Memory hierarchy (registers → L1/L2/L3 → RAM) | *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy, Ch. 2 (Memory Hierarchy Design) |
| Endianness, binary/hex representation | *Hacking: The Art of Exploitation* — Jon Erickson, Ch. 0x100 (Introduction) |
| Stack and heap at the hardware level (PUSH/POP/CALL/RET) | *Hacking: The Art of Exploitation* — Jon Erickson, Ch. 0x200 (Programming) |
| Interrupts, exceptions, page faults | *The Linux Programming Interface* — Michael Kerrisk, Ch. 23 (Timers and Sleeping) and Ch. 21 (Signals) |
| How a CPU works — visual explainers | Ben Eater (YouTube): *Building an 8-bit CPU from scratch*; Crash Course CS episodes 7–9 |
