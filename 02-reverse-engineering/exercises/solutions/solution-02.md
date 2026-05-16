# Solution 02 — XOR Crackme

---

## Step 1 — Confirm strings does not help

```bash
strings crackme-02
```

You will see standard library names, format strings (`"Key: "`, `"Correct!"`, `"Wrong."`), but no obvious key. The bytes `0xda 0xcb 0xd9 0xd9 0x9b` are not printable ASCII, so `strings` skips them entirely.

---

## Step 2 — Find the check function with objdump

```bash
objdump -d -M intel crackme-02
```

Look for a function named `check` (it will appear as `<check>`):

```bash
objdump -d -M intel crackme-02 | grep -A 50 "<check>"
```

Expected output (addresses will differ on your machine):

```nasm
0000000000001169 <check>:
    1169: push   rbp
    116a: mov    rbp, rsp
    116d: sub    rsp, 0x20
    1171: mov    QWORD PTR [rbp-0x18], rdi      ; save input pointer
    ...
    ; loading the enc array onto the stack:
    118a: mov    BYTE PTR [rbp-0x6], 0xda
    118f: mov    BYTE PTR [rbp-0x5], 0xcb
    1194: mov    BYTE PTR [rbp-0x4], 0xd9
    1199: mov    BYTE PTR [rbp-0x3], 0xd9
    119e: mov    BYTE PTR [rbp-0x2], 0x9b
    ...
    ; the XOR comparison loop:
    11c5: movzx  eax, BYTE PTR [rax]            ; load input[i]
    11c8: xor    eax, 0xaa                      ; input[i] ^ 0xaa
    11cb: cmp    al,  BYTE PTR [rdx]            ; compare to enc[i]
    11ce: je     11d4 <check+0x6b>
    11d0: mov    eax, 0x0                       ; not equal: return 0
    11d5: ...
```

**What to notice:**
- The bytes `0xda, 0xcb, 0xd9, 0xd9, 0x9b` are loaded as the `enc` array
- The `xor eax, 0xaa` instruction reveals the XOR constant is `0xaa`
- The comparison checks `input[i] ^ 0xaa == enc[i]`

---

## Step 3 — Understand the XOR math

The check is:

```
input[i] ^ 0xaa == enc[i]
```

XOR is its own inverse — XOR both sides by `0xaa`:

```
input[i] == enc[i] ^ 0xaa
```

So the correct key byte at position `i` is `enc[i] ^ 0xaa`.

---

## Step 4 — Recover the key with Python

```python
enc = [0xda, 0xcb, 0xd9, 0xd9, 0x9b]
key = ''.join(chr(b ^ 0xaa) for b in enc)
print(key)
```

Step by step:

```
0xda ^ 0xaa = 0x70 = 112 = 'p'
0xcb ^ 0xaa = 0x61 =  97 = 'a'
0xd9 ^ 0xaa = 0x73 = 115 = 's'
0xd9 ^ 0xaa = 0x73 = 115 = 's'
0x9b ^ 0xaa = 0x31 =  49 = '1'
```

Key: **`pass1`**

---

## Step 5 — (Optional) How this looks in Ghidra

Load `crackme-02` in Ghidra, auto-analyze, then open the `check` function in the Decompiler window. It will show something like:

```c
int check(char *input) {
    undefined enc[5];
    enc[0] = 0xda;
    enc[1] = 0xcb;
    enc[2] = 0xd9;
    enc[3] = 0xd9;
    enc[4] = 0x9b;

    if (strlen(input) != 5) return 0;
    for (int i = 0; i < 5; i++) {
        if ((input[i] ^ 0xaa) != enc[i]) return 0;
    }
    return 1;
}
```

The XOR constant and the encrypted bytes are immediately visible.

---

## Step 6 — Verify

```bash
echo "pass1" | ./crackme-02
```

Expected output:

```
Key: Correct!
```

---

## What just happened

The author stored the key XOR-ed with `0xaa` so it would not appear in a `strings` scan. The five bytes `0xda 0xcb 0xd9 0xd9 0x9b` are not printable ASCII, so the scanner skips them.

However, the XOR constant `0xaa` and the encrypted bytes are both visible in the binary. Any disassembler reveals them. XOR obfuscation delays a casual scan — it does not resist a reverse engineer with `objdump`.
