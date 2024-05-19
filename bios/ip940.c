/*
 * ip940.c - IP940 specific functions.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/*
 * All interrupts are autovectored, and assigned as follows:
 *
 * 1 - expansion connector
 * 2 - unused
 * 3 - OX16C954 quad UART
 * 4 - 50Hz VBL timer
 * 5 - unused
 * 6 - 200Hz timer C emulation
 * 7 - unused
 *
 */

#include "emutos.h"
#ifdef MACHINE_IP940
#include "asm.h"
#include "blkdev.h"
#include "clock.h"
#include "delay.h"
#include "disk.h"
#include "gemerror.h"
#include "ip940.h"
#include "ikbd.h"
#include "machine.h"
#include "processor.h"
#include "screen.h"
#include "serport.h"
#include "string.h"
#include "tosvars.h"
#include "vectors.h"
#include "cookie.h"

static void com_console_tick(void);
static void com_console_input(UBYTE b);

LONG gettime(void)
{
    return 0;
}

void settime(LONG time)
{
}

void screen_init_mode(void)
{
    /* more or less correct */
    sshiftmod = ST_HIGH;
    defshiftmod = ST_HIGH;
}

/*
 * Override processor.S:processor_init, which does unhelpful things
 * like turning off the MMU and messing with the TTRs.
 *
 * For IP940 the loader configures the MMU to present a compatible
 * memory map, and DTT0 to map IP940 I/O space, so we need those
 * mappings to be left alone.
 */
void processor_init(void)
{
    longframe = 1;
    mcpu = 40;
    fputype = 0x00080000;

    /*
     * Enable the instruction and data caches.
     *
     * Caches are normally off during the boot flow, and startup.S
     * explicitly turns them off as well, so it's safe to assume
     * they are off and there is nothing dirty in a writeback region.
     * We invalidate to ensure that there's nothing left over from
     * before the last reset.
     */
    __asm__ volatile("    cinva  bc                 \n"
                     "    move.l #0x80008000,d0     \n"
                     "    movec  d0,cacr            \n"
                     : : : "d0", "memory", "cc"
                     );
}

/*
 * Level 4 / 50Hz pseudo-VBL interrupt handler.
 */
__attribute__((interrupt))
static void vbl_wrapper(void)
{
    /* returns with RTS for !CONF_WITH_ATARI_VIDEO */
    int_vbl();

    /* poke the serial console input state machine */
    com_console_tick();
}

/*
 * Override machine.c:machine_init
 */
void machine_init(void)
{
    KDEBUG(("CPLD rev 0x%02x\n", CPLD_REVISION));

    /* install interrupt handlers */
    VEC_LEVEL4 = vbl_wrapper;
    VEC_LEVEL6 = int_timerc;

    /* I$/D$ on */
    loopcount_1_msec = 11000;

    /* enable the 200Hz/50Hz timers */
    TIMER_START = 0;
}

/*
 * Add machine-specific cookies.
 */
void machine_add_cookies(void)
{
    /* install the PMMU cookie to claim the PMMU */
    cookie_add(0x504d4d55, 0UL);
}

/*
 * Override detect_32bit_address_bus, as it will hang and we already
 * know the answer.
 */
BOOL
detect_32bit_address_bus(void)
{
    return 1;
}

/*
 * Override machine_name and return something appropriate.
 */
const char *
machine_name(void)
{
    return "IP940";
}

/********************************************************************
 * ROMdisk
 *
 * The ROM'ed pagetable places indirect PTEs in the last three pages
 * of the 512k ROM aperture; we use one of them as a window into the
 * remainder of the ROM, where a filesystem can be stored.
 */
#if CONF_WITH_ROMDISK

#define PAGE_SIZE   0x2000UL

/*
 * Remap the ROM reader window to point to the page containing the
 * requested ROMdisk sector.
 */
static const UBYTE *
rd_set_window(LONG sector)
{
    /* we use the indirect PTE in the vector 22 slot to map the ROM window */
    static ULONG * const pte = (ULONG *)0x58UL;
    static const ULONG pte_phys = RAM_PHYS + 0x58UL;

    /* this PTE maps the last page of the 512K ROM aperture */
    static const UBYTE * const window = (const UBYTE *)(0x00e80000UL - PAGE_SIZE);

    static ULONG current_mapping;
    ULONG target_offset = sector * SECTOR_SIZE;
    ULONG target_mapping = (ROMDISK_PHYS + target_offset) & ~(PAGE_SIZE - 1);

    if (current_mapping != target_mapping) {
        /*
         * Update the window PTE to the target page;
         * supervisor, uncached, write-protected, used/modified, resident.
         * Note that this descriptor is in cached / writeback space, which
         * the CPU really doesn't like (all RAM is cached/writeback), so we
         * make sure to push the line out immediately.
         *
         * We have secret knowledge here:
         *  - Vectors are always at the bottom of physical RAM, whose physical
         *    address we should not really know (we could get it by translating
         *    %vbr, but we cheat because it will never change).
         *  - The PTEs for the last three pages of ROM space indirect to the slots
         *    for exceptions 20/21/22 (this is a contract with ip940_loader.S).
         */
        *pte = target_mapping | 0xff;
        KDEBUG(("romdisk: map sector 0x%lx, window %p->0x%lx, pte 0x%08lx\n", sector, window, target_mapping, *pte));

        __asm__ volatile("    cpushl %%dc,%0@   \n" /* clean the PTE to RAM and invalidate the cache line */
                         "    moveq  #5,d0      \n" /* we want to clear supervisor translations */
                         "    movec  d0,dfc     \n" /* so set the destination function code to suit */
                         "    pflush %1@        \n" /* clean any old translation for the window from the ATC */
                         :
                         : "a"(pte_phys), "a"(window)
                         : "d0", "memory");
        current_mapping = target_mapping;
    }
    return window + (target_offset % PAGE_SIZE);
}

void
romdisk_init(WORD dev, LONG *devices_available)
{
    UNIT * const u = &units[dev];
    const UBYTE * const secptr = rd_set_window(0);

    /* look for FAT bootsector signature */
    if ((secptr[0x1fe] != 0x55) || (secptr[0x1ff] != 0xaa)) {
        KDEBUG(("romdisk: unexpected bootsector signature %02x,%02x\n", secptr[0x1fe], secptr[0x1ff]));
        return;
    }

    /* try adding this as though it were a partition - type must be something recognized */
    if (add_partition(dev, devices_available, "GEM", 0, ROMDISK_SIZE / SECTOR_SIZE)) {
        KDEBUG(("romdisk: add_partition failed\n"));
        return;
    }

    KDEBUG(("romdisk: attached unit %d\n", dev));

    u->valid = 1;
    u->size = ROMDISK_SIZE / SECTOR_SIZE;
    u->psshift = get_shift(SECTOR_SIZE);
    u->last_access = 0;
    u->status = 0;
    u->features = 0;
}

LONG
romdisk_ioctl(WORD dev, UWORD ctrl, void *arg)
{
    switch (ctrl) {
    case GET_DISKINFO:
        {
            ULONG *info = (ULONG *)arg;
            info[0] = ROMDISK_SIZE / SECTOR_SIZE;
            info[1] = SECTOR_SIZE;
            return E_OK;
        }

    case GET_DISKNAME:
        strcpy(arg, "romdisk");
        return E_OK;

    case GET_MEDIACHANGE:
        return MEDIANOCHANGE;
    }
    return ERR;
}

LONG
romdisk_rw(WORD rw, LONG sector, WORD count, UBYTE *buf, WORD dev)
{
    if ((rw & RW_RW) != RW_READ) {
        return EWRPRO;
    }

    while (count--) {
        const UBYTE *window = rd_set_window(sector);
        memcpy(buf, window, SECTOR_SIZE);
        sector++;
        buf += SECTOR_SIZE;
    }
    return E_OK;
}
#endif /* CONF_WITH_ROMDISK */

/********************************************************************
 * Serial ports on the OX16C954.
 *
 * We run these in 16C950 mode with maxed-out FIFOs and (when
 * selected) automatic hardware flow control.
 *
 * Note: COM_BUFSIZE must be 256; both for TOS compatibility and
 *       because we exploit the power-of-2 nature of the value
 *       here.
 *
 * Note: we use a full EXT_IOREC even though only the input
 *       side is technically exposed because a pointer to
 *       the input side is vended to applications and who
 *       knows what assumptions they make...
 *
 * Note: access to indexed registers requires masking interrupts to
 *       prevent interference from handler code.
 */

#define NUM_COM_PORTS   4
#define COM_BUFSIZE     256

#ifndef B921600
# define B921600    92
#endif
#ifndef B460800
# define B460800    46
#endif

static struct
{
    WORD    code;
    UBYTE   cpr;
    UBYTE   dlm;
    UBYTE   dll;
    UBYTE   tcr;
} com_rate_table[] = {
    { B921600,  0x09, 0x00, 0x02, 0x00 },
    { B460800,  0x09, 0x00, 0x04, 0x00 },
    { B230400,  0x08, 0x00, 0x09, 0x00 },
    { B153600,  0x6d, 0x00, 0x01, 0x00 },
    { B115200,  0x1d, 0x00, 0x05, 0x00 },
    { B78600,   0x35, 0x00, 0x04, 0x00 },
    { B57600,   0x11, 0x00, 0x11, 0x00 },
    { B38400,   0x0e, 0x00, 0x1f, 0x00 },
    { B19200,   0x1c, 0x00, 0x1f, 0x00 },
    { B9600,    0x1f, 0x00, 0x38, 0x00 },
    { B4800,    0x38, 0x00, 0x3e, 0x00 },
    { B3600,    0x34, 0x00, 0x59, 0x00 },
    { B2400,    0x4e, 0x00, 0x59, 0x00 },
    { B2000,    0x55, 0x00, 0x62, 0x00 },
    { B1800,    0x59, 0x00, 0x68, 0x00 },
    { B1200,    0x70, 0x00, 0x7c, 0x00 },
    { B600,     0x9c, 0x00, 0xb2, 0x00 },
    { B300,     0xeb, 0x00, 0xec, 0x00 },
    { B150,     0x1d, 0x00, 0x05, 0x00 },   /* RSVE extension -> 115200 */
    { B134,     0x11, 0x00, 0x11, 0x00 },   /* RSVE extension -> 57600 */
    { B110,     0x0e, 0x00, 0x1f, 0x00 },   /* RSVE extension -> 38400 */
    { -1},
};

typedef struct
{
    WORD        rate_index;
    WORD        flow_code;
    WORD        line_code;
    UBYTE       acr_shadow;
    EXT_IOREC   iorec;
    UBYTE       ibuf[COM_BUFSIZE];
} com_sc_t;
static com_sc_t com_sc[NUM_COM_PORTS];

static void
com_reg_write(UWORD port, UBYTE reg, UBYTE value)
{
    if (reg & QUART_INDEXED) {
        const WORD old_sr = set_sr(0x2700);
        com_reg_write(port, QUART_SPR, reg & ~QUART_INDEXED);
        com_reg_write(port, QUART_ICR, value);
        if (reg == QUART_ACR) {
            com_sc[port].acr_shadow = value;
        }
        set_sr(old_sr);
    } else {
        QUART_REG(port, reg) = value;
    }
}

static UBYTE
com_reg_read(UWORD port, UBYTE reg)
{
    if (reg & QUART_INDEXED) {
        const WORD old_sr = set_sr(0x2700);
        UBYTE value;

        com_reg_write(port, QUART_ACR, com_sc[port].acr_shadow | QUART_ACR_RDEN);
        com_reg_write(port, QUART_SPR, reg & ~QUART_INDEXED);
        value = com_reg_read(port, QUART_ICR);
        com_reg_write(port, QUART_ACR, com_sc[port].acr_shadow & ~QUART_ACR_RDEN);
        set_sr(old_sr);
        return value;
    } else {
        return QUART_REG(port, reg);
    }
}

static LONG
com_bconstat(UWORD port)
{
    const IOREC * const in = &com_sc[port].iorec.in;

    return (in->head == in->tail) ? 0 : -1L;
}

static LONG
com_bconin(UWORD port)
{
    while (!com_bconstat(port)) {}

    {
        const WORD old_sr = set_sr(0x2700);
        IOREC * const in = &com_sc[port].iorec.in;
        LONG value;

        in->head = (in->head + 1) % COM_BUFSIZE;
        value = *(UBYTE *)(in->buf + in->head);

        set_sr(old_sr);
        return value;
    }
}

static LONG
com_bcostat(UWORD port)
{
    return (com_reg_read(port, QUART_LSR) & QUART_LSR_THRE) ? -1L : 0;
}

static LONG
com_bconout(UWORD port, WORD b)
{
    while (!com_bcostat(port)) {
    }
    com_reg_write(port, QUART_THR, (UBYTE)b);
    return 1L;
}

static void
com_drain(UWORD port)
{
    WORD    pollcount = 133;        /* 128B FIFO @ 9600bps = 133ms */

    while (!(com_reg_read(port, QUART_LSR) & QUART_LSR_TXEMT) && pollcount--) {
        delay_loop(loopcount_1_msec);
    }
}

static ULONG
com_rsconf(UWORD port, WORD speed, WORD flow, WORD ucr, WORD rsr, WORD tsr, WORD scr)
{
    com_sc_t * const sc = &com_sc[port];
    LONG ret;

    /* check for current speed request */
    if (speed == -2) {
        return sc->iorec.baudrate;
    }
    ret = (LONG)sc->iorec.ucr << 24;
    if (com_reg_read(port, QUART_LSR) & QUART_LSR_BREAK) {
        ret |= 0x0800;
    }

    /* check for legal speed code & set */
    for (WORD i = 0; com_rate_table[i].code != -1; i++) {
        if (com_rate_table[i].code == speed) {
            sc->rate_index = i;
            sc->iorec.baudrate = speed;
            break;
        }
    }

    /* update flow control selection */
    if ((flow >= MIN_FLOW_CTRL) && (flow < MAX_FLOW_CTRL)) {
        sc->flow_code = flow;
    }

    /* update data format selection */
    if (ucr > 0) {
        /* data length */
        sc->line_code = 3 - ((ucr & 0x60) >> 5);

        /* stop bit(s) */
        sc->line_code |= (ucr & 0x10) ? QUART_LCR_STOP2 : QUART_LCR_STOP1;

        /* parity */
        if (ucr & 0x04) {
            sc->line_code |= (ucr & 0x02) ? QUART_LCR_PAREVEN : QUART_LCR_PARODD;
        }
    }

    /* wait for the TX FIFO to drain */
    com_drain(port);

    /* configure as requested */
    com_reg_write(port, QUART_FCR, QUART_FCR_FIFOEN);       /* enable FIFOs, extended mode */
    com_reg_write(port, QUART_LCR, QUART_LCR_EXTEN);        /* enable extended registers */
    com_reg_write(port, QUART_EFR, QUART_EFR_RTS | QUART_EFR_ENHMODE);
    com_reg_write(port, QUART_LCR, QUART_LCR_DLAB);         /* enable divisor latch */
    com_reg_write(port, QUART_DLL, com_rate_table[sc->rate_index].dll);
    com_reg_write(port, QUART_DLM, com_rate_table[sc->rate_index].dlm);
    com_reg_write(port, QUART_LCR, sc->line_code);
    com_reg_write(port, QUART_CPR, com_rate_table[sc->rate_index].cpr);
    com_reg_write(port, QUART_TCR, com_rate_table[sc->rate_index].tcr);
    com_reg_write(port, QUART_MCR, QUART_MCR_INTEN | QUART_MCR_PRESCALE);
    com_reg_write(port, QUART_IER, QUART_IER_RXRDY);
    /* XXX handle flow control */

    return ret;
}

static void
com_interrupt(UWORD port)
{
    IOREC *in = &com_sc[port].iorec.in;

    /* drain the receive FIFO as best we can */
    while (com_reg_read(port, QUART_LSR) & QUART_LSR_RXRDY) {
        UBYTE b = com_reg_read(port, QUART_RHR);
        WORD tail = (in->tail + 1) % COM_BUFSIZE;

        if (tail == in->head) {
            /* buffer overflow, drop byte */
            /* XXX we could just turn off interrupts here and let the FIFO fill up */
        } else {
            in->buf[tail] = b;
        }
        in->tail = tail;
#if CONF_SERIAL_CONSOLE && !CONF_SERIAL_CONSOLE_POLLING_MODE
        if (port == 0) {
            com_console_input(b);
        }
#endif
    }
}

__attribute__((interrupt))
static void
com_shared_interrupt(void)
{
    for (WORD i = 0; i < NUM_COM_PORTS; i++) {
        com_interrupt(i);
    }
}

#define ESC_SEQ_LEN_MAX 8
static UBYTE    esc_seq[ESC_SEQ_LEN_MAX];
static UWORD    esc_seq_len;
static UWORD    esc_seq_timer;

static const struct scancode_sequence
{
    ULONG       scancode;
    UBYTE       sequence[ESC_SEQ_LEN_MAX];
} sequence_table[] =
{
    /*
     * Keeping ESC in this table seems wasteful, but it greatly
     * simplifies the input state machine / fallback code.
     */
    { 0x48, { 0x1b, '[', 'A' } },           /* up arrow */
    { 0x50, { 0x1b, '[', 'B' } },           /* down arrow */
    { 0x4d, { 0x1b, '[', 'C' } },           /* right arrow */
    { 0x4b, { 0x1b, '[', 'D' } },           /* left arrow */
    { 0x3b, { 0x1b, '[', '1', 'P', } },     /* vt F1 */
    { 0x3c, { 0x1b, '[', '1', 'Q', } },     /* vt F2 */
    { 0x3d, { 0x1b, '[', '1', 'R', } },     /* vt F3 */
    { 0x3e, { 0x1b, '[', '1', 'S', } },     /* vt F4 */
    { 0x3b, { 0x1b, 'O', 'P', } },          /* Xterm F1 */
    { 0x3c, { 0x1b, 'O', 'Q', } },          /* Xterm F2 */
    { 0x3d, { 0x1b, 'O', 'R', } },          /* Xterm F3 */
    { 0x3e, { 0x1b, 'O', 'S', } },          /* Xterm F4 */
    { 0x3f, { 0x1b, '[', '1', '5', '~' } }, /* Xterm F5 */
    { 0x40, { 0x1b, '[', '1', '7', '~' } }, /* Xterm F6 */
    { 0x41, { 0x1b, '[', '1', '8', '~' } }, /* Xterm F7 */
    { 0x42, { 0x1b, '[', '1', '9', '~' } }, /* Xterm F8 */
    { 0x43, { 0x1b, '[', '2', '0', '~' } }, /* Xterm F9 */
    { 0x44, { 0x1b, '[', '2', '1', '~' } }, /* Xterm F10 */
    { 0 }
};

static void
com_console_tick(void)
{
    WORD    i;

    switch (esc_seq_timer) {
    case 0:
        /* no timer, do nothing */
        break;
    case 1:
        /* timer expiring, push timed-out sequence in as ascii */
        for (i = 0; i < esc_seq_len; i++) {
            push_ascii_ikbdiorec(esc_seq[i]);
        }
        esc_seq_len = 0;
        /* FALLTHROUGH */
    default:
        /* timer still running */
        esc_seq_timer--;
        break;
    }
}

static void
com_console_input(UBYTE b)
{
    WORD    i, j;

    /* if not processing a sequence and not the start of a sequence, send as input */
    if ((esc_seq_len == 0) && (b != 0x1b)) {
        push_ascii_ikbdiorec(b);
        return;
    }
    esc_seq[esc_seq_len++] = b;

    /* scan for sequence */
    BOOL candidate = FALSE;
    const struct scancode_sequence * seq;
    for (seq = &sequence_table[0]; seq->scancode != 0; seq++) {
        /* do sequence lengths match? */
        if (seq->sequence[esc_seq_len] != 0) {
            /* could still be a candidate */
            candidate = TRUE;
            /* no, not this sequence */
            continue;
        }
        /* compare sequences */
        for (j = 0; j < esc_seq_len; j++) {
            if (esc_seq[j] != seq->sequence[j]) {
                break;
            }
        }
        /* match? */
        if (j == esc_seq_len) {
            /* yes: push scan code, reset state machine, done */
            KDEBUG(("cvt 0x%08lx\n", MAKE_ULONG(seq->scancode, 0)));
            push_ikbdiorec(MAKE_ULONG(seq->scancode, 0));
            esc_seq_len = 0;
            esc_seq_timer = 0;
            return;
        }
    }

    /* are there still candidates we might match? */
    if (candidate) {
        /* set timer to ~100ms and wait */
        esc_seq_timer = 2;
    } else {
        /* not a recognized sequence, just stuff the bytes in */
        for (i = 0; i < esc_seq_len; i++) {
            push_ascii_ikbdiorec(esc_seq[i]);
        }
        /* ... and reset the state machine */
        esc_seq_len = 0;
        esc_seq_timer = 0;
    }
}

#define COM_DEFINE_FUNCTIONS(_x)                                                            \
static LONG com##_x##_bconstat(void)            { return com_bconstat(_x); }                \
static LONG com##_x##_bconin(void)              { return com_bconin(_x); }                  \
static LONG com##_x##_bcostat(void)             { return com_bcostat(_x); }                 \
static LONG com##_x##_bconout(WORD dev, WORD b) { return com_bconout(_x, b); }              \
static ULONG com##_x##_rsconf(WORD baud, WORD ctrl, WORD ucr, WORD rsr, WORD tsr, WORD scr) \
    { return com_rsconf(_x, baud, ctrl, ucr, rsr, tsr, scr); }                              \
struct hack

COM_DEFINE_FUNCTIONS(0);
COM_DEFINE_FUNCTIONS(1);
COM_DEFINE_FUNCTIONS(2);
COM_DEFINE_FUNCTIONS(3);

static const MAPTAB maptab_init[NUM_COM_PORTS] = {
#define COM_MAPTAB_ENTRY(_x)    { .Bconstat = com##_x##_bconstat,   \
                                  .Bconin   = com##_x##_bconin,     \
                                  .Bcostat  = com##_x##_bcostat,    \
                                  .Bconout  = com##_x##_bconout,    \
                                  .Rsconf   = com##_x##_rsconf,     \
                                  .Iorec    = &com_sc[_x].iorec }
    COM_MAPTAB_ENTRY(0),
    COM_MAPTAB_ENTRY(1),
    COM_MAPTAB_ENTRY(2),
    COM_MAPTAB_ENTRY(3),
};

/* default iorec contents; mostly ignored */
static const EXT_IOREC iorec_init = {
    { NULL, COM_BUFSIZE, 0, 0, COM_BUFSIZE/4, 3*COM_BUFSIZE/4 },
    { NULL, COM_BUFSIZE, 0, 0, COM_BUFSIZE/4, 3*COM_BUFSIZE/4 },
    DEFAULT_BAUDRATE, FLOW_CTRL_NONE, 0x88, 0xff, 0xea };

/* RSVF cookie structure */
typedef struct {
    const char  *name;
    UBYTE       flags;
    UBYTE       _res0;
    UBYTE       bios_dev;
    UBYTE       _res1;
} rsvf_interface_t;

static rsvf_interface_t rsvf_data[4];

/*
 * Override init_serport() and install our own bconmap table.
 */
void init_serport(void)
{
    static MAPTAB maptable[NUM_COM_PORTS];
    const MAPTAB * maptabptr;

    /*
     * Minimal reimplementation of init_bconmap.
     *
     * It seems attractive to start with a ROM'ed mapping table, but the
     * Bconmap implementation assumes that the 'old' table can be written.
     */
    memcpy(&maptable, &maptab_init, sizeof(maptab_init));
    bconmap_root.maptab = &maptable[0];
    bconmap_root.maptabsize = NUM_COM_PORTS;
    bconmap_root.mapped_device = BCONMAP_START_HANDLE;

    /* install interrupt handler */
    VEC_LEVEL3 = com_shared_interrupt;

    /* init COM port data and configure ports */
    for (WORD i = 0; i < NUM_COM_PORTS; i++) {
        memcpy(&com_sc[i].iorec, &iorec_init, sizeof(iorec_init));
        com_sc[i].iorec.in.buf = com_sc[i].ibuf;
        com_rsconf(i, DEFAULT_BAUDRATE, 0, 0x88, 0, 0, 0);
    }

    /* wire up COM0 as the console device */
    maptabptr = &maptable[bconmap_root.mapped_device-BCONMAP_START_HANDLE];
    bconstat_vec[1] = maptabptr->Bconstat;
    bconin_vec[1] = maptabptr->Bconin;
    bcostat_vec[1] = maptabptr->Bcostat;
    bconout_vec[1] = maptabptr->Bconout;
    rsconfptr = maptabptr->Rsconf;
    rs232iorecptr = maptabptr->Iorec;

    /* install RSVF cookie and initial table to give names to BIOS ports */
    rsvf_data[0].name = "COM1";
    rsvf_data[0].flags = 0xa0;      /* is-interface, BIOS only */
    rsvf_data[0].bios_dev = BCONMAP_START_HANDLE + 1;
    rsvf_data[1].name = "COM2";
    rsvf_data[1].flags = 0xa0;      /* is-interface, BIOS only */
    rsvf_data[1].bios_dev = BCONMAP_START_HANDLE + 2;
    rsvf_data[2].name = "COM3";
    rsvf_data[2].flags = 0xa0;      /* is-interface, BIOS only */
    rsvf_data[2].bios_dev = BCONMAP_START_HANDLE + 3;
    rsvf_data[3].name = 0;
    rsvf_data[3].flags = 0;
    cookie_add(0x52535646UL, (ULONG)&rsvf_data);

    /* install RSVE cookie to indicate support for 38400/57600/115200 */
    cookie_add(0x52535645UL, 0UL);
}

/*
 * Override kprintf_outc and talk directly to QUART channel 0.
 */
void kprintf_outc(int c)
{
    if (c == '\n') {
        kprintf_outc('\r');
    }

    while ((QUART_REG(0, QUART_LSR) & QUART_LSR_THRE) == 0) {}
    QUART_REG(0, QUART_THR) = c;

    if (c == '\n') {
        com_drain(0);
    }
}

#endif /* MACHINE_IP940 */
