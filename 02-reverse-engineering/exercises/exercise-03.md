# Exercise 03 — Runtime Token

**Technique:** Dynamic analysis
**Difficulty:** Mid
**Tools:** `strings`, `objdump`, `gdb`

---

## The Challenge

`crackme-03` asks for a token. The token is not stored anywhere in the binary as a readable string — it is constructed at runtime from raw bytes. Static analysis alone will not reveal it.

Your goal: intercept the token at runtime using GDB.

---

## Compile

```bash
gcc -g -o crackme-03 crackme-03.c
```

(`-g` embeds debug symbols so GDB shows function names)

---

## What to Do

1. Confirm with `strings` that the token is not readable in the binary.
2. Use `objdump` to locate where the comparison happens (look for a `strcmp` call).
3. Run the binary under GDB, set a breakpoint at the comparison, and read both arguments from the registers.

---

## Background: reading arguments in GDB

On x86-64 Linux, function arguments are passed in registers in this order:

```
1st argument → RDI
2nd argument → RSI
3rd argument → RDX
...
```

`strcmp(a, b)` receives `a` in RDI and `b` in RSI. When GDB pauses at the `strcmp` call, you can print either string with:

```
(gdb) x/s $rdi
(gdb) x/s $rsi
```

---

## Hints

1. Run the binary once normally to understand what it asks.
2. Set the breakpoint before starting the program with `run`.
3. When the program prompts you, type any string — the breakpoint fires immediately after.
4. One of the two registers holds the secret token.

---

## What You Should Know When Finished

- How to set a breakpoint at a library function in GDB
- How to read string arguments from registers at a function call
- Why dynamic analysis reveals secrets that static analysis cannot
