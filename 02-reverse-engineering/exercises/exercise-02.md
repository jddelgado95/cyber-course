# Exercise 02 — XOR Crackme

**Technique:** Static analysis
**Difficulty:** Easy–Mid
**Tools:** `strings`, `objdump`, Ghidra (optional), Python

---

## The Challenge

`crackme-02` asks for a key. The author tried to hide it by not storing it as plain text. `strings` will not directly reveal the answer.

Your goal: recover the key by reading the binary's logic and reversing the obfuscation.

---

## Compile

```bash
gcc -o crackme-02 crackme-02.c
```

---

## What to Do

1. Confirm that `strings` does not immediately reveal the key.
2. Disassemble the binary and find the function that checks the input.
3. Identify what transformation is applied to the input before comparing.
4. Reverse that transformation to recover the original key.

---

## Hints

1. The check function XORs each input byte with a constant before comparing it to a stored value.
2. XOR is its own inverse: if `input ^ KEY == stored`, then `stored ^ KEY == input`.
3. Once you know the stored bytes and the XOR constant, a single line of Python recovers the key.

---

## What You Should Know When Finished

- How to locate and read a specific function in `objdump` output
- What a XOR-obfuscated comparison looks like in disassembly
- How to reverse XOR obfuscation with Python
- Why XOR obfuscation is not real security (the key is in the binary)
