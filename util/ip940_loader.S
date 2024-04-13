/*
 * ROM bootloader for EmuTOS on IP940
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
    .equ    RAM_PHYS,       0x01000000
    .equ    LOADER_BASE,    0x00004000
    .equ    LOADER_SIZE,    0x00002000
    .equ    ROM_PHYS,       (LOADER_BASE + LOADER_SIZE)
    .equ    ROM_SIZE,       (256 * 1024)

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

/* console UART on the IP940 baseboard */
    .equ    QUART_BASE,     0x02110000
    .equ    QUART_THR,      QUART_BASE+(0x00<<2)+3
    .equ    QUART_LSR,      QUART_BASE+(0x05<<2)+3

/* emit a string, trashes a0 */
.macro mputs str
    lea     \str,%a0
    bsr     puts
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

/* exception vectors */
DSYM vectors
    .dc.l   0x01800000
    .dc.l   start
    .dcb.l  3,exception             /* access, address, illegal */

/* entrypoint from bootloader */
    .globl  start
TSYM start
    mputs   msg_boot1
    /* point VBR at our vectors */
    move.l  #vectors,%d0
    movec   %d0,%vbr

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

/* handle an (unexpected) exception, does not return */
TSYM exception
    mputs   err_exception1
    move.w  %sp@(6),%d0             /* format/vector code */
    ror.w   #2,%d0                  /* index -> vector # */
    bsr     putx8
    mputs   err_exception2
    move.l  %sp@(2),%d0
    bsr     putx32
    mputs   msg_newline
    bra     .

/* print the string in a0 */
TSYM puts
    tst.b   %a0@
    bne     1f
    rts
1:
    btst.b  #5,QUART_LSR
    beq     1b
    move.b  %a0@+,QUART_THR
    bra     puts

/* print the character in d0 */
TSYM putc
    btst.b  #5,QUART_LSR
    beq     putc
    move.b  %d0,QUART_THR
    rts

/* print 32/16/8-bit hex number */
TSYM putx32
    swap    %d0
    bsr     putx16
    swap    %d0
TSYM putx16
    rol.w   #8,%d0
    bsr     putx8
    rol.w   #8,%d0
TSYM putx8
    rol.b   #4,%d0
    bsr     putx
    rol.b   #4,%d0
TSYM putx
    move.b  %d0,%d1
    and.b   #0xf,%d1
    cmp.b   #10,%d1
    bge     1f
    add.b   #48,%d1
    bra     2f
1:
    add.b   #55,%d1
2:
    exg     %d0,%d1
    bsr     putc
    exg     %d0,%d1
    rts

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

    .align 4
DSYM end                /* nothing after this available with MMU on */
    .dc.l   0           /* keep end and rom_page_tables distinct */

/*
 * Pagetables.
 *
 * Since memory layout is fixed, we can compute these statically and leave
 * them in the ROM. If performance is terrible, they could be moved to RAM.
 * To minimise pagetable size and make the most of the ATC, we use 8K pages.
 *
 * We need:
 *  - a complete root table; 128 entries, 512B
 *  - one pointer table mapping 0x0000_0000-0x01ff_ffff, 128 entries, 512B
 *  - 48 page tables mapping 12M of RAM from 0x0100_0000 to 0x0000_0000-0x00bd_ffff, 48 x 32 entries, 6144B
 *  - two page tables mapping 488K of ROM from 0x0000_6000 to 0x00e0_0000, 61 entries, 244B
 *    (bootloader and this loader consume 3 pages of the first 512K of the ROM)
 *  - three indirect entries at the end of ROM space mapping any 8k to 0x00e7_[a,c,e]000, 3 entries, 12B
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
 * is mapped writeback-cachable, we attempt to placate the 040 table walker by
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

    .align 128
DSYM rom_page_tables
    mk_rom_page_table   table_index=0
    mk_rom_page_table   table_index=1,count=28
    .dc.l   RAM_PHYS + 0x50 + PDT_INDIRECT
    .dc.l   RAM_PHYS + 0x54 + PDT_INDIRECT
    .dc.l   RAM_PHYS + 0x58 + PDT_INDIRECT

    .align 128
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

    .align 512
DSYM pointer_table
    /* VA = 0x0000_0000 */
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
    .dcb.l          8,PDT_INVALID
    /* VA = 0x00e0_0000 */
    mk_pointer_entry    page_base=rom_page_tables,page_index=0
    mk_pointer_entry    page_base=rom_page_tables,page_index=1
    .dcb.l          70,PDT_INVALID

    .align 512
DSYM root_table
    /* VA = 0x0000_0000 */
    dc.l        pointer_table + U_USED + PDT_RESIDENT
    .dcb.l      127,PDT_INVALID

/* ensure that we don't spill out of the loader's allocation */
    .org    LOADER_SIZE

/* inhale the EmuTOS image */
DSYM emutos
    .incbin "emutos.img"
