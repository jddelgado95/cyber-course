# Exercise 04 — Serial Validator

**Technique:** Static analysis + Dynamic analysis
**Difficulty:** Mid
**Tools:** `objdump`, Ghidra (optional), Python, `gdb`

---

## The Challenge

`crackme-04` takes a serial number as a command-line argument and validates it against several conditions. There is no single hardcoded string to find — the program checks multiple mathematical properties of the input.

```bash
./crackme-04 <serial>
```

Your goal: find a valid 6-character serial using static analysis, then verify it step-by-step with GDB.

---

## Compile

```bash
gcc -g -O0 -o crackme-04 crackme-04.c
```

(`-O0` keeps the disassembly close to the source, making it easier to read)

---

## What to Do

### Phase 1 — Static analysis

1. Disassemble the binary and find the `validate` function.
2. Read through it and identify every condition the serial must satisfy.
   There are **four checks**. Write them down as math.
3. Pick printable ASCII characters that satisfy all four checks simultaneously.
4. Use Python to verify your math before running the binary.

### Phase 2 — Dynamic verification

5. Launch GDB, set a breakpoint at `validate`, and run with your serial.
6. Step through each check one instruction at a time.
7. Confirm that every condition passes and the function returns 1.

---

## Hints

1. The first check is on length — count characters carefully.
2. The second check pins one specific character at a specific position.
3. The third check is a sum — you have freedom to choose the remaining characters as long as they add up correctly.
4. The fourth check uses XOR on two adjacent characters.
5. Start with the pinned character and the XOR pair, then solve the sum last.
6. In GDB, `next` steps over a function call, `stepi` steps one machine instruction.

---

## What You Should Know When Finished

- How to extract multiple conditions from a disassembly and express them as equations
- How to solve a multi-constraint problem systematically (fix the easy constraints, solve the free variables last)
- How to use GDB to step through a function and confirm each condition at the assembly level
- How static and dynamic analysis complement each other: static reveals the logic, dynamic confirms the solution
