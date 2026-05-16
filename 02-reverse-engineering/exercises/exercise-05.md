# Exercise 05 — ltrace: Library Call Interception

**Technique:** Dynamic analysis
**Difficulty:** Mid
**Tools:** `ltrace`, `objdump` (optional)

---

## The Challenge

`crackme-05` takes a password as a command-line argument:

```bash
./crackme-05 <password>
```

The validation logic calls several libc string functions in sequence. Tracing through the disassembly is tedious because intermediate results flow between calls via registers and pointer arithmetic. But every library call is intercepted and printed by `ltrace` — including its arguments and return value.

Your goal: find the correct password using **only `ltrace`**. No disassembler required.

---

## Compile

```bash
gcc -o crackme-05 crackme-05.c
```

---

## Background: what ltrace does

`ltrace` wraps a program and prints every call it makes to shared library functions (libc, etc.) — the function name, its arguments, and its return value.

```bash
ltrace ./crackme-05 hello
```

Example output format:

```
strlen("hello")              = 5
puts("Wrong.")               = 6
Wrong.
```

Each line shows: `function(args...) = return_value`

This is different from `strace`, which traces **system calls** (read, write, open). `ltrace` traces **library calls** (strcmp, strlen, malloc).

---

## Strategy

Run `ltrace` with a series of guesses. Each run either:
- Shows a failed check (tells you what the input must satisfy)
- Shows a comparison with the expected value (tells you the answer directly)

Iterate — each attempt reveals more information about the format the program expects.

---

## Hints

1. Start with a completely wrong guess and read every line of ltrace output.
2. Each call that returns unexpectedly tells you one constraint you violated.
3. Fix one constraint at a time and run again.
4. Pay close attention to `strcmp` and `strncmp` calls — their **second argument** is what the program expects your input to match.

---

## What You Should Know When Finished

- How `ltrace` intercepts and prints library calls at runtime
- How to read ltrace output: function, arguments, return value
- Why dynamic tracing can be faster than static analysis for multi-step string checks
- The difference between `ltrace` (library calls) and `strace` (system calls)
