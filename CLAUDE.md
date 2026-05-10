# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a structured cybersecurity course delivered entirely as Markdown. It teaches low-level security research (reverse engineering, binary exploitation, kernel exploitation) to learners starting from no prior systems background. There are no build systems, dependencies, or tests — all content is documentation.

## Course Structure

Four sequential modules, each building on the previous:

| Module | Path | Content |
|--------|------|---------|
| Foundations | `01-foundations/` | Architecture, C, assembly, Linux internals, memory layout, end-to-end workflow |
| Reverse Engineering | `02-reverse-engineering/` | Static analysis (Ghidra, objdump) and dynamic analysis (GDB, strace/ltrace) |
| Binary Exploitation | `03-binary-exploitation/` | Stack overflows, ROP chains, format strings, heap exploitation, mitigations |
| Kernel Exploitation | `04-kernel-exploitation/` | Kernel concepts, QEMU environment, vulnerability classes, exploitation techniques |

Modules must be taken in order — each file assumes mastery of all prior content.

## Content Conventions

- Each file covers one focused topic with conceptual explanation followed by practical techniques
- Code examples are in C or x86-64 assembly; exploit scripts use Python with pwntools
- Security mitigations (ASLR, NX, PIE, canaries, SMEP/SMAP, KASLR) are introduced where first relevant, then revisited in dedicated files
- Kernel module (`04-kernel-exploitation/`) targets Linux kernel debugging via QEMU + GDB with a custom initramfs

## Tools Referenced Throughout

Students are expected to install these externally:

- **Core:** `gcc`, `gdb`, `python3`, `nasm`, `binutils`
- **Exploitation:** `pwntools`, `pwndbg`, `ROPgadget`, `one_gadget`, `checksec`
- **Reverse Engineering:** `Ghidra`, `objdump`, `strings`, `readelf`, `strace`, `ltrace`
- **Kernel:** `QEMU`, GDB kernel stubs, initramfs tooling
