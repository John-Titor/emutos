#!/bin/sh
#
# Build emutos using unhelpfully-located cross-development tools
#
make ELF=1 TOOLCHAIN_PREFIX=/Volumes/CTNG/m68k-unknown-elf/bin/m68k-unknown-elf- WITH_AES=0 tiny68k
