# Solution 04 — Serial Validator

---

## Phase 1 — Static Analysis

### Step 1 — Find and read the validate function

```bash
objdump -d -M intel crackme-04 | grep -A 80 "<validate>"
```

Work through the disassembly and map each block to a condition. With `-g -O0` the code stays close to the source. You will find four checks:

```
Check 1:  strlen(s) != 6      → reject
Check 2:  s[0] != 'K'         → reject
Check 3:  sum of all bytes != 504   → reject
Check 4:  s[2] ^ s[3] != 3   → reject
```

In Ghidra the decompiler output makes this even more readable:

```c
int validate(char *s) {
    if (strlen(s) != 6) return 0;
    if (s[0] != 'K') return 0;
    int sum = 0;
    for (int i = 0; i < 6; i++) sum += (unsigned char)s[i];
    if (sum != 504) return 0;
    if ((s[2] ^ s[3]) != 3) return 0;
    return 1;
}
```

### Step 2 — Solve the constraints on paper

Write the conditions in order of freedom (most constrained first):

```
s has 6 characters
s[0] = 'K'                         fixed: K = 75
s[2] ^ s[3] = 3                    one XOR pair to choose
s[0]+s[1]+s[2]+s[3]+s[4]+s[5] = 504
```

**Pick the XOR pair first.**

Choose printable uppercase letters where `A ^ B = 3`. XOR of two bytes equals 3 when exactly bits 0 and 1 differ:

```
'U' = 85 = 01010101
'V' = 86 = 01010110
      XOR = 00000011 = 3  ✓
```

So `s[2] = 'U'`, `s[3] = 'V'`.

**Now solve the sum.**

Known so far: s[0]=K(75), s[2]=U(85), s[3]=V(86).
Their sum: 75 + 85 + 86 = 246.
Remaining for s[1], s[4], s[5]: 504 − 246 = 258.
258 ÷ 3 = 86 exactly → s[1] = s[4] = s[5] = 86 = `'V'`.

**Candidate serial: `KVUVVV`**

### Step 3 — Verify the math with Python

```python
s = "KVUVVV"

print("Length :", len(s))
print("s[0]   :", s[0], "== 'K'?", s[0] == 'K')
print("Sum    :", sum(ord(c) for c in s), "== 504?", sum(ord(c) for c in s) == 504)
print("s[2]^s[3]:", ord(s[2]) ^ ord(s[3]), "== 3?", (ord(s[2]) ^ ord(s[3])) == 3)
```

Expected output:

```
Length : 6
s[0]   : K == 'K'? True
Sum    : 504 == 504? True
s[2]^s[3]: 3 == 3? True
```

All four checks pass. Run the binary to confirm:

```bash
./crackme-04 KVUVVV
```

Expected output:

```
Serial accepted.
```

---

## Phase 2 — Dynamic Verification with GDB

Now step through `validate` in GDB to watch each check succeed at the assembly level.

### Step 4 — Launch GDB and set a breakpoint

```bash
gdb -q ./crackme-04
```

```
(gdb) break validate
Breakpoint 1 at 0x...: file crackme-04.c, line 4.
(gdb) run KVUVVV
```

GDB pauses at the first line of `validate`.

### Step 5 — Inspect the argument

```
(gdb) x/s $rdi
```

Expected:

```
0x7fffffffe680:  "KVUVVV"
```

RDI holds a pointer to your serial. Confirmed it arrived intact.

### Step 6 — Step through each check

Use `next` to advance one source line at a time (since we compiled with `-g`):

```
(gdb) next
```

**Check 1 — length:**

```
(gdb) next
(gdb) print (int)strlen("KVUVVV")
$1 = 6
```

The condition `strlen(s) != 6` is false → check passes, execution continues.

**Check 2 — first character:**

```
(gdb) next
(gdb) print (char)$rdi[0]
```

Or inspect the register directly:

```
(gdb) info registers
```

The comparison `s[0] != 'K'` is false → passes.

**Check 3 — sum:**

Step through the loop with `next` until you reach the sum comparison. Print the accumulated sum:

```
(gdb) print sum
$2 = 504
```

`504 != 504` is false → passes.

**Check 4 — XOR:**

```
(gdb) next
(gdb) print (int)'U' ^ (int)'V'
$3 = 3
```

`3 != 3` is false → passes.

### Step 7 — Watch validate return 1

```
(gdb) finish
```

GDB runs to the end of `validate` and shows the return value:

```
Value returned is $4 = 1
```

`1` means accepted. Execution returns to `main`, which prints `"Serial accepted."`.

---

## What just happened

**Static analysis** told you the complete logic of the program — four mathematical constraints that define the valid serial space. You solved them as equations.

**Dynamic analysis** confirmed your solution at the instruction level. Each check was visible in GDB as it executed, with register and variable values you could inspect directly.

This is the standard workflow for a real CTF: static analysis gives you the shape of the problem, dynamic analysis verifies your answer and catches edge cases you might have missed.

---

## Note: there are many valid serials

Any 6-character string starting with `K`, summing to 504, with `s[2] ^ s[3] == 3` is accepted. `KVUVVV` is one solution. Others include `KVUVWU`, `KAUUVV`, etc. The constraints are under-specified by design — the exercise is about reading the conditions, not about finding a unique answer.
