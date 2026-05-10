# Cybersecurity Course: Reverse Engineering, Binary Exploitation & Kernel Exploitation

A structured, hands-on curriculum for learning low-level security research. Designed for students with no prior experience in systems programming or security.

---

## Course Structure

```
cyber-course/
├── 01-foundations/
│   ├── 00-computer-architecture.md   ← START HERE
│   ├── 01-c-programming.md
│   ├── 02-assembly.md
│   ├── 03-linux-internals.md
│   ├── 04-memory-layout.md
│   └── 05-end-to-end-workflow.md     ← connects everything
├── 02-reverse-engineering/
│   ├── 01-static-analysis.md
│   ├── 02-dynamic-analysis.md
│   ├── 03-tools.md
│   └── 04-practice.md
├── 03-binary-exploitation/
│   ├── 01-stack-overflow.md
│   ├── 02-rop-chains.md
│   ├── 03-format-string.md
│   ├── 04-heap-exploitation.md
│   └── 05-mitigations.md
└── 04-kernel-exploitation/
    ├── 01-kernel-concepts.md
    ├── 02-environment-setup.md
    ├── 03-vulnerability-classes.md
    └── 04-exploitation-techniques.md
```

---

## Learning Path

Follow the modules in strict order — each one builds on the previous.

| Phase | Module | Key Outcome |
|---|---|---|
| **1** | Computer Architecture | Understand CPU, memory, bits/bytes, how a program runs |
| **1** | C Programming | Memory, pointers, unsafe functions, stack vs heap |
| **1** | x86-64 Assembly | Read disassembly, understand registers, calling convention |
| **1** | Linux Internals | Processes, syscalls, ELF format, permissions |
| **1** | Memory Layout | Virtual memory, stack frames, heap chunks, mitigations overview |
| **1** | End-to-End Workflow | C → compile → load → execute → overflow → exploit, all connected |
| **2** | Static Analysis | Analyze binaries with Ghidra, objdump, strings |
| **2** | Dynamic Analysis | Debug with GDB, trace with strace/ltrace, find offsets |
| **3** | Stack Overflow | Overflow buffers, control RIP, write first exploit |
| **3** | ROP Chains | Bypass NX, chain gadgets, leak libc with ret2libc |
| **3** | Format String | Read and write arbitrary memory via printf |
| **3** | Heap Exploitation | UAF, tcache poisoning, heap spray |
| **3** | Mitigations | Understand and bypass canary, NX, ASLR, PIE, RELRO |
| **4** | Kernel Concepts | Rings, cred struct, KASLR, SMEP/SMAP, syscalls |
| **4** | Environment Setup | QEMU, GDB kernel debugging, initramfs |
| **4** | Vulnerability Classes | Stack/heap overflow, UAF, race conditions in the kernel |
| **4** | Exploitation Techniques | ret2usr, kernel ROP, heap spray, iretq return |

---

## Prerequisites

- A Linux machine or VM (Ubuntu 22.04 recommended)
- Basic comfort with a terminal (cd, ls, cat, echo)
- No prior programming or security experience required

---

## Tools to Install (Ubuntu/Debian)

```bash
# Core
sudo apt install gcc gdb python3 python3-pip nasm binutils
sudo apt install build-essential flex bison libssl-dev libelf-dev bc cpio

# pwntools (exploit scripting)
pip3 install pwntools

# GDB plugin (highly recommended)
git clone https://github.com/pwndbg/pwndbg && cd pwndbg && ./setup.sh

# checksec (mitigation checker)
pip3 install checksec  # or: sudo apt install checksec

# ROPgadget (gadget finder)
pip3 install ROPgadget

# one_gadget (libc execve gadget finder)
gem install one_gadget

# Ghidra (decompiler — install Java 17 first)
sudo apt install openjdk-17-jdk
# Then download from https://ghidra-sre.org

# QEMU (kernel exploitation)
sudo apt install qemu-system-x86
```

---

## Recommended External Resources

| Resource | Focus |
|---|---|
| [pwn.college](https://pwn.college) | Best free structured course for RE + pwn |
| [exploit.education](https://exploit.education) | Vulnerable VMs to practice on |
| [ir0nstone gitbook](https://ir0nstone.gitbook.io/notes) | Binary exploitation notes |
| [lkmidas blog](https://lkmidas.github.io) | Kernel exploitation intro series |
| [LiveOverflow (YouTube)](https://youtube.com/@LiveOverflow) | Excellent video walkthroughs |
| [Branch Education — "How do CPUs Work?"](https://youtube.com/@BranchEducation) | 3D animated deep dive into CPU hardware and silicon |
| *Hacking: The Art of Exploitation* — Jon Erickson | Book: C, shellcode, exploitation from scratch |
| *The Linux Programming Interface* — Michael Kerrisk | Book: definitive Linux internals reference |
| *Computer Architecture: A Quantitative Approach* — Patterson & Hennessy | Book: deep dive into CPU design, pipelines, memory hierarchy, and instruction sets |
