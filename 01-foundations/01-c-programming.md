# C Programming for Security Research

C is the language of the operating system, compilers, and virtually every exploitable binary you will encounter. Understanding it at a deep level — especially its memory model — is non-negotiable.

---

## Why C?

- The Linux kernel is written in C
- Most vulnerable binaries are compiled from C/C++
- C gives you direct control over memory (and direct ways to corrupt it)
- Understanding undefined behavior explains most vulnerability classes

---

## Memory Layout of a Process

When a program runs, the OS gives it a virtual address space divided into regions:

```
High addresses (0xFFFFFFFFFFFFFFFF)
+----------------------------------+
|         Kernel Space             |  <- inaccessible from userspace
+----------------------------------+
|           Stack                  |  <- grows DOWNWARD
|       (local variables,          |
|        return addresses,         |
|        saved registers)          |
|                |                 |
|                v                 |
|                                  |
|                ^                 |
|                |                 |
|           Heap                   |  <- grows UPWARD
|       (malloc/free memory)       |
+----------------------------------+
|     BSS Segment                  |  <- uninitialized globals (zeroed)
+----------------------------------+
|     Data Segment                 |  <- initialized globals & statics
+----------------------------------+
|     Text Segment                 |  <- executable code (read-only)
+----------------------------------+
Low addresses (0x0000000000000000)
```

### Stack Frame Layout

Each function call creates a "stack frame". This is where buffer overflows happen.

```
Higher addresses
+---------------------------+  <- previous frame
|   ...previous frame...   |
+---------------------------+
|   function argument N    |  (if more than 6 args on x86-64)
+---------------------------+
|   return address (RIP)   |  <- overwriting this = code execution
+---------------------------+
|   saved RBP              |  <- caller's base pointer
+---------------------------+
|   local variable 2       |
+---------------------------+
|   local variable 1       |
+---------------------------+
|   char buf[32]           |  <- if you overflow buf, you march upward
|   buf[0] ... buf[31]     |     and eventually overwrite RIP
+---------------------------+  <- RSP (stack pointer, grows down)
Lower addresses
```

### Buffer Overflow Visualization

```c
void vulnerable(char *input) {
    char buf[32];
    strcpy(buf, input);  // no bounds check!
}
```

```
Before overflow:
+---------------------------+
|   return address          |  0x00401234 (legitimate)
+---------------------------+
|   saved RBP               |  0x7fff...f050
+---------------------------+
|   buf[31]  ...  buf[0]    |  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
+---------------------------+

After overflow with 48 bytes of 'A':
+---------------------------+
|   return address          |  0x4141414141414141  <- CORRUPTED
+---------------------------+
|   saved RBP               |  0x4141414141414141  <- CORRUPTED
+---------------------------+
|   buf[31]  ...  buf[0]    |  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
+---------------------------+
```

#### Understanding the layout

When `vulnerable()` is called, the CPU builds a workspace on the stack for that function. The stack grows **downward** in memory, so lower addresses are at the bottom of the diagram. The workspace is laid out from bottom to top:

- **`buf[32]`** — a fixed-size box of 32 bytes. The function declared it, so the CPU reserved exactly 32 bytes for it.
- **saved RBP** (8 bytes) — before entering this function, the CPU saved the previous frame's base pointer here. It is bookkeeping so the caller can restore itself when this function returns.
- **return address** (8 bytes) — the address of the instruction in the caller that should run *after* `vulnerable()` finishes. The CPU reads this when it hits `RET`.

Total distance from the start of `buf` to the end of the return address: **32 + 8 + 8 = 48 bytes**.

#### What `strcpy` does

`strcpy(buf, input)` copies every byte from `input` into `buf`, starting at `buf[0]`, until it hits a null byte (`\0`). It does **not** check whether `input` is longer than `buf`. It just writes. If `input` is 48 bytes, it writes 48 bytes — straight through `buf`, through saved RBP, and into the return address.

#### The overflow, byte by byte

The attacker sends 48 `'A'` characters. In ASCII, `'A'` = `0x41`.

```
Bytes  0–31  → fill buf exactly          → buf is now "AAAA...AAAA"
Bytes 32–39  → spill into saved RBP      → saved RBP  is now 0x4141414141414141
Bytes 40–47  → spill into return address → return address is now 0x4141414141414141
```

#### What happens when the function returns

The function hits `RET`. That instruction says: *read whatever is at the top of the stack and jump there.* It reads `0x4141414141414141` as the next address to execute. The CPU tries to fetch an instruction from that address — which is not mapped in memory — and crashes with a **Segmentation Fault**.

But if instead of `'A'` bytes the attacker puts a **real address** in those last 8 bytes — say, the address of a `win()` function or a shell — the CPU jumps there instead. That is code execution.

> **One-line summary:** `buf` has 32 bytes of space. `strcpy` does not know that. Writing 48 bytes marches right through saved RBP and overwrites the return address. When the function returns, the CPU blindly jumps to whatever is written there.

---

## Unsafe Functions — Full Explanation

### `gets(buf)` — Never Use This

**What it does:** Reads a line from stdin and stores it in `buf`. Stops reading at newline or EOF.

**The problem:** It has no way to know how large `buf` is. It will write as many bytes as the user types, regardless of buffer size.

```c
char buf[32];
gets(buf);   // user types 100 chars -> 68 bytes overflow into the stack
```

```
Input: "AAAA...AAAA" (100 bytes)

Stack before:
[ buf: 32 bytes ][ saved RBP ][ return addr ]

Stack after gets():
[ AAAA...AAAA ][ AAAA...AA ][ AAAA...AAAA ]
                 ^overflow     ^return addr overwritten
```

**Safe alternative:** `fgets(buf, sizeof(buf), stdin)`
- Takes the buffer size as a second argument
- Reads at most `size - 1` bytes, always null-terminates

```c
char buf[32];
fgets(buf, sizeof(buf), stdin);  // safe: max 31 chars + '\0'
```

---

### `strcpy(dst, src)` — Blind Copy

**What it does:** Copies the string at `src` (including the null terminator `\0`) into `dst`.

**The problem:** No length check. If `src` is longer than `dst`, it overflows into adjacent memory.

```c
char dst[8];
char *src = "Hello, this is way too long!";
strcpy(dst, src);   // writes 29 bytes into an 8-byte buffer
```

```
Memory layout (dst on stack):
Address:   [0x10][0x11][0x12][0x13][0x14][0x15][0x16][0x17][0x18]...
dst holds: [ H  ][ e  ][ l  ][ l  ][ o  ][ ,  ][ ' '][ t  ][ h  ]...
                                                         ^past dst^
```

**Safe alternative:** `strncpy(dst, src, n)`
- Takes a maximum number of bytes to copy
- Caution: does NOT guarantee null-termination if `src` is longer than `n`

```c
char dst[8];
strncpy(dst, src, sizeof(dst) - 1);
dst[sizeof(dst) - 1] = '\0';   // always manually null-terminate
```

---

### `strcat(dst, src)` — Blind Append

**What it does:** Appends the string `src` to the end of `dst`. It finds the null terminator in `dst`, then copies `src` starting there.

**The problem:** Assumes `dst` has enough space for both strings. No length check.

```c
char buf[16] = "Hello";   // 5 chars used, 11 remaining
strcat(buf, ", world!!"); // 9 chars to append -> fits
strcat(buf, " overflow"); // 9 more chars -> overflows the 16-byte buffer
```

```
After first strcat (ok):
[ H ][ e ][ l ][ l ][ o ][ , ][ ' '][ w ][ o ][ r ][ l ][ d ][ ! ][ ! ][\0][ ? ]

After second strcat (overflow):
[ H ][ e ][ l ][ l ][ o ][ , ][ ' '][ w ][ o ][ r ][ l ][ d ][ ! ][ ! ][ ' '][ o ][ v ][ e ][ r ]...
                                                                               ^past buffer^
```

**Safe alternative:** `strncat(dst, src, n)`
- `n` = remaining space in `dst`, not the total size

```c
char buf[16] = "Hello";
strncat(buf, ", world", sizeof(buf) - strlen(buf) - 1);
```

---

### `sprintf(buf, fmt, ...)` — Formatted Write Without Limits

**What it does:** Works like `printf` but writes the output into `buf` (a string in memory) instead of stdout.

**The problem:** Does not take a buffer size. If the formatted output is larger than `buf`, it overflows.

```c
char buf[16];
int big_number = 1234567890;
sprintf(buf, "Number: %d", big_number);   // "Number: 1234567890" = 19 chars + \0 = overflow
```

```
buf[16]:  [ N ][ u ][ m ][ b ][ e ][ r ][ : ][ ' '][ 1 ][ 2 ][ 3 ][ 4 ][ 5 ][ 6 ][ 7 ][ 8 ]
overflow: [ 9 ][ 0 ][\0 ] <- written past the end of buf
```

**Safe alternative:** `snprintf(buf, size, fmt, ...)`
- Always specify the buffer size
- Truncates output at `size - 1` bytes and null-terminates

```c
char buf[16];
snprintf(buf, sizeof(buf), "Number: %d", big_number);  // truncates safely
```

---

### `scanf("%s", buf)` — Whitespace-Delimited, No Limit

**What it does:** Reads a whitespace-delimited token from stdin and stores it in `buf`.

**The problem:** `%s` with no width modifier reads until whitespace — no size limit.

```c
char buf[16];
scanf("%s", buf);   // user types 100 chars -> 84 bytes overflow
```

```
Input: "AAAAAAAAAAAAAAAAAAAAAAAA" (24 chars)
buf[16]: [ A ][ A ][ A ][ A ][ A ][ A ][ A ][ A ][ A ][ A ][ A ][ A ][ A ][ A ][ A ][ A ]
overflow:[ A ][ A ][ A ][ A ][ A ][ A ][ A ][ A ][\0 ] <- past buf
```

**Safe alternative:** Width-limited `%s`
- The width in the format string is the max characters to read (excluding `\0`)

```c
char buf[16];
scanf("%15s", buf);   // reads at most 15 chars, null-terminates
```

---

### `printf(user_input)` — Format String Vulnerability

**What it does:** Prints formatted output to stdout. Format specifiers like `%d`, `%s`, `%x` consume arguments from the stack.

**The problem:** If `user_input` contains format specifiers and you pass it directly as the format string, `printf` will read (or write) stack memory.

```c
char user_input[64];
fgets(user_input, sizeof(user_input), stdin);
printf(user_input);   // DANGEROUS if input contains %x, %s, %n
```

```
If user types: "%x %x %x %x"

printf has no extra args, so it reads from the stack directly:
Output: "f7a3b200 0 fbad2088 25207825"
         ^-- leaked stack values --^
```

Even worse, `%n` writes the number of bytes printed so far to an address on the stack — giving an attacker arbitrary write.

```
Input: "%100d%n"  ->  writes the value 100 to a stack address
```

**Safe alternative:** Always use a literal format string

```c
printf("%s", user_input);   // user_input is treated as data, not a format string
```

---

## Compilation and Flags

```bash
# Basic compilation
gcc -o program program.c

# With debug symbols (important for learning)
gcc -g -o program program.c

# Disable security mitigations (for practice only)
gcc -fno-stack-protector \   # disable stack canary
    -no-pie \                # disable Position Independent Executable
    -z execstack \           # make stack executable
    -o vuln vuln.c

# Full example for a practice binary
gcc -g -fno-stack-protector -no-pie -z execstack -o vuln vuln.c
```

---

## Reading Compiler Output

```c
// C source
int add(int a, int b) {
    return a + b;
}
```

```bash
gcc -O0 -g -o add add.c
objdump -d -M intel add | grep -A 15 "<add>:"
```

```nasm
; Expected output (x86-64)
add:
    push   rbp
    mov    rbp, rsp
    mov    DWORD PTR [rbp-0x4], edi   ; store argument a
    mov    DWORD PTR [rbp-0x8], esi   ; store argument b
    mov    edx, DWORD PTR [rbp-0x4]
    mov    eax, DWORD PTR [rbp-0x8]
    add    eax, edx                   ; a + b -> eax (return value)
    pop    rbp
    ret
```

---

## Exercises

1. Write a C program that prints the address and value of a local variable, a heap variable, and a global variable. Observe the address ranges — can you identify which segment each lives in?
2. Write a function with `char buf[32]`. Use a loop to print each byte's address. Confirm the stack grows downward.
3. Compile a program with `gets()`. Use `checksec` and `gdb` to find the exact byte offset from the start of the buffer to the return address.
4. Write a program that demonstrates the format string bug: take user input and pass it directly to `printf`. Try inputs like `%x`, `%x.%x.%x.%x`, and `%s`.
5. Read: `man 3 gets`, `man 3 strcpy`, `man 3 printf`, `man 3 malloc`.

---

## References

| Topic | Source |
|---|---|
| C memory model, pointers, stack vs heap | *Hacking: The Art of Exploitation* — Jon Erickson, Ch. 0x200 (Programming) |
| `gets`, `strcpy`, `sprintf` and unsafe patterns | *Hacking: The Art of Exploitation* — Jon Erickson, Ch. 0x300 (Exploitation) |
| Stack frame layout, buffer overflow mechanics | *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy, Appendix B (Instruction Set Principles) |
| Process memory layout (.text, .data, .bss, heap, stack) | *The Linux Programming Interface* — Michael Kerrisk, Ch. 6 (Processes) |
| Format string vulnerabilities (`%n`, arbitrary write) | *Hacking: The Art of Exploitation* — Jon Erickson, Ch. 0x350 (Format Strings) |
| `malloc`/`free` internals | *The Linux Programming Interface* — Michael Kerrisk, Ch. 7 (Memory Allocation) |
| C language reference | *The C Programming Language* — Brian W. Kernighan & Dennis M. Ritchie (K&R), 2nd ed. |
