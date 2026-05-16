# Solution 03 — Runtime Token

---

## Step 1 — Confirm strings does not help

```bash
strings crackme-03
```

You will see `"Token: "`, `"Correct!"`, and `"Wrong."` — but no token. The token is built at runtime by XOR-ing two byte arrays, neither of which contains printable ASCII alone.

---

## Step 2 — Locate the strcmp call with objdump

```bash
objdump -d -M intel crackme-03 | grep -B 5 "strcmp"
```

Expected output (addresses will differ):

```nasm
    11d8: lea    rax, [rbp-0x60]     ; load address of input buffer
    11dc: mov    rsi, rcx            ; RSI = token (2nd argument)
    11df: mov    rdi, rax            ; RDI = input (1st argument)
    11e2: call   1060 <strcmp@plt>   ; strcmp(input, token)
```

This confirms `strcmp` is called with two pointers. At the moment of the call:
- **RDI** → what the user typed (`input`)
- **RSI** → the assembled token (`token`)

---

## Step 3 — Launch GDB

```bash
gdb -q ./crackme-03
```

The `-q` flag suppresses the banner.

---

## Step 4 — Enable pending breakpoints and break at strcmp

```
(gdb) set breakpoint pending on
(gdb) break strcmp
```

Expected output:

```
Breakpoint 1 (strcmp) pending.
```

`set breakpoint pending on` tells GDB to accept the breakpoint even though `strcmp` lives in libc and has not been loaded yet. It will arm when libc is mapped at program start.

---

## Step 5 — Run the program

```
(gdb) run
```

The program starts and prints its prompt:

```
Token:
```

Type any string and press Enter — for example `hello`:

```
Token: hello
```

GDB immediately pauses:

```
Breakpoint 1, __strcmp_sse2 () at ...
```

---

## Step 6 — Read both arguments

At this point `strcmp` has just been called. Its two arguments are in RDI and RSI:

```
(gdb) x/s $rdi
```

Expected:

```
0x7fffffffe380:  "hello"
```

```
(gdb) x/s $rsi
```

Expected:

```
0x7fffffffe350:  "r3v3rs3d!"
```

RDI holds what you typed. RSI holds the secret token: **`r3v3rs3d!`**

---

## Step 7 — Quit GDB and verify

```
(gdb) quit
```

```bash
echo "r3v3rs3d!" | ./crackme-03
```

Expected output:

```
Token: Correct!
```

---

## What just happened

The source builds the token by XOR-ing two byte arrays:

```c
unsigned char a[] = {0x16, 0x01, 0x24, ...};
unsigned char b[] = {0x64, 0x32, 0x52, ...};
for (int i = 0; i < 9; i++) out[i] = a[i] ^ b[i];
```

Neither `a` nor `b` contains printable text. The XOR result `"r3v3rs3d!"` only exists in memory after `build_token()` runs — `strings` has no way to see it because it never appears on disk.

Dynamic analysis bypasses this entirely. By the time `strcmp` is called, the token is sitting in RSI as a plain C string, ready to read.

---

## Key GDB commands used

| Command | What it does |
|---|---|
| `set breakpoint pending on` | Accept breakpoints on symbols not yet loaded |
| `break strcmp` | Pause every time strcmp is called |
| `run` | Start the program |
| `x/s $rdi` | Print the string at the address in RDI |
| `x/s $rsi` | Print the string at the address in RSI |
