# Reverse Engineering

## What Is Reverse Engineering?

Reverse engineering is the process of understanding how something works by examining it — without access to the original design or source code.

In software security it means: **you have a compiled binary, you have no C source, and you need to figure out what it does.**

The compiler turned human-readable C into machine code. Reverse engineering goes the other direction:

```
Source code  ──(compiler)──▶  Binary
                              Binary  ──(you)──▶  Understanding
```

You are not perfectly recovering the source — variable names, comments, and structure are gone forever. What you recover is **behavior**: what the program computes, what it checks, what it sends, where the bugs are.

---

## Why Reverse Engineer?

| Situation | Goal |
|---|---|
| CTF crackme | Find the input that makes it print "Correct" |
| Malware analysis | Understand what a suspicious binary does without running it |
| Vulnerability research | Find bugs in closed-source software |
| Interoperability | Understand an undocumented protocol or file format |
| Exploit development | Know exactly which code path you are hijacking |

---

## The Two Approaches

Every reverse engineering session uses one or both of these:

**Static analysis** — read the binary without running it. Safe, complete, but slower. You see all code paths, including ones that never trigger at runtime.

**Dynamic analysis** — run the binary and observe it live. Faster for understanding behavior, but you only see what actually executes during that run.

They complement each other. A typical session starts static (understand structure, find interesting functions) then goes dynamic (step through those functions in GDB to confirm your theory).

---

## What You Are Looking At

When you open a binary in a disassembler, you see machine code — instructions the CPU executes directly. A decompiler (Ghidra, IDA) translates those instructions back into C-like pseudocode. Neither output is perfect, but both are readable with practice.

```
Binary bytes      Disassembly (objdump)     Decompiler (Ghidra)
─────────────     ──────────────────────    ───────────────────
55                push   rbp                int check_key(char *input) {
48 89 e5          mov    rbp, rsp
48 83 ec 10       sub    rsp, 0x10              char expected[] = {...};
...               ...                           for (int i = 0; i < 5; i++)
                                                    if ((input[i]^0x13) != expected[i])
                                                        return 0;
                                                return 1;
                                            }
```

Disassembly is ground truth — every instruction is real. Decompiler output is a best guess — useful, but always verify against the assembly when it matters.

---

## Mental Model

Think of a reverse engineer as a detective handed a finished jigsaw puzzle with no picture on the box. The pieces are the bytes. Static analysis is laying them all out and studying their shapes. Dynamic analysis is watching someone else assemble them in real time.

Your goal is not to rebuild the box art. Your goal is to answer one specific question: **where is the weakness?**

---

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

### Headless Mode (Terminal)

Ghidra ships with `analyzeHeadless`, a CLI tool that runs analysis and scripts without opening the GUI.
It lives in `$GHIDRA_HOME/support/analyzeHeadless`.

**Basic syntax:**

```bash
$GHIDRA_HOME/support/analyzeHeadless <project_dir> <project_name> \
    -import <binary> \
    [options]
```

**Import and analyze a binary (no script):**

```bash
# Creates ~/ghidra_projects/demo/, imports and auto-analyzes ./target
$GHIDRA_HOME/support/analyzeHeadless ~/ghidra_projects demo \
    -import ./target
```

**Dump decompiler output for every function:**

Ghidra's headless runner can execute Java or Python scripts after analysis.
Save this as `DecompileAll.py` anywhere on disk:

```python
# DecompileAll.py  — run with analyzeHeadless -postScript
from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor

decompiler = DecompInterface()
decompiler.openProgram(currentProgram)
monitor = ConsoleTaskMonitor()

for func in currentProgram.getFunctionManager().getFunctions(True):
    result = decompiler.decompileFunction(func, 30, monitor)
    if result and result.decompileCompleted():
        print("=== {} ===".format(func.getName()))
        print(result.getDecompiledFunction().getC())
```

Then run it:

```bash
$GHIDRA_HOME/support/analyzeHeadless ~/ghidra_projects demo \
    -process target \
    -postScript DecompileAll.py \
    2>/dev/null          # suppress Ghidra's own log noise
```

**List all function names and addresses:**

```bash
# Built-in script — prints every function entry point
$GHIDRA_HOME/support/analyzeHeadless ~/ghidra_projects demo \
    -process target \
    -postScript PrintFunctionNames.java \
    2>/dev/null
```

`PrintFunctionNames.java` ships with Ghidra in `$GHIDRA_HOME/Ghidra/Features/Base/ghidra_scripts/`.

**Find dangerous function calls from the terminal (no Ghidra needed):**

For quick hunting you rarely need Ghidra's headless mode — `objdump` + `grep` is faster:

```bash
objdump -d -M intel ./target | grep -E "call.*(gets|strcpy|sprintf|system)"
```

Use headless mode when you need the decompiler's C output or want to run cross-reference analysis that `objdump` cannot do.

**Useful headless flags:**

| Flag | Effect |
|---|---|
| `-import <file>` | Import and analyze a new binary |
| `-process <name>` | Re-process a binary already in the project |
| `-postScript <script>` | Run a script after analysis |
| `-scriptPath <dir>` | Directory Ghidra searches for scripts |
| `-noanalysis` | Skip auto-analysis (import only) |
| `-deleteProject` | Delete the project directory when done |
| `-log /dev/null` | Suppress the log file |

**Typical CTF workflow from the terminal:**

```bash
# 1. One-shot: analyze + dump decompiled main
$GHIDRA_HOME/support/analyzeHeadless /tmp ghidra_tmp \
    -import ./crackme \
    -postScript DecompileAll.py \
    -deleteProject 2>/dev/null | grep -A 40 "=== main ==="

# 2. Search for interesting strings already visible in the binary
strings ./crackme | grep -iE "flag|pass|key|secret|correct"

# 3. Cross-reference who calls a function (needs the decompiler script)
# Faster alternative: objdump + grep
objdump -d -M intel ./crackme | grep -B5 "call.*check_password"
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

A typical CTF reverse engineering challenge — you get a binary, no source code. You load it in Ghidra and the decompiler produces:

```c
int main() {
    char buf[32];
    printf("Enter key: ");
    fgets(buf, 32, stdin);
    buf[strcspn(buf, "\n")] = 0;   // strip the newline fgets leaves

    if (check_key(buf) == 1) {
        puts("Correct!");
    } else {
        puts("Wrong.");
    }
}

int check_key(char *input) {
    char expected[] = {0x76, 0x60, 0x72, 0x60, 0x7b};
    for (int i = 0; i < 5; i++) {
        if ((input[i] ^ 0x13) != expected[i]) return 0;
    }
    return 1;
}
```

### Step 1 — Understand what the program does

`main` reads a string from the user and passes it to `check_key`.
`check_key` returns `1` (correct) or `0` (wrong).
Your job: figure out which string makes `check_key` return `1`.

### Step 2 — Read check_key carefully

The loop condition is:

```c
if ((input[i] ^ 0x13) != expected[i]) return 0;
```

In plain English: *"XOR the i-th character of the input with 0x13. If it does not equal expected[i], reject it."*

For the key to be accepted, **every** character must satisfy:

```
input[i] ^ 0x13  ==  expected[i]
```

### Step 3 — Use XOR's reversibility

XOR has one crucial property: **it is its own inverse**.

```
If  A ^ B = C
Then C ^ B = A          (XOR both sides by B again)
```

Applied here:

```
input[i] ^ 0x13 == expected[i]
           ↓  XOR both sides with 0x13
input[i]        == expected[i] ^ 0x13
```

So each input character is just the matching expected byte XOR-ed with 0x13. The key is already embedded in the binary — just XOR it out.

### Step 4 — Reverse each byte by hand

```
index  expected   binary       ^ 0x13     binary       decimal  char
  0     0x76    0111 0110   ^  0001 0011 = 0110 0101  = 101  =  'e'
  1     0x60    0110 0000   ^  0001 0011 = 0111 0011  = 115  =  's'
  2     0x72    0111 0010   ^  0001 0011 = 0110 0001  =  97  =  'a'
  3     0x60    0110 0000   ^  0001 0011 = 0111 0011  = 115  =  's'
  4     0x7b    0111 1011   ^  0001 0011 = 0110 1000  = 104  =  'h'
```

### Step 5 — Recover the key with Python

```python
expected = [0x76, 0x60, 0x72, 0x60, 0x7b]
key = ''.join(chr(b ^ 0x13) for b in expected)
print(key)   # esash
```

Run the binary and enter `esash` — it prints `Correct!`.

### Why the author used XOR

XOR obfuscation is the simplest way to hide a hardcoded string from `strings ./binary`. The raw bytes in the binary are `76 60 72 60 7b`, which are not printable ASCII, so a quick `strings` scan misses them. As soon as you spot the XOR loop in the decompiler and the constant `0x13`, the game is over — XOR is trivially reversible.

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
