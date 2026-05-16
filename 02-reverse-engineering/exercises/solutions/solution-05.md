# Solution 05 — ltrace: Library Call Interception

The password is assembled iteratively. Each `ltrace` run reveals one more constraint until the full password is visible.

---

## Step 1 — First guess: anything

```bash
ltrace ./crackme-05 hello
```

Output:

```
strlen("hello") = 5
puts("Wrong.")  = 6
Wrong.
```

**What this tells you:** `strlen` was called and the program bailed out immediately after. The input failed a length check. You don't know the required length yet, but you know one exists.

---

## Step 2 — Try a longer input

```bash
ltrace ./crackme-05 helloworld
```

Output:

```
strlen("helloworld") = 10
puts("Wrong.")       = 6
Wrong.
```

Still fails at `strlen`. Try a different length:

```bash
ltrace ./crackme-05 hello123
```

Output:

```
strlen("hello123")                = 8
strchr("hello123", '-')           = nil
puts("Wrong.")                    = 6
Wrong.
```

**What this tells you:**
- Length **8** passes the first check — the password is 8 characters long.
- `strchr(pass, '-')` returned `nil` — the password must contain a `-` character.

---

## Step 3 — Add the separator

You now know the password has 8 characters and contains `-`. Try putting it in different positions:

```bash
ltrace ./crackme-05 hel-o123
```

Output:

```
strlen("hel-o123")                = 8
strchr("hel-o123", '-')           = "-o123"
strncmp("hel-o123", "CTF", 3)    = 1
puts("Wrong.")                    = 6
Wrong.
```

**What this tells you:**
- `strchr` found `-` — this position passed.
- `strncmp("hel-o123", "CTF", 3)` compares the first 3 characters of your input to `"CTF"`. Return value `1` means not equal.
- The password must **start with `CTF`**, and the `-` must be at position 3 (right after `CTF`).

---

## Step 4 — Fix the prefix

```bash
ltrace ./crackme-05 CTF-XXXX
```

Output:

```
strlen("CTF-XXXX")                = 8
strchr("CTF-XXXX", '-')           = "-XXXX"
strncmp("CTF-XXXX", "CTF", 3)    = 0
strcmp("XXXX", "2024")            = 1
puts("Wrong.")                    = 6
Wrong.
```

**What this tells you:**
- `strncmp` returned `0` — prefix `CTF` is correct.
- `strcmp("XXXX", "2024")` — the program compares the part of your input **after the `-`** with `"2024"`. The second argument is the expected value: **`2024`**.

The password is **`CTF-2024`**.

---

## Step 5 — Verify

```bash
ltrace ./crackme-05 CTF-2024
```

Output:

```
strlen("CTF-2024")                = 8
strchr("CTF-2024", '-')           = "-2024"
strncmp("CTF-2024", "CTF", 3)    = 0
strcmp("2024", "2024")            = 0
puts("Access granted!")           = 15
Access granted!
```

Every check passes. `strcmp` returns `0` (equal) and the program prints `Access granted!`.

---

## Why ltrace worked where static analysis was slow

The validation logic in the binary looks roughly like this in disassembly:

```nasm
; strlen(pass) → result in RAX
call strlen
cmp  rax, 8
jne  wrong

; strchr(pass, '-') → pointer in RAX
mov  esi, 0x2d          ; '-'
call strchr
test rax, rax
jz   wrong
; pointer arithmetic: sep - pass != 3 ?
mov  rcx, rax
sub  rcx, [rbp-0x18]   ; rcx = sep - pass
cmp  rcx, 3
jne  wrong

; strncmp(pass, "CTF", 3)
...
call strncmp
test eax, eax
jnz  wrong

; strcmp(pass+4, "2024")
lea  rdi, [rbp-0x18]
add  rdi, 4             ; pass + 4
lea  rsi, [rip+0x...]   ; pointer to "2024"
call strcmp
```

To follow this statically you must:
- Track which register holds `sep` after `strchr` returns
- Manually compute `sep - pass` in your head
- Find the `.rodata` address that `rsi` points to for the `strcmp` call

`ltrace` does all of this automatically. It intercepts each call at the PLT stub and prints the resolved arguments — including string pointers already dereferenced — before the function even runs. No manual register tracking required.

---

## ltrace vs strace — when to use which

| Tool | Intercepts | Useful for |
|---|---|---|
| `ltrace` | Library calls (`strcmp`, `malloc`, `strlen`, ...) | Understanding program logic, finding comparisons |
| `strace` | System calls (`read`, `write`, `open`, `execve`, ...) | Understanding I/O, file access, network activity |

For crackmes: **ltrace first**. If the check happens entirely in application code with no library calls, fall back to GDB.
