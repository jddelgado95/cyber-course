# Solution 01 — Strings Hunt

---

## Step 1 — Identify the binary

```bash
file crackme-01
```

Expected output:

```
crackme-01: ELF 64-bit LSB pie executable, x86-64, dynamically linked, not stripped
```

Key information:
- **ELF 64-bit** — Linux executable for x86-64
- **dynamically linked** — uses shared libraries (libc)
- **not stripped** — symbol names are still in the binary (function names are readable)

---

## Step 2 — Check security mitigations

```bash
checksec --file=crackme-01
```

Expected output:

```
RELRO     STACK CANARY  NX    PIE   RPATH  RUNPATH  Symbols
Full RELRO  No canary   NX enabled  PIE enabled  No  No  No
```

No stack canary and no ASLR bypass needed — this exercise is purely about finding the secret, not exploiting anything.

---

## Step 3 — List all readable strings

```bash
strings crackme-01
```

Expected output (trimmed to interesting lines):

```
/lib64/ld-linux-x86-64.so.2
...
puts
strcmp
fgets
printf
...
Enter password:
sup3rs3cr3t
Access granted!
Access denied.
...
```

`sup3rs3cr3t` stands out — it is not a library name, a format string, or a standard message. It is the password.

---

## Step 4 — Confirm with objdump (optional)

If you want to see exactly where it is used:

```bash
objdump -d -M intel crackme-01 | grep -A 5 "strcmp"
```

You will see a `call` to `strcmp` with the two string pointers loaded into RDI and RSI immediately before. One of those pointers leads to `"sup3rs3cr3t"`.

---

## Step 5 — Verify

```bash
echo "sup3rs3cr3t" | ./crackme-01
```

Expected output:

```
Enter password: Access granted!
```

---

## What just happened

The C source has:

```c
if (strcmp(input, "sup3rs3cr3t") == 0)
```

String literals like `"sup3rs3cr3t"` are stored verbatim in the binary's `.rodata` section (read-only data). The `strings` tool scans for sequences of printable ASCII bytes — it found the password directly.

**Lesson:** never store a secret as a plain string literal in compiled code. Anyone with `strings` finds it in seconds.
