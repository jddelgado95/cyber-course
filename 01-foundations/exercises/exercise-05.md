# Exercise 5 — Mitigations: Watching the Exploit Break

**Goal:** Re-compile `program-vuln.c` with real-world mitigations enabled one at a time, run `checksec` after each build, and observe exactly how each mitigation breaks the exploit from exercise 4.

**File:** `program-vuln.c`

**Prerequisite:** Complete exercise 4. You should have `exploit.py` ready.

---

## Baseline — The exploit works without mitigations

Start by confirming the exercise 4 exploit still works:

```bash
gcc -g -fno-stack-protector -no-pie -w -o program-vuln program-vuln.c
python3 exploit.py
```

You should see:

```
[+] You redirected execution to win()!
```

Run `checksec` on this binary to see its protection profile:

```bash
checksec --file=./program-vuln
```

Expected output:

```
RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH
Partial RELRO   No canary found   NX enabled    No PIE          No RPATH   No RUNPATH
```

Note: **No canary found**, **No PIE**. This is the insecure baseline.

---

## Mitigation 1 — Stack Canary (`-fstack-protector`)

### What it does

The compiler inserts a secret random value (the "canary") between local variables and the saved return address. Before the function returns, it checks that the canary is unchanged. If the canary was overwritten — as it will be during a stack overflow — the program calls `__stack_chk_fail()` and aborts immediately, before `RET` ever executes.

```
Stack layout with canary:

[rbp - 32]  buf[32]
[rbp -  8]  canary          <- inserted by compiler
[rbp +  0]  saved RBP
[rbp +  8]  return address
```

To overwrite the return address, an attacker must also overwrite the canary. The check at function epilogue catches this.

### Step 1 — Compile with the stack canary

```bash
gcc -g -fstack-protector -no-pie -w -o program-vuln-canary program-vuln.c
```

### Step 2 — Run checksec

```bash
checksec --file=./program-vuln-canary
```

Expected output:

```
RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH
Partial RELRO   Canary found      NX enabled    No PIE          No RPATH   No RUNPATH
```

**Canary found** now appears.

### Step 3 — Update exploit.py to target the new binary

Edit `exploit.py` temporarily to point at the new binary:

```python
elf = ELF('./program-vuln-canary')
```

### Step 4 — Run the exploit

```bash
python3 exploit.py
```

Expected output:

```
[+] Starting local process './program-vuln-canary': pid 13338
[*] Switching to interactive mode
Enter your name: Hello!, AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
*** stack smashing detected ***: terminated
[*] Got EOF while reading in interactive
[*] Process './program-vuln-canary' stopped with exit code -6 (SIGABRT)
```

The canary check fired. The program called `abort()` instead of executing `RET`. Your `win()` address was never reached.

### Step 5 — Confirm in GDB

```bash
gdb ./program-vuln-canary
(gdb) run <<< $(python3 -c "from pwn import *; payload = b'A'*32 + p64(0x401126); sys.stdout.buffer.write(payload + b'\n')")
```

GDB will stop with:

```
Program received signal SIGABRT, Aborted.
```

Not SIGSEGV. The canary check aborted the process before reaching `RET`.

---

## Mitigation 2 — PIE (`-pie`)

### What it does

Position Independent Executable: the binary is compiled to run at any base address. Combined with ASLR (which you disabled at the start but is on by default in production), the addresses of all functions — including `win()` — are randomized on every run.

Without PIE, `win()` is always at a fixed address (e.g., `0x401126`). You hardcoded that in your exploit.

With PIE, `win()` might be at `0x55a3f2401126` on one run, and `0x7f3c01401126` on the next. Your hardcoded address is wrong on every run.

### Step 1 — Re-enable ASLR

```bash
echo 2 | sudo tee /proc/sys/kernel/randomize_va_space
```

This re-enables ASLR (the default Linux setting).

### Step 2 — Compile with PIE and no canary

Keep the canary off for now so PIE is the only variable:

```bash
gcc -g -fno-stack-protector -pie -w -o program-vuln-pie program-vuln.c
```

### Step 3 — Run checksec

```bash
checksec --file=./program-vuln-pie
```

Expected output:

```
RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH
Full RELRO      No canary found   NX enabled    PIE enabled     No RPATH   No RUNPATH
```

**PIE enabled** now appears.

### Step 4 — Find win() address on two separate runs

```bash
gdb ./program-vuln-pie -ex "print win" -ex quit --quiet 2>/dev/null
gdb ./program-vuln-pie -ex "print win" -ex quit --quiet 2>/dev/null
```

With ASLR on and PIE enabled, `win()` is at a different address each time:

```
$1 = {void ()} 0x55a3f2401126 <win>
$1 = {void ()} 0x563e04c01126 <win>
```

With your old exploit hardcoding `0x401126`, the address is always wrong.

### Step 5 — Update exploit.py to target the PIE binary and run it

```python
elf = ELF('./program-vuln-pie')
```

Run it:

```bash
python3 exploit.py
```

Expected output:

```
[+] Starting local process './program-vuln-pie': pid 13339
[*] Switching to interactive mode
Enter your name: Hello!, AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
Segmentation fault (core dumped)
[*] Got EOF while reading in interactive
```

The program crashes — but at a random wrong address, not at `win()`. The `ELF()` helper in pwntools reads the binary's file on disk to find symbols, but with ASLR, those file offsets are no longer the runtime addresses. You would need a memory leak to find `win()`'s actual location at runtime.

### Step 6 — Disable ASLR again for future exercises

```bash
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

---

## Mitigation 3 — Both enabled

Compile with both protections:

```bash
gcc -g -fstack-protector -pie -w -o program-vuln-hardened program-vuln.c
```

```bash
checksec --file=./program-vuln-hardened
```

Expected output:

```
RELRO           STACK CANARY      NX            PIE             RPATH      RUNPATH
Full RELRO      Canary found      NX enabled    PIE enabled     No RPATH   No RUNPATH
```

Now both mitigations are active. Your exploit from exercise 4 fails completely against this binary.

---

## Summary

| Mitigation | Flag | What it breaks |
|---|---|---|
| Stack canary | `-fstack-protector` | Detects overflow before `RET` — process aborts |
| PIE + ASLR | `-pie` (+ ASLR on) | Randomizes function addresses — hardcoded `win_addr` is wrong |
| NX (always on) | default | Prevents injected shellcode from executing — you cannot just jump to bytes you pushed |

---

## What comes next

These mitigations are not unbeatable — they are just harder to work around:

- **Canary bypass**: requires leaking the canary value before sending the overflow
- **PIE/ASLR bypass**: requires an information leak (a format string bug, or an intentional `printf` of an address) to recover the runtime base
- **NX bypass**: return-oriented programming (ROP) — chain together existing executable code instead of injecting new code

Both leaks and ROP are covered in detail in module 3 (`03-binary-exploitation/`).
