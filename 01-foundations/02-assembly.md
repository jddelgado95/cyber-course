# x86-64 Assembly for Security Research

Assembly is the lowest level you can read code. Every binary you reverse engineer will be presented to you as assembly. You don't need to write complex programs in it — but you must be able to read it fluently.

---

## Why Assembly?

- Decompilers (Ghidra, IDA) produce C-like pseudocode from assembly — but they lie. Assembly is ground truth.
- Exploit development requires understanding exactly what instructions execute and when.
- Shellcode is written in assembly.
- Mitigations like ASLR, NX, and stack canaries are visible at the assembly level.

---

## Registers (x86-64)

Registers are tiny storage locations inside the CPU — the fastest memory available.

```
64-bit   32-bit   16-bit   8-bit high  8-bit low
RAX      EAX      AX       AH          AL
RBX      EBX      BX       BH          BL
RCX      ECX      CX       CH          CL
RDX      EDX      DX       DH          DL
RSI      ESI      SI                   SIL
RDI      EDI      DI                   DIL
RSP      ESP      SP                   SPL   <- stack pointer
RBP      EBP      BP                   BPL   <- base pointer
RIP      EIP      IP                         <- instruction pointer
R8 - R15 (64-bit only general purpose)
```

### Special Purpose Registers

| Register | Role |
|---|---|
| **RSP** | Stack pointer — always points to the top (lowest address) of the stack |
| **RBP** | Base pointer — anchor for the current stack frame |
| **RIP** | Instruction pointer — address of the next instruction to execute |
| **RAX** | Return value from a function |
| **RDI, RSI, RDX, RCX, R8, R9** | Function arguments (in order) |

### Flags Register (RFLAGS)

The CPU sets these bits after arithmetic/logic instructions:

| Flag | Name | Set when... |
|---|---|---|
| ZF | Zero Flag | Result was zero |
| SF | Sign Flag | Result was negative |
| CF | Carry Flag | Unsigned overflow |
| OF | Overflow Flag | Signed overflow |
| PF | Parity Flag | Even number of set bits |

Conditional jumps (`je`, `jne`, `jg`, etc.) check these flags.

---

## Memory Model in Assembly

```
Instruction                    Meaning
-----------------------------------------------
mov rax, 5                    rax = 5            (immediate)
mov rax, rbx                  rax = rbx           (register)
mov rax, [rbx]                rax = *rbx          (memory read)
mov [rbx], rax                *rbx = rax          (memory write)
mov rax, [rbp - 8]            rax = *(rbp - 8)    (local variable)
mov rax, [rip + 0x200a]       rax = *(rip + offset) (RIP-relative, PIE)
```

Square brackets `[ ]` mean "dereference" — go to that address and read/write memory there.

---

## Core Instructions

### Data Movement

```nasm
mov  dst, src      ; dst = src
lea  dst, [src]    ; dst = address of src (no dereference)
xchg dst, src      ; swap dst and src
push src           ; RSP -= 8; [RSP] = src
pop  dst           ; dst = [RSP]; RSP += 8
```

### Arithmetic

```nasm
add  dst, src      ; dst = dst + src
sub  dst, src      ; dst = dst - src
imul dst, src      ; dst = dst * src (signed)
idiv src           ; RDX:RAX / src -> quotient in RAX, remainder in RDX
inc  dst           ; dst = dst + 1
dec  dst           ; dst = dst - 1
neg  dst           ; dst = -dst
```

### Bitwise / Logic

```nasm
and  dst, src      ; dst = dst & src
or   dst, src      ; dst = dst | src
xor  dst, src      ; dst = dst ^ src  (xor dst, dst -> zero dst)
not  dst           ; dst = ~dst
shl  dst, n        ; dst = dst << n (logical left shift)
shr  dst, n        ; dst = dst >> n (logical right shift)
sar  dst, n        ; dst = dst >> n (arithmetic right shift, preserves sign)
```

### Comparison and Jumps

```nasm
cmp  a, b          ; computes a - b, sets flags, discards result
test a, b          ; computes a & b, sets flags, discards result

; Unconditional
jmp  label         ; RIP = label

; Signed comparisons
je   label         ; jump if equal           (ZF=1)
jne  label         ; jump if not equal       (ZF=0)
jg   label         ; jump if greater         (ZF=0 and SF=OF)
jge  label         ; jump if greater/equal   (SF=OF)
jl   label         ; jump if less            (SF!=OF)
jle  label         ; jump if less/equal      (ZF=1 or SF!=OF)

; Unsigned comparisons
ja   label         ; jump if above           (CF=0 and ZF=0)
jb   label         ; jump if below           (CF=1)
```

### Function Calls

```nasm
call label         ; push RIP (return address), then jmp label
ret                ; pop [RSP] into RIP (return to caller)
```

### System Calls

```nasm
syscall            ; transition to kernel; RAX = syscall number
```

---

## Calling Convention (System V AMD64 ABI)

This is the standard for Linux x86-64. You must know this to understand function calls.

```
Arguments (in order):   RDI, RSI, RDX, RCX, R8, R9
                        (7th+ arguments go on the stack)

Return value:           RAX (or RDX:RAX for 128-bit values)

Caller-saved registers: RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11
                        (the callee may clobber these)

Callee-saved registers: RBX, RBP, R12, R13, R14, R15
                        (the callee must preserve these)

Stack alignment:        RSP must be 16-byte aligned before a call instruction
```

### Function Call Example

```c
// C
int result = add(1, 2);
```

```nasm
; Assembly equivalent
mov  edi, 1        ; first argument -> RDI
mov  esi, 2        ; second argument -> RSI
call add           ; push return address, jump to add
; RAX now holds the return value
```

---

## Stack Frame Construction

Every function follows this prologue/epilogue pattern:

```nasm
; Prologue (function entry)
push rbp           ; save caller's base pointer
mov  rbp, rsp      ; establish our base pointer
sub  rsp, 0x20     ; allocate 32 bytes for local variables

; ... function body ...

; Epilogue (function exit)
leave              ; equivalent to: mov rsp, rbp; pop rbp
ret                ; return to caller
```

### Stack Frame Diagram

```
Before call:
+-------------------+  <- RSP (aligned to 16 bytes)

After call (return address pushed):
+-------------------+
| return address    |  <- RSP

After push rbp:
+-------------------+
| saved RBP         |  <- RSP
| return address    |

After mov rbp, rsp:
+-------------------+
| saved RBP         |  <- RSP = RBP
| return address    |

After sub rsp, 0x20:
+-------------------+
| local var (0x20)  |  <- RSP
| ...               |
| local var (0x08)  |  [rbp - 0x18]
| local var (0x04)  |  [rbp - 0x08]  <- e.g., int x at [rbp-0x8]
| saved RBP         |  <- RBP
| return address    |  [rbp + 0x08]
```

---

## Common Patterns in Disassembly

### If Statement

```c
if (x == 0) { foo(); }
```

```nasm
mov  eax, [rbp - 4]   ; load x
test eax, eax          ; x & x (sets ZF if x==0)
jne  .skip             ; if x != 0, skip
call foo
.skip:
```

### While Loop

```c
while (i < 10) { i++; }
```

```nasm
.loop_start:
mov  eax, [rbp - 4]   ; load i
cmp  eax, 10
jge  .loop_end         ; if i >= 10, exit loop
add  DWORD PTR [rbp - 4], 1
jmp  .loop_start
.loop_end:
```

#### What is `add DWORD PTR [rbp - 4], 1`?

This is the assembly for `i++`. Break it into parts:

- **`[rbp - 4]`** — a memory address: the slot on the stack 4 bytes below RBP, where the local variable `i` lives.
- **`DWORD PTR`** — "Double Word Pointer" — tells the CPU to treat that address as 4 bytes (the size of an `int`). DWORD = 32 bits = 4 bytes.
- **`add ... 1`** — add 1 directly to the value at that memory address.

This single instruction does three things: reads the integer at `[rbp-4]`, adds 1, and writes it back to the same location. x86 can operate directly on memory — you do not always need to load a value into a register first.

### Array Access

```c
int arr[5];
arr[i] = 99;
```

```nasm
mov  eax, [rbp - 0x4]         ; load i
cdqe                            ; sign-extend eax to rax
mov  DWORD PTR [rbp + rax*4 - 0x18], 99   ; arr[i] = 99
```

#### Why `rbp - 0x18`?

The compiler laid out the local variables on the stack frame like this:

```
[rbp - 0x04]  i        (4 bytes — a single int)
[rbp - 0x08]  arr[4]   (4 bytes)
[rbp - 0x0c]  arr[3]   (4 bytes)
[rbp - 0x10]  arr[2]   (4 bytes)
[rbp - 0x14]  arr[1]   (4 bytes)
[rbp - 0x18]  arr[0]   (4 bytes)  ← start of the array
```

`i` takes 4 bytes at the top. Then the 5 elements of `arr` occupy the next 20 bytes going down. The first element `arr[0]` lands at `rbp - 4 - 20 = rbp - 24 = rbp - 0x18`.

So `rbp - 0x18` is just the address of `arr[0]`.

#### Line-by-line explanation

**`mov eax, [rbp - 0x4]`**
Load `i` from the stack into EAX (32-bit).

**`cdqe`**
"Convert Doubleword to Quadword Extended." Sign-extends EAX (32-bit) into the full RAX (64-bit). This is necessary because memory addresses are 64 bits. If `i` is negative, this preserves the sign correctly — without it, `-1` in EAX would become `0x00000000FFFFFFFF` in RAX instead of `0xFFFFFFFFFFFFFFFF`, producing a completely wrong address.

**`mov DWORD PTR [rbp + rax*4 - 0x18], 99`**
Write 99 to `arr[i]`. The address is computed as:

```
rbp - 0x18        = address of arr[0]
rax * 4           = i × 4  (each int is 4 bytes, so element i is i×4 bytes past arr[0])
rbp + rax*4 - 0x18 = address of arr[i]
```

Verification:
```
i=0 → rbp + 0  - 0x18 = rbp - 0x18  = arr[0]  ✓
i=1 → rbp + 4  - 0x18 = rbp - 0x14  = arr[1]  ✓
i=4 → rbp + 16 - 0x18 = rbp - 0x08  = arr[4]  ✓
```

The CPU supports `base + index * scale + displacement` in a single memory operand — hardware built specifically for array indexing. The `scale` (here 4) matches the element size so you never manually multiply.

---

## Linux Syscalls

### What is a syscall?

Your program runs in **userspace** (Ring 3) — a restricted environment where it cannot directly touch hardware, access other processes' memory, or talk to the kernel. When your program needs to do something privileged — read a file, write to the terminal, allocate memory, spawn a process — it has to **ask the kernel** to do it on its behalf.

A **syscall** (system call) is the formal mechanism for making that request. It works like this:

```
Your program (Ring 3)
  │
  │  1. Load syscall number into RAX
  │  2. Load arguments into RDI, RSI, RDX, R10, R8, R9
  │  3. Execute the "syscall" instruction
  │
  ▼
CPU switches to Ring 0 (kernel mode)
  │
  │  4. Kernel reads RAX, identifies which service you want
  │  5. Kernel reads the arguments from registers
  │  6. Kernel performs the operation (reads file, writes bytes, etc.)
  │  7. Kernel places return value in RAX
  │
  ▼
CPU returns to Ring 3 (your program continues)
  │
  │  8. Your program reads RAX to see if it succeeded
```

The `syscall` instruction is the actual trigger — it atomically switches privilege level and jumps into the kernel's entry point. When the kernel is done, it uses `sysret` to hand control back.

### The calling convention for syscalls

Each syscall is identified by a number in RAX. Arguments go into specific registers in order:

```
RAX = syscall number      (which operation to perform)
RDI = argument 1
RSI = argument 2
RDX = argument 3
R10 = argument 4
R8  = argument 5
R9  = argument 6

Return value → RAX        (negative value = error code)
```

Note: syscall arguments use **R10** instead of RCX for the 4th argument (unlike the normal function calling convention which uses RCX). This is because the `syscall` instruction itself uses RCX internally.

### Common Syscalls

| RAX | Name | RDI | RSI | RDX | What it does |
|---|---|---|---|---|---|
| 0 | `read` | fd | buf | count | Read bytes from a file descriptor into buf |
| 1 | `write` | fd | buf | count | Write bytes from buf to a file descriptor |
| 2 | `open` | filename | flags | mode | Open a file, returns a file descriptor |
| 3 | `close` | fd | | | Close a file descriptor |
| 9 | `mmap` | addr | length | prot | Map memory (used by malloc internally) |
| 11 | `munmap` | addr | length | | Unmap memory |
| 59 | `execve` | filename | argv | envp | Replace current process with a new program |
| 60 | `exit` | status | | | Terminate the process |

**File descriptors (fd):** Every open file, socket, or pipe is represented as a small integer. Three are always open: `0` = stdin, `1` = stdout, `2` = stderr. When you `open()` a file, the kernel returns the next available fd (3, 4, 5...).

**`execve` — why it matters for exploitation:** `execve("/bin/sh", NULL, NULL)` replaces the current process with a shell. This is the goal of most shellcode — make the kernel run `/bin/sh` with your privileges.

### Syscalls vs library functions

You almost never call syscalls directly in C — you use library functions that wrap them:

```
printf("hello")      →  calls write(1, "hello", 5)        →  syscall 1
fopen("file", "r")   →  calls open("file", O_RDONLY, 0)   →  syscall 2
malloc(100)          →  calls mmap() or brk()             →  syscall 9 or 12
exit(0)              →  calls exit_group(0)               →  syscall 231
```

`strace` shows you the real syscalls underneath any C program.

### Write "Hello" to stdout

```nasm
section .data
    msg db "Hello, World!", 0x0a
    len equ $ - msg

section .text
    global _start
_start:
    mov rax, 1          ; syscall number 1 = write
    mov rdi, 1          ; fd 1 = stdout
    lea rsi, [rel msg]  ; RSI = pointer to the string
    mov rdx, len        ; RDX = number of bytes to write
    syscall             ; → kernel writes "Hello, World!\n" to terminal

    mov rax, 60         ; syscall number 60 = exit
    xor rdi, rdi        ; exit status 0 (xor reg,reg is a fast way to zero it)
    syscall             ; → kernel terminates the process
```

```bash
# Assemble and run
nasm -f elf64 -o hello.o hello.asm
ld -o hello hello.o
./hello
```

#### What happens step by step

1. `mov rax, 1` — tell the kernel we want `write`
2. `mov rdi, 1` — write to stdout (fd 1)
3. `lea rsi, [rel msg]` — point RSI at the start of the string in memory
4. `mov rdx, len` — tell the kernel how many bytes to write
5. `syscall` — CPU switches to Ring 0; kernel writes the bytes; returns number of bytes written in RAX
6. `mov rax, 60` — tell the kernel we want `exit`
7. `xor rdi, rdi` — set exit code to 0 (success)
8. `syscall` — process terminates

---

## Exercises

1. Write a NASM program that reads a number from stdin (syscall `read`), adds 1, and writes the result to stdout.
2. Open a binary in GDB. Set a breakpoint at `main`. Step through with `si` (step instruction). After each instruction, note what changed in the registers (`info registers`).
3. In Ghidra, find a function that contains a loop. Identify the `cmp`/`jmp` pair that controls it.
4. Identify the calling convention in action: set a breakpoint just before a `call` in GDB and confirm the arguments in `RDI`, `RSI`, `RDX`.

---

## References

| Topic | Source |
|---|---|
| x86-64 registers, instruction encoding, addressing modes | *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy, Appendix B (Instruction Set Principles) |
| x86-64 instruction reference (authoritative) | [Intel 64 and IA-32 Architectures Software Developer's Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html), Vol. 2 |
| System V AMD64 ABI (calling convention) | [System V Application Binary Interface — AMD64 Architecture Processor Supplement](https://gitlab.com/x86-psABIs/x86-64-ABI) |
| Linux syscall table (x86-64) | [syscall.sh](https://syscall.sh) or `/usr/include/asm/unistd_64.h` |
| Assembly for exploitation (shellcode, stack frames) | *Hacking: The Art of Exploitation* — Jon Erickson, Ch. 0x200 (Programming) |
| Reading disassembly, flags, common patterns | *Programming from the Ground Up* — Jonathan Bartlett (free PDF, x86 focused) |
