# Dynamic Analysis

Dynamic analysis means running a binary and observing its behavior in real time. You interact with the program, pause it, inspect memory and registers, and trace execution.

---

## Why Dynamic Analysis?

- Reveals runtime values that static analysis cannot (decrypted strings, resolved addresses)
- Essential for understanding how data flows through a program
- Required to verify exploits and measure offsets precisely
- Shows actual memory state: heap contents, stack layout, register values

---

## GDB: The Core Tool

GDB (GNU Debugger) is the standard Linux debugger. It lets you pause execution, inspect/modify memory and registers, and step instruction-by-instruction.

### Install GDB + pwndbg

pwndbg is a plugin that makes GDB far more useful for exploit development.

```bash
# Install pwndbg (recommended)
git clone https://github.com/pwndbg/pwndbg
cd pwndbg && ./setup.sh

# Alternative: peda
git clone https://github.com/longld/peda.git ~/.peda
echo "source ~/.peda/peda.py" >> ~/.gdbinit
```

---

## GDB Fundamentals

### Starting GDB

```bash
gdb ./binary              # load binary
gdb ./binary core         # load binary with a core dump
gdb -p <pid>              # attach to a running process
gdb --args ./binary arg1 arg2   # pass arguments
```

### Running the Program

```
(gdb) run                          # run with no arguments
(gdb) run arg1 arg2                # run with arguments
(gdb) run < input.txt              # stdin from file
(gdb) run <<< $(python3 -c "print('A'*100)")   # python-generated input
(gdb) continue  (or c)             # continue execution after a pause
(gdb) kill                         # kill the running process
(gdb) quit  (or q)                 # exit GDB
```

---

## Breakpoints

A breakpoint pauses execution when the CPU reaches a specific address or function.

```
(gdb) break main               # break at start of main()
(gdb) break *0x401234          # break at specific address
(gdb) break foo                # break at function foo
(gdb) break *main+42           # break 42 bytes into main

(gdb) info breakpoints         # list all breakpoints
(gdb) delete 1                 # delete breakpoint #1
(gdb) delete                   # delete all breakpoints
(gdb) disable 2                # disable breakpoint #2
(gdb) enable 2                 # re-enable it
```

### Conditional Breakpoints

```
(gdb) break *0x401234 if $rax == 0    # only break if rax == 0
(gdb) break foo if i == 10            # only break in foo when i == 10
```

### Watchpoints

Stop execution when a memory location is read or written.

```
(gdb) watch *0x7fff1234          # break when address is written
(gdb) rwatch *0x7fff1234         # break when address is read
(gdb) awatch *0x7fff1234         # break on read or write
```

---

## Stepping Through Code

```
(gdb) next      (n)   # execute one source line, step OVER function calls
(gdb) step      (s)   # execute one source line, step INTO function calls
(gdb) nexti     (ni)  # execute one INSTRUCTION, step over calls
(gdb) stepi     (si)  # execute one INSTRUCTION, step into calls
(gdb) finish          # run until current function returns
(gdb) until *0x401234 # run until reaching this address
```

For exploit development, you almost always want `ni` and `si` — you're working at the instruction level.

---

## Examining Registers

```
(gdb) info registers          # show all registers
(gdb) info registers rax rbp  # show specific registers
(gdb) print $rax              # print value of rax
(gdb) print/x $rax            # print in hex
(gdb) set $rax = 0x41         # modify a register
```

With pwndbg, registers are shown automatically on every stop with color-coded change highlighting.

---

## Examining Memory

The `x` (examine) command is essential. Format: `x/[count][format][size] address`

```
Format letters:
  x = hex
  d = decimal
  s = string (null-terminated)
  i = instruction (disassemble)
  c = character

Size letters:
  b = byte (1 byte)
  h = halfword (2 bytes)
  w = word (4 bytes)
  g = giant/qword (8 bytes)
```

```
(gdb) x/20gx $rsp             # 20 qwords (8 bytes each) from RSP in hex
(gdb) x/32bx $rsp             # 32 bytes from RSP in hex
(gdb) x/s 0x402010            # read null-terminated string at address
(gdb) x/10i $rip              # disassemble 10 instructions from RIP
(gdb) x/gx $rbp+8             # read return address
```

### Memory Layout Inspection (pwndbg)

```
(gdb) vmmap                   # show full virtual memory map
(gdb) heap                    # show heap chunks
(gdb) stack 30                # show top 30 entries of the stack
(gdb) telescope $rsp 20       # smart-dereference 20 values from RSP
```

---

## Stack Analysis

### Find the Return Address

```
(gdb) info frame              # show current frame info, including return address
(gdb) x/gx $rbp+8            # return address is always at [rbp+8]
(gdb) backtrace               # show call stack
```

### Visualize the Stack (pwndbg)

When stopped at a breakpoint, pwndbg's `context` output shows:

```
pwndbg> context stack

STACK
00:0000│ rsp 0x7fffffffe4c0 ◂— 'AAAAAAAA...'
01:0008│     0x7fffffffe4c8 ◂— 0x4141414141414141
...
08:0040│     0x7fffffffe500 —▸ 0x7fffffffe500 ◂— (saved rbp)
09:0048│     0x7fffffffe508 —▸ 0x401189 (return address → main+...)
```

---

## Disassembly in GDB

```
(gdb) disas main              # disassemble main function
(gdb) disas 0x401156          # disassemble at address
(gdb) disas 0x401156, +50     # disassemble 50 bytes from address
(gdb) set disassembly-flavor intel   # use Intel syntax (recommended)
```

---

## Modifying Execution

This is useful for bypassing checks during analysis.

```
(gdb) set $rip = 0x401234         # jump to a different address
(gdb) set $rax = 1                # fake a return value
(gdb) set *(int*)0x7fff1234 = 99  # write to memory
(gdb) jump *0x401234              # jump to address (continues execution)
```

---

## strace and ltrace

### strace: Trace System Calls

```bash
strace ./binary
strace -e trace=read,write,open ./binary   # filter specific calls
strace -o trace.txt ./binary               # save output to file
strace -f ./binary                         # follow forks/threads
strace -s 200 ./binary                     # print up to 200 bytes of strings
```

Example output:
```
execve("./binary", ["./binary"], 0x...) = 0
brk(NULL) = 0x55a1b4600000
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
read(3, "\x7fELF\x02\x01\x01"..., 832) = 832
write(1, "Enter password: ", 16) = 16
read(0, "testpass\n", 256) = 9
write(1, "Wrong!\n", 7) = 7
exit_group(1) = ?
```

### ltrace: Trace Library Calls

```bash
ltrace ./binary
ltrace -e strcmp ./binary    # only trace strcmp calls
```

Example output:
```
puts("Enter password: ")        = 17
fgets("testpass\n", 64, stdin)  = 0x...
strcmp("testpass", "s3cr3t")    = 1     <- comparison visible in plaintext!
puts("Wrong!")                  = 8
```

`ltrace` is extremely powerful for crackmes — it often shows you the comparison directly.

---

## Finding the Buffer Overflow Offset

When you overflow a buffer, you need to know exactly how many bytes reach the return address. Use cyclic patterns.

### Method 1: pwntools cyclic (recommended)

```bash
# In your terminal
python3 -c "from pwn import *; print(cyclic(200))"
# Generates a de Bruijn sequence: aaaabaaacaaadaaae...
```

```
# In GDB
(gdb) run
# paste the cyclic pattern as input
# program crashes with:  RIP: 0x6161616b ("kaaa")

# Find the offset
python3 -c "from pwn import *; print(cyclic_find(0x6161616b))"
# Output: 44   <- 44 bytes from buffer start to return address
```

### Method 2: Pattern from pwndbg

```
pwndbg> cyclic 200             # generate pattern
pwndbg> cyclic -l 0x6161616b  # find offset from crashed RIP value
```

### Method 3: Manual Calculation

```bash
# From Ghidra/objdump, find:
# 1. Buffer start: sub rsp, 0x50 -> buffer is at [rbp - 0x50]
# 2. Return address: always at [rbp + 0x8]
# Offset = 0x50 (buf to rbp) + 0x8 (saved rbp) = 0x58 = 88 bytes

# Verify in GDB:
(gdb) break *vuln_function+5
(gdb) run
(gdb) x/gx $rbp+8             # print return address
(gdb) p $rbp - (address of buf start)   # manual calculation
```

---

## Practical Debugging Session

```bash
$ gdb -q ./vuln
Reading symbols from ./vuln... (no debugging symbols found)

pwndbg> break main
Breakpoint 1 at 0x401156

pwndbg> run
Breakpoint 1, 0x0000000000401156 in main ()

pwndbg> disas main
   0x0000000000401156 <+0>:  push   rbp
   0x0000000000401157 <+1>:  mov    rbp,rsp
   0x000000000040115a <+4>:  sub    rsp,0x40
   ...
   0x0000000000401163 <+13>: call   0x401040 <gets@plt>

pwndbg> break *0x401163       # break before gets()
pwndbg> continue

# GDB stops before gets is called
pwndbg> x/gx $rbp+8           # look at current return address
0x7fffffffe4f8: 0x00007ffff7de2083

pwndbg> ni                     # step into gets (it will wait for input)
# type: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA (50 A's)

pwndbg> x/gx $rbp+8
0x7fffffffe4f8: 0x4141414141414141   # return address overwritten!
```

---

## Exercises

1. Compile a vulnerable program with `gets()`. Run it in GDB. Use a cyclic pattern to find the exact offset to the return address.
2. Use `ltrace` on a crackme. Does it reveal the comparison?
3. Set a breakpoint before a `strcmp` call. Use `x/s $rdi` and `x/s $rsi` to read both strings being compared.
4. Use `strace` on `/bin/ls`. How many `openat` syscalls does it make? What files does it open?
5. In GDB: break at `main`, use `vmmap` to find the base address of libc. Then use `p system` to find the address of `system`. Calculate the offset of `system` from the libc base.
