# Module 2 — Reverse Engineering: Exercises

Four crackmes that cover static and dynamic analysis, in order of difficulty.

| # | Name | Primary technique | Difficulty |
|---|------|-------------------|------------|
| 01 | Strings Hunt | Static — `strings`, `objdump` | Easy |
| 02 | XOR Crackme | Static — Ghidra / `objdump` + Python | Easy–Mid |
| 03 | Runtime Token | Dynamic — GDB | Mid |
| 04 | Serial Validator | Static + Dynamic | Mid |

Work through them in order. Each exercise introduces a tool or concept used in the next.

---

## Environment

Kali Linux (VM or bare metal). All tools are pre-installed.

**Required:** `gcc`, `gdb`, `objdump`, `strings`, `checksec`, `python3`

**Optional (exercises 02, 04):** Ghidra

---

## One-Time Setup

```bash
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

---

## Compile All

```bash
cd /path/to/02-reverse-engineering/exercises

gcc            -o crackme-01 crackme-01.c
gcc            -o crackme-02 crackme-02.c
gcc -g         -o crackme-03 crackme-03.c
gcc -g -O0     -o crackme-04 crackme-04.c
```

`-g` embeds debug symbols so GDB can show function names.
`-O0` disables optimizations so the disassembly stays readable.

---

## Files

| File | Exercise |
|------|----------|
| `crackme-01.c` | 01 — Strings Hunt |
| `crackme-02.c` | 02 — XOR Crackme |
| `crackme-03.c` | 03 — Runtime Token |
| `crackme-04.c` | 04 — Serial Validator |
| `solutions/solution-01.md` | Full walkthrough for exercise 01 |
| `solutions/solution-02.md` | Full walkthrough for exercise 02 |
| `solutions/solution-03.md` | Full walkthrough for exercise 03 |
| `solutions/solution-04.md` | Full walkthrough for exercise 04 |
