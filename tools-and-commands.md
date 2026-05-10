# Tools & Commands Reference

Quick reference for the most important programs and commands used in reverse engineering, binary exploitation, and kernel exploitation on Kali Linux.

---

## Reverse Engineering

### file — Identify a Binary

```bash
file ./binary
# ELF 64-bit LSB executable, x86-64, dynamically linked, not stripped
```

### checksec — Show Security Mitigations

```bash
checksec --file=./binary
# Shows: Arch, RELRO, Stack canary, NX, PIE
```

### strings — Extract Readable Strings

```bash
strings ./binary                        # all strings >= 4 chars
strings -n 8 ./binary                   # minimum length 8
strings -t x ./binary                   # show file offset in hex
strings ./binary | grep -i "pass\|flag\|key"
```

### hexdump — Raw Byte View

```bash
hexdump -C ./binary | head -20          # hex + ASCII side by side
xxd ./binary | head -20                 # alternative with xxd
```

### readelf — ELF Structure Inspector

```bash
readelf -h ./binary                     # ELF header (entry point, arch)
readelf -S ./binary                     # section headers (.text, .data, .got, etc.)
readelf -l ./binary                     # program headers (segments)
readelf --syms ./binary                 # symbol table (function names)
readelf -d ./binary                     # dynamic section (shared libraries)
readelf -r ./binary                     # relocations (GOT entries)
```

### nm — Symbol Table

```bash
nm ./binary                             # all symbols
nm -D ./binary                          # dynamic symbols only
nm -D ./binary | grep " U "            # undefined (imported from libc)
nm -D /lib/x86_64-linux-gnu/libc.so.6 | grep system   # find system() offset in libc
```

### objdump — Disassembler

```bash
objdump -d -M intel ./binary            # disassemble .text (Intel syntax)
objdump -d -M intel -S ./binary         # interleave source (needs -g)
objdump -d -M intel ./binary | grep -A 30 "<main>:"   # specific function
objdump -s -j .rodata ./binary          # dump .rodata section contents
```

### Ghidra — Decompiler (GUI)

```bash
# Launch
./ghidraRun

# Workflow:
# 1. File -> Import File -> select binary
# 2. Auto-analyze: Yes
# 3. Symbol Tree (left) -> Functions -> main
# 4. Decompiler window (right) = C pseudocode

# Key shortcuts:
# L         rename variable/label
# ;         add comment
# Ctrl+L    go to address
# G         go to address (alternate)
# Ctrl+F    search for string or bytes
# Right-click -> References -> show all callers of a function
```

### strace — Trace System Calls

```bash
strace ./binary                         # trace all syscalls
strace -e trace=read,write,open ./binary  # filter specific calls
strace -s 200 ./binary                  # print up to 200 bytes of strings
strace -o trace.txt ./binary            # save output to file
strace -f ./binary                      # follow child processes
```

### ltrace — Trace Library Calls

```bash
ltrace ./binary                         # trace all library calls
ltrace -e strcmp ./binary               # only trace strcmp (great for crackmes)
```

### ldd — List Shared Library Dependencies

```bash
ldd ./binary                            # show linked libraries and load addresses
ldd ./binary | grep libc                # find libc path
```

---

## Binary Exploitation

### GDB + pwndbg

```bash
# Start GDB
gdb ./binary
gdb -q ./binary                         # quiet mode (no banner)
gdb --args ./binary arg1 arg2           # pass arguments

# Running
run                                     # run the program
run < input.txt                         # stdin from file
continue  (c)                           # continue after breakpoint
kill                                    # kill the process
quit  (q)                               # exit GDB

# Breakpoints
break main                              # break at main()
break *0x401234                         # break at address
break foo                               # break at function foo
info breakpoints                        # list breakpoints
delete 1                                # delete breakpoint #1

# Stepping
next  (n)                               # next source line (step over)
step  (s)                               # next source line (step into)
nexti (ni)                              # next instruction (step over)
stepi (si)                              # next instruction (step into)
finish                                  # run until function returns

# Registers
info registers                          # show all registers
print $rax                              # print register value
print/x $rax                            # print in hex
set $rax = 0x41                         # modify register

# Memory examination: x/[count][format][size] address
x/20gx $rsp                             # 20 qwords from RSP (hex)
x/32bx $rsp                             # 32 bytes from RSP (hex)
x/s 0x402010                            # null-terminated string at address
x/10i $rip                              # disassemble 10 instructions from RIP
x/gx $rbp+8                             # read return address

# Stack and frame
info frame                              # current frame info
backtrace                               # call stack
set disassembly-flavor intel            # use Intel syntax

# pwndbg-specific
vmmap                                   # full virtual memory map
heap                                    # show heap chunks
bins                                    # show free bins (tcache, fastbin, etc.)
vis_heap_chunks                         # visual heap layout
tcache                                  # show tcache contents
stack 30                                # show top 30 stack entries
telescope $rsp 20                       # smart-dereference 20 values from RSP
cyclic 100                              # generate de Bruijn pattern
cyclic -l 0x6161616b                    # find offset from crashed RIP value
context                                 # show registers + stack + disasm
```

### pwntools — Exploit Scripting (Python)

```python
from pwn import *

# Connect to process or remote
p = process('./binary')
p = remote('host', 1337)

# Send / receive
p.send(b'data')                         # send bytes (no newline)
p.sendline(b'data')                     # send bytes + newline
p.sendafter(b'prompt', b'data')         # send after seeing prompt
p.sendlineafter(b'prompt', b'data')

p.recv(n)                               # receive n bytes
p.recvline()                            # receive until newline
p.recvuntil(b'marker')                  # receive until marker string
p.interactive()                         # interactive shell mode

# ELF inspection
elf = ELF('./binary')
elf.symbols['win']                      # address of win()
elf.plt['puts']                         # PLT stub for puts
elf.got['puts']                         # GOT entry for puts
elf.bss()                               # start of .bss section

# Packing / unpacking
p64(0xdeadbeef)                         # pack as 64-bit little-endian
p32(0xdeadbeef)                         # pack as 32-bit little-endian
u64(b'\xef\xbe\xad\xde\x00\x00\x00\x00')  # unpack 64-bit

# ROP
rop = ROP(elf)
pop_rdi = rop.find_gadget(['pop rdi', 'ret'])[0]
ret     = rop.find_gadget(['ret'])[0]
rop.dump()                              # print chain
rop.chain()                             # raw bytes

# Shellcode
context.arch = 'amd64'
context.os   = 'linux'
shellcode = asm(shellcraft.sh())        # execve /bin/sh shellcode

# Cyclic patterns
cyclic(100)                             # generate 100-byte pattern
cyclic_find(0x6161616b)                 # find offset from value

# Format string
fmtstr_payload(offset, {addr: value})   # build format string payload

# Libc
libc = ELF('/lib/x86_64-linux-gnu/libc.so.6')
libc.symbols['system']                  # offset of system()
next(libc.search(b'/bin/sh'))           # offset of /bin/sh string
```

### ROPgadget — Find ROP Gadgets

```bash
pip3 install ROPgadget

ROPgadget --binary ./binary             # list all gadgets
ROPgadget --binary ./binary | grep "pop rdi"
ROPgadget --binary ./binary | grep "pop rsi"
ROPgadget --binary ./binary --string "/bin/sh"   # search for string
ROPgadget --binary /lib/x86_64-linux-gnu/libc.so.6 | grep "pop rdi ; ret"
```

### ropper — Alternative Gadget Finder

```bash
pip3 install ropper

ropper -f ./binary
ropper -f ./binary --search "pop rdi"
ropper -f ./binary --search "pop rdi ; ret"
```

### one_gadget — Find execve Gadgets in libc

```bash
gem install one_gadget

one_gadget /lib/x86_64-linux-gnu/libc.so.6
# Lists addresses where execve("/bin/sh") is called unconditionally
# (if register constraints at that point are met)
```

### patchelf — Change Binary's Linked libc

```bash
sudo apt install patchelf

patchelf --set-interpreter ./ld.so ./binary
patchelf --replace-needed libc.so.6 ./libc.so ./binary
# Used in CTFs to match a specific libc version
```

---

## Kernel Exploitation

### QEMU — Boot a Kernel VM

```bash
# Minimal boot
qemu-system-x86_64 \
    -kernel bzImage \
    -initrd rootfs.cpio.gz \
    -append "console=ttyS0 nokaslr quiet" \
    -nographic \
    -m 256M

# With GDB server (pause at boot)
qemu-system-x86_64 \
    -kernel bzImage \
    -initrd rootfs.cpio.gz \
    -append "console=ttyS0 nokaslr quiet" \
    -nographic -m 256M \
    -s -S                               # -s = gdbserver :1234, -S = pause

# With mitigations disabled
-append "console=ttyS0 nokaslr nopti nosmap nosmep quiet"
-cpu qemu64

# With mitigations enabled
-append "console=ttyS0 kaslr quiet"
-cpu qemu64,+smep,+smap
```

### GDB — Kernel Debugging

```bash
gdb vmlinux                             # load kernel with symbols

# Inside GDB
target remote :1234                     # connect to QEMU gdbserver
c                                       # continue (let kernel boot)
Ctrl+C                                  # pause kernel

break sys_read                          # break on read syscall
break commit_creds                      # break when creds change
break *0xffffffff81082200               # break at kernel address

x/20gx $rsp                            # kernel stack
x/10i $rip                             # kernel instructions at RIP
x/s 0xffffffff81000000                  # read kernel string

source vmlinux-gdb.py                   # load Linux GDB helpers
lx-ps                                   # list all processes
lx-dmesg                                # read kernel log
```

### initramfs — Pack/Unpack Root Filesystem

```bash
# Unpack (extract CTF filesystem)
mkdir extracted && cd extracted
cp ../rootfs.cpio.gz .
gunzip rootfs.cpio.gz
cpio -idv < rootfs.cpio

# Repack (after adding exploit binary)
find . | cpio -o --format=newc | gzip > ../rootfs.cpio.gz
```

### Kernel Information Gathering

```bash
# Kernel function addresses (requires root or kptr_restrict=0)
cat /proc/kallsyms | grep "commit_creds\|prepare_kernel_cred"
cat /proc/kallsyms | grep " T _text"   # kernel base address

# Allow non-root to read kallsyms (QEMU practice only)
echo 0 > /proc/sys/kernel/kptr_restrict

# Kernel version and build info
uname -r                                # kernel version
cat /proc/version
dmesg | head -5

# Mitigations status
cat /proc/sys/kernel/randomize_va_space # ASLR level (0/1/2)
cat /proc/cpuinfo | grep -E "smep|smap" # CPU feature flags
cat /proc/cmdline                       # boot parameters (kaslr/nokaslr)
cat /sys/kernel/debug/x86/cpu_mitigations  # Meltdown/Spectre status

# Heap (SLUB allocator)
cat /proc/slabinfo                      # slab cache stats
sudo slabtop                            # live slab view
```

### Kernel Module Commands

```bash
# Load / unload modules
sudo insmod ./challenge.ko              # load module
sudo rmmod challenge                    # unload module
lsmod                                   # list loaded modules
modinfo ./challenge.ko                  # show module metadata

# Build a kernel module
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
make -C /lib/modules/$(uname -r)/build M=$(pwd) clean

# Kernel log
dmesg                                   # full kernel log
dmesg | tail -20                        # recent messages
dmesg | grep -i "oops\|panic\|bug\|error"
dmesg -C                                # clear log
```

### Compiling Exploits for the Kernel Environment

```bash
# Static binary (runs inside minimal initramfs)
gcc -static -o exploit exploit.c

# With no PIE and no stack protector (for practice)
gcc -static -fno-stack-protector -no-pie -o exploit exploit.c

# Cross-compile for a different kernel arch (if needed)
sudo apt install gcc-aarch64-linux-gnu
aarch64-linux-gnu-gcc -static -o exploit exploit.c
```

### Useful /proc and /sys Paths

```bash
/proc/kallsyms              # kernel symbol addresses
/proc/cmdline               # kernel boot parameters
/proc/cpuinfo               # CPU features (smep, smap, etc.)
/proc/slabinfo              # SLUB allocator stats
/proc/self/maps             # current process memory map
/proc/<pid>/maps            # memory map of any process
/sys/kernel/debug/          # kernel debug filesystem (debugfs)
/dev/ptmx                   # open for tty_struct heap spray
/proc/self/mem              # read/write process memory directly
```

---

## One-Line Reference Card

| Task | Command |
|---|---|
| Identify binary | `file ./binary` |
| Check mitigations | `checksec --file=./binary` |
| Extract strings | `strings -n 8 ./binary` |
| Disassemble | `objdump -d -M intel ./binary` |
| Find function offset in libc | `nm -D /lib/x86_64-linux-gnu/libc.so.6 \| grep system` |
| Find ROP gadgets | `ROPgadget --binary ./binary \| grep "pop rdi"` |
| Find one_gadget | `one_gadget /lib/.../libc.so.6` |
| Find overflow offset | `cyclic 200` in pwndbg, then `cyclic -l <RIP value>` |
| Read return address in GDB | `x/gx $rbp+8` |
| Show memory map in GDB | `vmmap` (pwndbg) |
| Show heap in GDB | `vis_heap_chunks` (pwndbg) |
| Boot kernel in QEMU | `qemu-system-x86_64 -kernel bzImage -initrd rootfs.cpio.gz ...` |
| Attach GDB to QEMU | `gdb vmlinux` then `target remote :1234` |
| Find kernel symbols | `cat /proc/kallsyms \| grep commit_creds` |
| Load kernel module | `sudo insmod ./challenge.ko` |
| Unpack initramfs | `gunzip rootfs.cpio.gz && cpio -idv < rootfs.cpio` |
| Repack initramfs | `find . \| cpio -o --format=newc \| gzip > ../rootfs.cpio.gz` |
