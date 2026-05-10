# Static Analysis

Static analysis means examining a binary **without running it**. You read the code, understand the structure, and reason about behavior purely from the file on disk.

---

## Why Static Analysis?

- Safe — you never execute potentially malicious code
- Works on binaries you cannot run (wrong OS, wrong architecture, requires hardware)
- Reveals all code paths, including ones that are hard to trigger dynamically
- Essential when you need to understand an entire program before writing an exploit

---

## Workflow Overview

```
Binary File
    |
    v
1. Identify the file            (file, checksec)
    |
    v
2. Extract strings              (strings)
    |
    v
3. Examine sections/symbols     (readelf, nm)
    |
    v
4. Disassemble                  (objdump)
    |
    v
5. Decompile                    (Ghidra, IDA)
    |
    v
6. Understand logic, find bugs
```

---

## Step 1: Identify the Binary

Before anything else, know what you're dealing with.

```bash
file ./binary
# ELF 64-bit LSB executable, x86-64, dynamically linked, not stripped

checksec --file=./binary
# Arch, RELRO, Stack canary, NX, PIE
```

Key things to note:
- **64-bit or 32-bit** — affects register names and addresses
- **stripped or not stripped** — stripped = no function/variable names
- **statically or dynamically linked** — dynamically linked = uses libc externally
- **PIE enabled or not** — affects whether addresses are fixed

---

## Step 2: Extract Strings

Strings reveal a lot: file paths, error messages, format strings, hardcoded credentials, and flags.

```bash
strings ./binary              # all printable sequences >= 4 chars
strings -n 8 ./binary         # minimum length 8 (reduces noise)
strings -t x ./binary         # show offset of each string in hex
strings ./binary | grep -i "password\|flag\|secret\|key\|admin"
```

Example output:
```
/lib64/ld-linux-x86-64.so.2
Enter password:
Access granted!
/bin/sh
CTF{...flag...}
```

Any of these is a clue about the program's behavior.

---

## Step 3: Examine the ELF Structure

```bash
# Section headers (where is .text, .data, .plt, .got?)
readelf -S ./binary

# Program headers (how segments are loaded into memory)
readelf -l ./binary

# Symbol table (function names, if not stripped)
readelf --syms ./binary
nm ./binary

# Dynamic symbols (imported functions from libc, etc.)
readelf -d ./binary           # dynamic section
nm -D ./binary                # dynamic symbol table
```

### What to look for

```bash
readelf --syms ./binary | grep -E "FUNC|OBJECT"
# Shows function names and global variables

nm -D ./binary
# U = undefined (imported from library)
# T = defined in .text
# D = defined in .data
```

Common imported functions and what they hint at:
- `system`, `execve` → shell execution
- `strcmp`, `strncmp` → string comparison (password checks)
- `gets`, `strcpy` → potential buffer overflow
- `printf`, `fprintf` → potential format string
- `malloc`, `free` → heap usage

---

## Step 4: Disassemble with objdump

`objdump` converts machine code bytes into human-readable assembly.

```bash
# Disassemble all code (Intel syntax is more readable)
objdump -d -M intel ./binary

# Disassemble and show source interleaved (if debug symbols present)
objdump -d -M intel -S ./binary

# Disassemble a specific function
objdump -d -M intel ./binary | grep -A 50 "<main>:"

# Show raw hex bytes alongside assembly
objdump -d -M intel ./binary | head -40
```

### Reading objdump Output

```
0000000000401156 <main>:
  401156:       55                      push   rbp
  401157:       48 89 e5                mov    rbp,rsp
  40115a:       48 83 ec 40             sub    rsp,0x40
  40115e:       bf 10 20 40 00          mov    edi,0x402010
  401163:       e8 c8 fe ff ff          call   401030 <puts@plt>
```

```
[address] : [hex bytes]    [assembly instruction]

401156       55             push rbp
  ^addr      ^machine code  ^human-readable
```

---

## Step 5: Decompile with Ghidra

Ghidra (free, NSA) converts assembly back into C-like pseudocode. It is the primary tool for static analysis.

### Installation

```bash
# Download from https://ghidra-sre.org
# Requires Java 17+
sudo apt install openjdk-17-jdk
# Extract archive, run ./ghidraRun
```

### Basic Workflow

1. Create a new project: File → New Project
2. Import binary: File → Import File → select your binary
3. Auto-analyze: Yes (takes 30s–2min depending on binary size)
4. Open the **Symbol Tree** (left panel) → Functions → find `main`
5. The **Decompiler** window (right) shows C pseudocode
6. The **Listing** window (center) shows the disassembly

### Key Windows

```
+------------------+---------------------------+------------------+
| Symbol Tree      |    Listing (Assembly)     |   Decompiler     |
|                  |                           |   (Pseudocode)   |
| Functions:       | 00401156 PUSH RBP         | void main() {    |
|   main           | 00401157 MOV RBP,RSP      |   char buf[64];  |
|   foo            | 0040115a SUB RSP,0x40     |   gets(buf);     |
|   check_pass     | 0040115e MOV EDI,...      |   puts("...");   |
|                  | 00401163 CALL puts        | }                |
+------------------+---------------------------+------------------+
```

### Ghidra Tips

```
Double-click a function name  -> navigate to it
L                             -> rename label/variable
;                             -> add comment
Ctrl+L                        -> go to address
Right-click -> References      -> find all callers of a function
G                             -> go to specific address
Ctrl+F                        -> search for string/bytes
```

---

## Step 6: Finding Vulnerabilities Statically

### Look for Dangerous Functions

```bash
objdump -d -M intel ./binary | grep -E "call.*gets|call.*strcpy|call.*printf"
# or in Ghidra: Search -> For Direct References -> gets
```

### Trace Data Flow

When you find a dangerous call like `gets(buf)`:
1. Find `buf` in the decompiler — what size was it allocated? (`char buf[64]` → 64 bytes)
2. Find the return address offset — how far past `buf` is the saved RIP?
3. Is there a canary protecting it?

### Recognize Password Check Patterns

```c
// Ghidra decompiler output — classic strcmp check
if (strcmp(user_input, "s3cr3t") == 0) {
    puts("Access granted");
    system("/bin/sh");
}
```

This pattern is immediately obvious in pseudocode. In disassembly:

```nasm
lea  rdi, [user_input]
lea  rsi, [hardcoded_string]   ; address in .rodata
call strcmp
test eax, eax                   ; strcmp returns 0 if equal
jne  .access_denied             ; jump if NOT equal
; fall through to "access granted"
```

### Recognize Format String Patterns

```nasm
; Dangerous: user controls the format string
mov  rdi, [user_input]   ; first arg to printf = user data
call printf               ; user_input IS the format string

; Safe:
lea  rdi, [fmt_str]      ; "%s" literal
mov  rsi, [user_input]   ; user data is the argument, not format
call printf
```

---

## Practical Example: Crackme

A typical CTF reverse engineering challenge:

```c
// Decompiler output:
int main() {
    char buf[32];
    printf("Enter key: ");
    fgets(buf, 32, stdin);
    buf[strcspn(buf, "\n")] = 0;   // remove newline

    if (check_key(buf) == 1) {
        puts("Correct!");
    } else {
        puts("Wrong.");
    }
}

int check_key(char *input) {
    // XOR each byte with 0x13 and compare to hardcoded values
    char expected[] = {0x76, 0x60, 0x72, 0x60, 0x7b};
    for (int i = 0; i < 5; i++) {
        if ((input[i] ^ 0x13) != expected[i]) return 0;
    }
    return 1;
}
```

From this you can reverse the key:
```python
expected = [0x76, 0x60, 0x72, 0x60, 0x7b]
key = ''.join(chr(b ^ 0x13) for b in expected)
print(key)   # "esaw h" or whatever the XOR produces
```

---

## Exercises

1. Run `strings` on `/bin/ls`. What interesting strings do you find? What do they reveal about the binary?
2. Compile a C program with a password check using `strcmp`. Load it in Ghidra. Can you find the hardcoded password in the decompiler view?
3. Disassemble `/bin/ls` with `objdump`. Find the `main` function. How many calls does it make in the first 20 instructions?
4. Use `readelf --syms` on a binary compiled with debug symbols (`gcc -g`). Then strip it (`strip binary`). Run `readelf --syms` again. What changed?
5. In Ghidra, import a binary and use "Search → For Strings" to find all strings. Navigate to each one's cross-references to find where it's used.

---

## References

| Topic | Source |
|---|---|
| ELF format, section/segment headers (`readelf`, `objdump`) | [ELF-64 Object File Format spec](https://uclibc.org/docs/elf-64-gen.pdf) |
| Static analysis methodology, finding vulnerabilities without running code | *Hacking: The Art of Exploitation* — Jon Erickson, Ch. 0x200 (Programming) |
| Ghidra usage and reverse engineering workflow | [Ghidra official documentation](https://ghidra-sre.org) |
| Reading disassembly, identifying patterns (loops, conditionals, calls) | *Practical Reverse Engineering* — Bruce Dang et al., Ch. 1 (x86 and x86-64) |
| Recognizing vulnerability patterns in disassembly | *The Art of Software Security Assessment* — Dowd, McDonald & Schuh, Ch. 6 (C Language Issues) |
| Dynamic symbols, PLT/GOT, stripping | *The Linux Programming Interface* — Michael Kerrisk, Ch. 41–42 (Shared Libraries) |
