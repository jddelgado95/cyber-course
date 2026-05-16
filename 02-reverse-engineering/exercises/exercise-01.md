# Exercise 01 — Strings Hunt

**Technique:** Static analysis
**Difficulty:** Easy
**Tools:** `file`, `strings`, `checksec`, `objdump`

---

## The Challenge

You have a compiled binary `crackme-01`. It asks for a password and either prints "Access granted!" or "Access denied." You do not have the source code.

Your goal: find the correct password **without running the program**.

---

## Compile

```bash
gcc -o crackme-01 crackme-01.c
```

---

## What to Do

Use static analysis tools to examine the binary without executing it. Start with the broadest tool (`file`, `strings`) and narrow in with `objdump` if needed.

**Submit:** the password that prints "Access granted!"

---

## Hints

1. Before anything else, identify what kind of file you are dealing with.
2. Readable strings are often embedded directly in the binary.
3. If you find a suspicious-looking string, try it as the password.

---

## What You Should Know When Finished

- How to identify an ELF binary and read its mitigations
- How `strings` extracts human-readable content from a binary
- Why hardcoded secrets are trivially recoverable with static analysis
