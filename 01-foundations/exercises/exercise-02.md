# Exercise 2 — Heap: Confirming Addresses with /proc/maps

**Goal:** Find the exact heap address returned by `malloc(64)` inside GDB, then verify that address falls inside the `[heap]` region shown in `/proc/<pid>/maps`.

**File:** `program.c`

**Prerequisite:** Complete exercise 1 first. You should already have `program` compiled.

---

## Step 1 — Confirm the binary is already compiled

```bash
ls -l program
```

If the file is missing, compile it:

```bash
gcc -g -fno-stack-protector -no-pie -o program program.c
```

---

## Step 2 — Launch GDB

```bash
gdb ./program
```

---

## Step 3 — Set a breakpoint after malloc

You want to stop execution right after `malloc(64)` returns so you can inspect the pointer it gave back. Set a breakpoint on the line that calls `fgets` — by that point, `malloc` has already run and `input` holds the heap address.

```
(gdb) break program.c:15
```

Line 15 is the `fgets` call. To confirm the line number:

```
(gdb) list
```

Adjust the number if needed so you break on `fgets`.

---

## Step 4 — Run the program

```
(gdb) run
```

GDB stops just before `fgets`. `malloc` has already returned.

---

## Step 5 — Print the heap pointer

```
(gdb) print input
```

Expected output:

```
$1 = 0x4052a0 ""
```

The value (here `0x4052a0`) is the address returned by `malloc`. This is where your 64-byte heap buffer lives. Write it down — you will look it up in the maps file.

---

## Step 6 — Get the process ID

```
(gdb) info proc
```

Expected output:

```
process 12345
cmdline = '/home/kali/exercises/program'
cwd = '/home/kali/exercises'
exe = '/home/kali/exercises/program'
```

Write down the PID (here `12345`).

---

## Step 7 — Open a second terminal and inspect /proc/maps

Leave GDB running. Open a second terminal window.

In the new terminal:

```bash
cat /proc/12345/maps
```

Replace `12345` with your actual PID. You will see several lines. Look for the one labeled `[heap]`:

```
004051000-004073000 rw-p 00000000 00:00 0    [heap]
```

The two hex values are the start and end of the heap region. Confirm that the address you wrote down in step 5 falls between them:

```
0x004051000  <=  0x4052a0  <=  0x004073000   ✓
```

You have just confirmed that `malloc` returned a pointer inside the heap.

---

## Step 8 — Filter to just the heap line

If the output is long, filter it:

```bash
cat /proc/12345/maps | grep heap
```

---

## Step 9 — Inspect the full memory map

While you have the maps file open, look at all the regions:

```bash
cat /proc/12345/maps
```

You should recognize these regions from the workflow diagram:

| Region label | What it is |
|---|---|
| `/home/kali/.../program` (r-xp) | `.text` — your machine code |
| `/home/kali/.../program` (rw-p) | `.data` / `.bss` — globals |
| `[heap]` | malloc arena |
| `/lib/.../libc.so.6` | C standard library |
| `[stack]` | The program stack |

---

## Step 10 — Resume and quit

Back in the GDB terminal, type your name and press Enter to let the program run to completion:

```
(gdb) continue
```

GDB will wait for `fgets` input. Type anything and press Enter.

```
(gdb) quit
```

---

## What you should be able to answer

1. What address did `malloc(64)` return?
2. What is the start and end address of the `[heap]` region?
3. Does the malloc address fall within those bounds?
4. Is the heap above or below the stack in the address space?
