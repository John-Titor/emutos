/*
 * pt68k5boot.S
 *
 * Copyright (C) 2023 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "asmdefs.h"

#ifdef MACHINE_PT68K5

    .set    hd_unit,0xfff               // MONK5 current fixed disk unit number

start:
    // move the user stack away from the potential load area
    lea     start(pc),sp                // nothing just below us in RAM
    subq.l  #8,sp                       // ... plus some padding for paranoia's sake

    // call the disk loader
    clr.l   d0
    move.b  hd_unit,d0
    move.l  d0,-(sp)
    move.l  a2,-(sp)                    // MONK5 current fixed disk base address
    bsr     _boot_main                  // go boot something
    trap    #0                          // failed, back to monitor

// void putc(int c)
//
    .globl _outc
_outc:
    move.w  4(sp),d0
    cmp.b   #'\n',d0
    bne     1f
    move.b  #'\r',d0
    trap    #2
    move.w  4(sp),d0
1:
    trap    #2
    rts

// int probe(uint32_t address)
//
    .globl _probe
_probe:
    move.l  4(sp),d0
    move.l  d0,a0
    move.l  d0,(a0)
    not     d0
    move.l  d0,4(a0)
    not     d0
    cmp.l   (a0),d0
    bne     1f
    not     d0
    cmp.l   4(a0),d0
    bne     1f
    move.w  1,d0
    rts
1:
    clr.w   d0
    rts

// uint32_t trap12(uint8_t op, uint8_t arg1, uint8_t arg2, void *addr)
//
    .globl  _trap12
_trap12:
    move.l  d7,-(sp)                    // save d7
    move.l  8(sp),d7                    // get opcode
    move.l  12(sp),d0                   // get args
    move.l  16(sp),d1
    move.l  20(sp),a1
    trap    #12                         // call MONK5
    move.l  d7,d0                       // return value in d7
    move.l  (sp)+,d7                    // restore d7
    rts

// void jump_to_loaded_os(uint32_t entrypoint)
//
    .globl _jump_to_loaded_os
_jump_to_loaded_os:
    move.l  4(sp),a1                    // get entrypoint
    lea     am_super(pc), a0            // continuation address
    move.l  a0,0xbc                     // trap #15 vector
    trap    #15                         // go to supervisor mode
am_super:
    moveq   #8,d0                       // invalidate and disable the instruction cache
    movec   d0,cacr
    jmp     (a1)                        // call entrypoint

#endif /* MACHINE_PT68K5 */
