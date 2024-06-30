/*
 * ROM bootloader for EmuTOS on MEN B5
 *
 * This loader and pagetable occupy the first 8K at 0x4000, with
 * the EmuTOS binary immediately following.
 *
 * To load EmuTOS, the loader is copied to RAM_PHYS + 0x4000, then
 * the MMU enabled. The pagetable maps RAM to 0, meaning that the CPU
 * sees the RAM copy of the loader where it was in ROM and continues
 * execution uninterrupted.
 *
 * The EmuTOS binary in ROM is mapped to the normal EmuTOS ROM
 * location at 0x00e0_0000, so once the MMU is enabled EmuTOS is
 * started by jumping to the entrypoint at the start of the image.
 */

/* memory layout */
    .equ    RAM_PHYS,       0x28000000
    .equ    LOADER_BASE,    0x04000000
    .equ    LOADER_SIZE,    0x00004000
    .equ    ROM_PHYS,       (LOADER_BASE + LOADER_SIZE)

/* relevant TOS variables */
    .equ    memctrl,        0x424
    .equ    resvalid,       0x426
    .equ    phystop,        0x42e
    .equ    ramtop,         0x5a4
    .equ    memvalid,       0x420
    .equ    memval2,        0x43a
    .equ    memval3,        0x51a
    .equ    ramvalid,       0x5a8
    .equ    warm_magic,     0x6fc

/* MENMON function vectors */
    .equ    v_put_char      (0x10880+0x00)  /* d0.b: char to print */
    .equ    v_chk_char      (0x10880+0x04)  /* d0.b = char read or -1 if none ready */
    .equ    v_printf        (0x10880+0x08)  /* d0.l: format string, args on stack */
    .equ    v_ee_read       (0x10880+0x0c)  /* d0.l: buffer, d1.w: index, length.w on stack */
    .equ    v_ee_write      (0x10880+0x10)  /* d0.l: buffer, d1.w: index, length.w on stack */
    .equ    v_initcons      (0x10880+0x14)  /* d0.w: 0=interrupt driven, 1=polled */
    .equ    v_termcons      (0x10880+0x18)  /* no args */
    .equ    v_enter_mm      (0x10880+0x28)  /* no args */
    .equ    v_get_rsr       (0x10880+0x30)  /* d0.w = 68360 reset status register */

.macro putc c
    move.l  v_put_char,%a0
    move.b  \c,%d0
    jsr     (%a0)
.endm

.macro printf str
    move.l  v_printf,%a0
    move.l  \str,%d0
    jsr     (%a0)
.endm

/* wrapper for function label */
.macro TSYM name
    .type   \name,"function"
\name:
.endm

/* wrapper for object label */
.macro DSYM name
    .type   \name,"object"
\name:
.endm

    .org    0x0
    .text
    .globl  start
TSYM start
    printf  msg_boot1
    bra     .

    /* set up transparent translation registers for I/O region */
    move.l  #0x0200a040,%d0         /* 0x02xx_xxxx uncacheable, serialized */
    movec   %d0,%dtt0
    move.q  #0,%d0
    movec   %d0,%dtt1
    movec   %d0,%itt0
    movec   %d0,%itt1

    /* duplicate the loader in RAM */
    lea     vectors,%a0             /* source */
    lea     vectors+RAM_PHYS,%a1    /* destination */
    lea     end,%a2                 /* source end */
1:
    move.l  (%a0)+,(%a1)+
    cmpl    %a0,%a2
    bne     1b

    /* reset, configure, and enable the MMU */
    pflusha
    lea     root_table,%a0
    movec   %a0,%urp
    movec   %a0,%srp
    move.w  #0xc000,%d0
    movec   %d0,%tc                 /* should now be executing from RAM */

    move.l  #0x8000,%sp             /* move stack to RAM */
    mputs   msg_boot2

    /* init TOS variables */
    moveq   #0,%d0
    move.l  %d0,memctrl             /* non-conforming memory setup */
    move.l  %d0,resvalid            /* prevent reset vector being called */
    move.l  %d0,ramtop              /* no TT memory */
    move.l  %d0,warm_magic          /* this is a cold boot */
    move.l  #0x00c00000,phystop     /* top of memory (XXX 8M boards) */
    move.l  #0x752019f3,memvalid    /* magic numbers to validate memory config */
    move.l  #0x237698aa,memval2
    move.l  #0x5555aaaa,memval3
    move.l  #0x1357bd13,ramvalid

    move.l  #0x00e00000,%a0         /* EmuTOS entrypoint */
    jmp     %a0@

DSYM err_exception1
    .asciz "Exception "
DSYM err_exception2
    .asciz " @ "
DSYM msg_boot1
    .asciz "MMU setup..."
DSYM msg_boot2
    .asciz "done.\r\n"
DSYM msg_newline
    .asciz "\r\n"

    .p2align 2
DSYM end                /* nothing after this available with MMU on */
    .dc.l   0           /* keep end and rom_page_tables distinct */

/*
 * Pagetables.
 *
 * Since memory layout is fixed, we can compute these statically and leave
 * them in the ROM. If performance is terrible, they could be moved to RAM.
 * To minimise pagetable size and make the most of the ATC, we use 8K pages.
 *
 * Since we're only supporting a board with 20M, we only use one pointer table.
 *
 * We need:
 *  - a complete root table; 128 entries, 512B
 *  - one pointer table mapping 0x0000_0000-0x02ff_ffff, 128 entries, 512B
 *  - 56 page tables mapping 14M of RAM from 0x2800_0000 to 0x0000_0000-0x00df_ffff, 56 x 32 entries, 7168B
 *  - two page tables mapping 488K of ROM from 0x0400_4000 to 0x00e0_0000, 61 entries, 244B
 *    (this loader consumes 2 pages of the first 512K of the ROM)
 *  - three indirect entries at the end of ROM space mapping any 8k to 0x00e7_[a,c,e]000, 3 entries, 12B
 *  - 24 page tables mapping 6M of RAM from 0x028e0_0000 to 0x0100_0000-0x015f_ffff, 24 * 32 entries, 3072B
 *
 * The root and pointer tables must be 512B aligned. Page tables must be 128B
 * aligned. Smaller tables are placed first to maximise space for the loader
 * code.
 *
 * Pointer and page table entries are marked used, and writable page table
 * entries marked modified to prevent the MMU from trying to update these bits
 * (040 UM fig 3-10).
 *
 * The indirect entries at the end of the ROM aperture can be used as a window to
 * read/write any physical range. This is intended as a cheap way of reading any
 * part of the entire ROM without having to spend more memory on pagetables. The
 * indirections point to RAM_PHYS+0x5x; assuming vectors are based at RAM_PHYS
 * this is the vector address for the reserved exceptions 20-22. Since all of RAM
 * is mapped writeback-cachable, we attempt to placate the 060 table walker by
 * having these PTEs in a location where they are very unlikely to ever be resident
 * in the cache for any reason.
 *
 * note: don't use '|' for bitwise-or, as it starts a comment; use + as no fields
 *       should overlap.
 */

    .equ    PDT_INVALID,    (0<<0)
    .equ    PDT_INDIRECT,   (2<<0)
    .equ    PDT_RESIDENT,   (3<<0)
    .equ    W_READONLY,     (1<<2)
    .equ    U_USED,         (1<<3)
    .equ    M_MODIFIED,     (1<<4)
    .equ    CM_WRITETHROUGH,(0<<5)
    .equ    CM_COPYBACK,    (1<<5)
    .equ    CM_SERIALIZED,  (2<<5)
    .equ    CM_UNCACHED,    (3<<5)
    .equ    S_SUPERVISOR,   (1<<7)

/* page table mapping RAM */
.macro mk_ram_page_table    table_index, pgi=0
    .dc.l   RAM_PHYS + (\table_index << 18) + (\pgi << 13) + CM_COPYBACK + M_MODIFIED + U_USED + PDT_RESIDENT
.if 31 - \pgi
    mk_ram_page_table \table_index, (\pgi + 1)
.endif
.endm

/* page table table mapping ROM, note option for reduced PTE count */
.macro mk_rom_page_table    table_index, pgi=0, count=31
    .dc.l   ROM_PHYS + (\table_index << 18) + (\pgi << 13) + CM_WRITETHROUGH + U_USED + W_READONLY + PDT_RESIDENT
.if \count - \pgi
    mk_rom_page_table \table_index, (\pgi + 1), \count
.endif
.endm

/* pointer table entry referencing a page table */
.macro mk_pointer_entry     page_base=ram_page_tables, page_index=0
    .dc.l   \page_base + (\page_index << 7) + U_USED + PDT_RESIDENT
.endm

    .p2align 7
DSYM rom_page_tables
    mk_rom_page_table   table_index=0
    mk_rom_page_table   table_index=1,count=28
    .dc.l   RAM_PHYS + 0x50 + PDT_INDIRECT
    .dc.l   RAM_PHYS + 0x54 + PDT_INDIRECT
    .dc.l   RAM_PHYS + 0x58 + PDT_INDIRECT

    .p2align 7
DSYM ram_page_tables
    mk_ram_page_table   table_index=0
    mk_ram_page_table   table_index=1
    mk_ram_page_table   table_index=2
    mk_ram_page_table   table_index=3
    mk_ram_page_table   table_index=4
    mk_ram_page_table   table_index=5
    mk_ram_page_table   table_index=6
    mk_ram_page_table   table_index=7
    mk_ram_page_table   table_index=8
    mk_ram_page_table   table_index=9
    mk_ram_page_table   table_index=10
    mk_ram_page_table   table_index=11
    mk_ram_page_table   table_index=12
    mk_ram_page_table   table_index=13
    mk_ram_page_table   table_index=14
    mk_ram_page_table   table_index=15
    mk_ram_page_table   table_index=16
    mk_ram_page_table   table_index=17
    mk_ram_page_table   table_index=18
    mk_ram_page_table   table_index=19
    mk_ram_page_table   table_index=20
    mk_ram_page_table   table_index=21
    mk_ram_page_table   table_index=22
    mk_ram_page_table   table_index=23
    mk_ram_page_table   table_index=24
    mk_ram_page_table   table_index=25
    mk_ram_page_table   table_index=26
    mk_ram_page_table   table_index=27
    mk_ram_page_table   table_index=28
    mk_ram_page_table   table_index=29
    mk_ram_page_table   table_index=30
    mk_ram_page_table   table_index=31
    mk_ram_page_table   table_index=32
    mk_ram_page_table   table_index=33
    mk_ram_page_table   table_index=34
    mk_ram_page_table   table_index=35
    mk_ram_page_table   table_index=36
    mk_ram_page_table   table_index=37
    mk_ram_page_table   table_index=38
    mk_ram_page_table   table_index=39
    mk_ram_page_table   table_index=40
    mk_ram_page_table   table_index=41
    mk_ram_page_table   table_index=42
    mk_ram_page_table   table_index=43
    mk_ram_page_table   table_index=44
    mk_ram_page_table   table_index=45
    mk_ram_page_table   table_index=46
    mk_ram_page_table   table_index=47
    mk_ram_page_table   table_index=48
    mk_ram_page_table   table_index=49
    mk_ram_page_table   table_index=50
    mk_ram_page_table   table_index=51
    mk_ram_page_table   table_index=52
    mk_ram_page_table   table_index=53
    mk_ram_page_table   table_index=54
    mk_ram_page_table   table_index=55

    .p2align 9
DSYM pointer_table
    /* VA = 0x0000_0000 "ST" RAM */
    mk_pointer_entry    page_index=0
    mk_pointer_entry    page_index=1
    mk_pointer_entry    page_index=2
    mk_pointer_entry    page_index=3
    mk_pointer_entry    page_index=4
    mk_pointer_entry    page_index=5
    mk_pointer_entry    page_index=6
    mk_pointer_entry    page_index=7
    mk_pointer_entry    page_index=8
    mk_pointer_entry    page_index=9
    mk_pointer_entry    page_index=10
    mk_pointer_entry    page_index=11
    mk_pointer_entry    page_index=12
    mk_pointer_entry    page_index=13
    mk_pointer_entry    page_index=14
    mk_pointer_entry    page_index=15
    mk_pointer_entry    page_index=16
    mk_pointer_entry    page_index=17
    mk_pointer_entry    page_index=18
    mk_pointer_entry    page_index=19
    mk_pointer_entry    page_index=20
    mk_pointer_entry    page_index=21
    mk_pointer_entry    page_index=22
    mk_pointer_entry    page_index=23
    mk_pointer_entry    page_index=24
    mk_pointer_entry    page_index=25
    mk_pointer_entry    page_index=26
    mk_pointer_entry    page_index=27
    mk_pointer_entry    page_index=28
    mk_pointer_entry    page_index=29
    mk_pointer_entry    page_index=30
    mk_pointer_entry    page_index=31
    mk_pointer_entry    page_index=32
    mk_pointer_entry    page_index=33
    mk_pointer_entry    page_index=34
    mk_pointer_entry    page_index=35
    mk_pointer_entry    page_index=36
    mk_pointer_entry    page_index=37
    mk_pointer_entry    page_index=38
    mk_pointer_entry    page_index=39
    mk_pointer_entry    page_index=40
    mk_pointer_entry    page_index=41
    mk_pointer_entry    page_index=42
    mk_pointer_entry    page_index=43
    mk_pointer_entry    page_index=44
    mk_pointer_entry    page_index=45
    mk_pointer_entry    page_index=46
    mk_pointer_entry    page_index=47
    mk_pointer_entry    page_index=48
    mk_pointer_entry    page_index=49
    mk_pointer_entry    page_index=50
    mk_pointer_entry    page_index=51
    mk_pointer_entry    page_index=52
    mk_pointer_entry    page_index=53
    mk_pointer_entry    page_index=54
    mk_pointer_entry    page_index=55
    /* VA = 0x00e0_0000 ROM */
    mk_pointer_entry    page_base=rom_page_tables,page_index=0
    mk_pointer_entry    page_base=rom_page_tables,page_index=1
    /* VA = 0x0100_0000 "TT" RAM */
    mk_pointer_entry    page_index=56
    mk_pointer_entry    page_index=57
    mk_pointer_entry    page_index=58
    mk_pointer_entry    page_index=59
    mk_pointer_entry    page_index=60
    mk_pointer_entry    page_index=61
    mk_pointer_entry    page_index=62
    mk_pointer_entry    page_index=63
    mk_pointer_entry    page_index=64
    mk_pointer_entry    page_index=65
    mk_pointer_entry    page_index=66
    mk_pointer_entry    page_index=67
    mk_pointer_entry    page_index=68
    mk_pointer_entry    page_index=69
    mk_pointer_entry    page_index=70
    mk_pointer_entry    page_index=71
    mk_pointer_entry    page_index=72
    mk_pointer_entry    page_index=73
    mk_pointer_entry    page_index=74
    mk_pointer_entry    page_index=75
    mk_pointer_entry    page_index=76
    mk_pointer_entry    page_index=77
    mk_pointer_entry    page_index=78
    mk_pointer_entry    page_index=79
    .dcb.l          46,PDT_INVALID

    .p2align 9
DSYM root_table
    /* VA = 0x0000_0000 */
    dc.l        pointer_table + U_USED + PDT_RESIDENT
    .dcb.l      127,PDT_INVALID

/* ensure that we don't spill out of the loader's allocation */
    .org    LOADER_SIZE

/* inhale the EmuTOS image */
DSYM emutos
    .incbin "emutos.img"
