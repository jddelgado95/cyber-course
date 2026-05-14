# End-to-End Workflow — Exercises

These exercises accompany `05-end-to-end-workflow.md`. Complete them in order — each one builds on the previous.

By the end you will have:
- Stepped through a program in GDB and drawn the stack by hand
- Verified heap allocations against `/proc/<pid>/maps`
- Located a return address at runtime and watched `RET` use it
- Written a working stack overflow exploit that redirects execution
- Seen how compiler mitigations break that exploit

---

## Environment

Kali Linux (VM or bare metal) has every required tool pre-installed. All exercises were written for Kali.

**Required tools:** `gcc`, `gdb`, `pwntools`, `checksec`

If `pwntools` is missing:

```bash
pip3 install pwntools
```

---

## One-Time Setup — Disable ASLR

ASLR randomizes memory addresses on every run. Disable it so addresses stay consistent while you learn:

```bash
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

Confirm it is off (output must be `0`):

```bash
cat /proc/sys/kernel/randomize_va_space
```

This resets on reboot. Re-run the first command each time you start a new session.

---

## Files in This Directory

| File | Purpose |
|------|---------|
| `program.c` | Safe version — used in exercises 1, 2, 3 |
| `program-vuln.c` | Vulnerable version with `gets()` and `win()` — used in exercises 4, 5 |

---

## Exercise Index

| # | File | What you will do |
|---|------|-----------------|
| 1 | `exercise-01.md` | Step through stack frames in GDB |
| 2 | `exercise-02.md` | Confirm heap allocation in `/proc/maps` |
| 3 | `exercise-03.md` | Find and verify the return address |
| 4 | `exercise-04.md` | Write a buffer overflow exploit |
| 5 | `exercise-05.md` | Watch mitigations break the exploit |
