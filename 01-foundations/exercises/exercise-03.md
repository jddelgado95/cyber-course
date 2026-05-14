# Exercise 3 — Finding and Verifying the Return Address

**Goal:** Set a breakpoint at `say_hello`, read the return address from the stack using `$rbp`, step through until `RET`, and confirm the CPU jumps to exactly that address.

**File:** `program.c`

**Prerequisite:** `program` must be compiled (done in exercise 1).

---

## Step 1 — Launch GDB

```bash
gdb ./program
```

---

## Step 2 — Set a breakpoint at say_hello

```
(gdb) break say_hello
```

Expected output:

```
Breakpoint 1 at 0x401126: file program.c, line 9.
```

---

## Step 3 — Run the program

```
(gdb) run
```

When GDB asks for input (fgets), type a short name and press Enter:

```
Alice
```

GDB stops at the first line of `say_hello`.

---

## Step 4 — Print the base pointer

```
(gdb) print $rbp
```

Expected output:

```
$1 = (void *) 0x7fffffffe000
```

RBP points to the base of say_hello's current stack frame. This is important because the stack layout around RBP is always the same:

```
[rbp - N] = local variables   (below RBP)
[rbp + 0] = saved RBP         (caller's base pointer)
[rbp + 8] = return address    (where to go when say_hello returns)
```

---

## Step 5 — Read the return address

```
(gdb) x/gx $rbp+8
```

- `x` = examine memory
- `/g` = 8-byte (giant) unit
- `x` = display as hex
- `$rbp+8` = the memory address to read from

Expected output:

```
0x7fffffffe008: 0x0000000000401193
```

The value on the right (`0x401193` in this example) is the return address. When `say_hello` executes `RET`, the CPU will pop this value into RIP and jump there.

**Write this address down.** You will verify it in step 8.

---

## Step 6 — Confirm it points inside main

Disassemble main to see where `0x401193` (your value) lands:

```
(gdb) disassemble main
```

Look for the instruction immediately after the `call say_hello` line. Its address should match what you found in step 5.

Example:

```
0x40118e <main+56>: call   0x401126 <say_hello>
0x401193 <main+61>: mov    edi, 0x0           <- this is the return address
```

The instruction at `0x401193` is what runs after `say_hello` returns. That is why it was saved on the stack — so `RET` knows where to come back to.

---

## Step 7 — Step to the RET instruction

Step through `say_hello` one instruction at a time with `ni` until you reach the `ret` instruction:

```
(gdb) ni
(gdb) ni
(gdb) ni
...
```

Watch the disassembly with:

```
(gdb) disassemble
```

Stop when you see the arrow (`=>`) pointing at the `ret` line. Do not execute it yet.

You can also jump directly to the `ret` with:

```
(gdb) finish
```

`finish` runs the rest of the function and stops right after it returns (back in `main`).

---

## Step 8 — Confirm RIP jumped to the return address

After `finish` (or after stepping through `ret` with `ni`), you are back in `main`. Check the current instruction pointer:

```
(gdb) print $rip
```

Expected output:

```
$2 = (void (*)()) 0x401193
```

This should match the value you wrote down in step 5. The `RET` instruction popped the saved return address off the stack and loaded it into RIP — exactly as predicted.

---

## Step 9 — Quit

```
(gdb) quit
```

---

## What you should be able to explain

1. Why is the return address stored at `$rbp + 8` specifically?
2. What instruction puts the return address onto the stack in the first place?  
   (Hint: look at what `CALL` does before `say_hello` runs.)
3. What would happen if you overwrote the value at `$rbp + 8` before `RET` executed?

The answer to question 3 is the entire premise of exercise 4.
