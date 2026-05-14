# Exercise 1 — GDB: Stepping Through Stack Frames

**Goal:** Run `program` under GDB, step through each function call, and use `info frame` to see how the stack changes. Draw the stack on paper after each function is entered.

**File:** `program.c`

---

## Step 1 — Compile with debug symbols and no mitigations

```bash
gcc -g -fno-stack-protector -no-pie -o program program.c
```

- `-g` embeds debug symbols so GDB can show source lines and variable names
- `-fno-stack-protector` disables the stack canary (covered in exercise 5)
- `-no-pie` fixes the binary at a predictable base address

---

## Step 2 — Launch GDB

```bash
gdb ./program
```

You will see the GDB banner and the `(gdb)` prompt.

---

## Step 3 — Set breakpoints at both functions

```
(gdb) break main
(gdb) break say_hello
```

Expected output:

```
Breakpoint 1 at 0x401156: file program.c, line 14.
Breakpoint 2 at 0x401126: file program.c, line 10.
```

The exact addresses on your machine may differ from these examples.

---

## Step 4 — Run the program

```
(gdb) run
```

GDB stops at the first line of `main`. You have not entered `say_hello` yet.

---

## Step 5 — Examine main's stack frame

```
(gdb) info frame
```

Expected output (addresses will differ):

```
Stack level 0, frame at 0x7fffffffe050:
 rip = 0x401156 in main (program.c:14); saved rip = 0x7ffff7ddd083
 source language c.
 Arglist at 0x7fffffffe040, args:
 Locals at 0x7fffffffe040, previous frame's sp is 0x7fffffffe050
 Saved registers:
  rbp at 0x7fffffffe040, rip at 0x7fffffffe048
```

Key fields to note:
- **frame at** — the address of this frame on the stack
- **saved rip** — the address `main` will return to when it finishes (`_start`)
- **rbp at** — where the saved base pointer lives
- **rip at** — where the saved return address lives (`rbp + 8`)

Draw this frame on paper now. Label: frame address, saved RBP location, return address location.

---

## Step 6 — Step through main's instructions

Use `ni` (next instruction) to advance one instruction at a time:

```
(gdb) ni
```

Press Enter repeatedly to keep stepping. Watch the source line indicator change as you move through `main`. Stop when you reach the `say_hello` call.

To see the current source context at any point:

```
(gdb) list
```

---

## Step 7 — Continue to say_hello

```
(gdb) continue
```

GDB hits the second breakpoint inside `say_hello`.

---

## Step 8 — Examine say_hello's stack frame

```
(gdb) info frame
```

Expected output:

```
Stack level 0, frame at 0x7fffffffe010:
 rip = 0x401126 in say_hello (program.c:10); saved rip = 0x401193
 called by frame at 0x7fffffffe050
 source language c.
 Arglist at 0x7fffffffe000, args: name=0x4052a0 "Alice\n"
 Locals at 0x7fffffffe000, previous frame's sp is 0x7fffffffe010
 Saved registers:
  rbp at 0x7fffffffe000, rip at 0x7fffffffe008
```

Notice:
- **called by frame at** — the address of `main`'s frame (the one you drew in step 5)
- **saved rip** — this is the address inside `main` that `say_hello` will return to
- The frame address is lower than `main`'s (the stack grows downward)

Draw this second frame below the first on your paper. Draw an arrow from say_hello's saved RIP to the corresponding instruction inside `main`.

---

## Step 9 — Inspect the local buffer

```
(gdb) info locals
```

Expected output:

```
buf = "\000\000\000\000\000..."
```

`buf[32]` exists but is uninitialized at this point. Note its address:

```
(gdb) print &buf
```

---

## Step 10 — Step through say_hello and watch buf fill

```
(gdb) ni
(gdb) ni
```

After the `sprintf` call, print buf:

```
(gdb) print buf
```

You will see the formatted greeting. Note how the buffer was filled from the lowest address upward.

---

## Step 11 — Return from say_hello

```
(gdb) finish
```

GDB runs until `say_hello` returns and stops in `main`. Notice you are back in main's frame. Run `info frame` again to confirm you are back in main's context.

---

## Step 12 — Quit GDB

```
(gdb) quit
```

---

## What to have on paper when you finish

A hand-drawn diagram like this:

```
HIGH ADDRESS
+------------------------------+
|  _start's return address     |  <- main returns here when done
+------------------------------+
|  saved RBP (_start's)        |  <- main's RBP points here
+------------------------------+
|  char *input (8 bytes)       |  [rbp - 8]
+------------------------------+
|  main's return address       |  <- say_hello returns here
+------------------------------+
|  saved RBP (main's)          |  <- say_hello's RBP points here
+------------------------------+
|  char buf[32]                |  [rbp - 32]
+------------------------------+  <- RSP (inside say_hello)
LOW ADDRESS
```

Fill in real addresses from your GDB session for each field.
