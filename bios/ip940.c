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
#include "clock.h"
#include "delay.h"
#include "ip940.h"
#include "machine.h"
#include "processor.h"
#include "screen.h"
#include "serport.h"
#include "string.h"
#include "tosvars.h"
#include "vectors.h"


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
}

/*
 * Override machine.c:machine_init
 */
void machine_init(void)
{
    /* install interrupt handlers */
    VEC_LEVEL4 = vbl_wrapper;
    VEC_LEVEL6 = int_timerc;

    /* I$/D$ on */
    loopcount_1_msec = 11000;

    /* enable the 200Hz/50Hz timers */
    TIMER_START = 0;
}

/*
 * Override detect_32bit_address_bus, as it will hang.
 */
BOOL
detect_32bit_address_bus(void)
{
    return 1;
}

const char *
machine_name(void)
{
    return "IP940";
}

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
# define B921600    921
#endif
#ifndef B460800
# define B460800    460
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
    { B115200,  0x1d, 0x00, 0x05, 0x00 },
    { B153600,  0x6d, 0x00, 0x01, 0x00 },
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
    { -1},
};

typedef struct
{
    volatile UBYTE *io;
    UBYTE       acr_shadow;
    WORD        rate_index;
    WORD        flow_code;
    WORD        line_code;
    EXT_IOREC   iorec;
    UBYTE       ibuf[COM_BUFSIZE];
} com_sc_t;
static com_sc_t com_sc[NUM_COM_PORTS];

static void
com_reg_write(com_sc_t *sc, UBYTE reg, UBYTE value)
{

    if (reg & QUART_INDEXED) {
        const WORD old_sr = set_sr(0x2700);
        com_reg_write(sc, QUART_SPR, reg & ~QUART_INDEXED);
        com_reg_write(sc, QUART_ICR, value);
        if (reg == QUART_ACR) {
            sc->acr_shadow = value;
        }
        set_sr(old_sr);
    } else {
        *(sc->io + reg) = value;
    }
}

static UBYTE
com_reg_read(com_sc_t *sc, UBYTE reg)
{
    if (reg & QUART_INDEXED) {
        const WORD old_sr = set_sr(0x2700);
        UBYTE value;

        com_reg_write(sc, QUART_ACR, sc->acr_shadow | QUART_ACR_RDEN);
        com_reg_write(sc, QUART_SPR, reg & ~QUART_INDEXED);
        value = com_reg_read(sc, QUART_ICR);
        com_reg_write(sc, QUART_ACR, sc->acr_shadow & ~QUART_ACR_RDEN);
        set_sr(old_sr);
        return value;
    } else {
        return *(sc->io + reg);
    }
}

static LONG
com_bconstat(WORD port)
{
    const IOREC * const in = &com_sc[port].iorec.in;

    return (in->head == in->tail) ? 0 : -1L;
}

static LONG
com_bconin(WORD port)
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
com_bcostat(WORD port)
{
    com_sc_t * const sc = &com_sc[port];

    return (com_reg_read(sc, QUART_LSR) & QUART_LSR_THRE) ? -1L : 0;
}

static LONG
com_bconout(WORD port, WORD dev, WORD b)
{
    com_sc_t * const sc = &com_sc[port];

    while (!com_bcostat(port)) {
    }
    com_reg_write(sc, QUART_THR, (UBYTE)b);
    return 1L;
}

static void
com_drain(com_sc_t *sc)
{
    WORD    pollcount = 133;        /* 128B FIFO @ 9600bps = 133ms */

    while (!(com_reg_read(sc, QUART_LSR) & QUART_LSR_TXEMT) && pollcount--) {
        delay_loop(loopcount_1_msec);
    }
}

static ULONG
com_rsconf(WORD port, WORD speed, WORD flow, WORD ucr, WORD rsr, WORD tsr, WORD scr)
{
    com_sc_t * const sc = &com_sc[port];
    LONG ret;

    /* check for current speed request */
    if (speed == -2) {
        return sc->iorec.baudrate;
    }
    ret = (LONG)sc->iorec.ucr << 24;
    if (com_reg_read(sc, QUART_LSR) & QUART_LSR_BREAK) {
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

    /* try to drain the transmitter */
    com_drain(sc);

    /* configure as requested */
    com_reg_write(sc, QUART_FCR, QUART_FCR_FIFOEN);       /* enable FIFOs, extended mode */
    com_reg_write(sc, QUART_LCR, QUART_LCR_EXTEN);        /* enable extended registers */
    com_reg_write(sc, QUART_EFR, QUART_EFR_RTS | QUART_EFR_ENHMODE);
    com_reg_write(sc, QUART_LCR, QUART_LCR_DLAB);         /* enable divisor latch */
    com_reg_write(sc, QUART_DLM, com_rate_table[sc->rate_index].dlm);
    com_reg_write(sc, QUART_DLL, com_rate_table[sc->rate_index].dll);
//    com_reg_write(sc, QUART_LCR, 0x03);                   /* XXX always n81 */
    com_reg_write(sc, QUART_LCR, sc->line_code);
    com_reg_write(sc, QUART_SPR, QUART_CPR);              /* select CPR */
    com_reg_write(sc, QUART_ICR, com_rate_table[sc->rate_index].cpr);
    com_reg_write(sc, QUART_SPR, QUART_TCR);              /* select TCR */
    com_reg_write(sc, QUART_ICR, com_rate_table[sc->rate_index].tcr);
    com_reg_write(sc, QUART_MCR, QUART_MCR_INTEN | QUART_MCR_PRESCALE);
    com_reg_write(sc, QUART_IER, QUART_IER_RXRDY);
    /* XXX handle flow control */

    return ret;
}

static void
com_interrupt(WORD port)
{
    com_sc_t * const sc = &com_sc[port];
    IOREC *in = &com_sc[port].iorec.in;

    /* drain the receive FIFO as best we can */
    while (com_reg_read(sc, QUART_LSR) & QUART_LSR_RXRDY) {
        UBYTE b = com_reg_read(sc, QUART_RHR);
        WORD tail = (in->tail + 1) % COM_BUFSIZE;

        if (tail == in->head) {
            /* buffer overflow, drop byte */
            /* XXX we could just turn off interrupts here and let the FIFO fill up */
        } else {
            KDEBUG(("com_interrupt: rx %02x\n", b));
            in->buf[tail] = b;
        }
        in->tail = tail;
#if CONF_SERIAL_CONSOLE && !CONF_SERIAL_CONSOLE_POLLING_MODE
        if (port == 0) {
            /* stuff the incoming byte into the keyboard buffer */
            push_ascii_ikbdiorec(b);
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

#define COM_DEFINE_FUNCTIONS(_x)                                                            \
static LONG com##_x##_bconstat(void)            { return com_bconstat(_x); }                \
static LONG com##_x##_bconin(void)              { return com_bconin(_x); }                  \
static LONG com##_x##_bcostat(void)             { return com_bcostat(_x); }                 \
static LONG com##_x##_bconout(WORD dev, WORD b) { return com_bconout(_x, dev, b); }         \
static ULONG com##_x##_rsconf(WORD baud, WORD ctrl, WORD ucr, WORD rsr, WORD tsr, WORD scr) \
    { return com_rsconf(_x, baud, ctrl, ucr, rsr, tsr, scr); }                              \
struct hack

COM_DEFINE_FUNCTIONS(0);
COM_DEFINE_FUNCTIONS(1);
COM_DEFINE_FUNCTIONS(2);
COM_DEFINE_FUNCTIONS(3);

static const MAPTAB maptab_init[4] = {
#define COM_MAPTAB_ENTRY(_x)    { com##_x##_bconstat, com##_x##_bconin, com##_x##_bcostat, com##_x##_bconout, com##_x##_rsconf, &com_sc[_x].iorec }
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
     * It seems attractive to start with a static mapping table, but the Bconmap
     * implementation assumes that the table can be written.
     */
    memcpy(&maptable, &maptab_init, sizeof(maptab_init));
    bconmap_root.maptab = maptable;
    bconmap_root.maptabsize = NUM_COM_PORTS;
    bconmap_root.mapped_device = BCONMAP_START_HANDLE;

    /* install interrupt handler */
    VEC_LEVEL3 = com_shared_interrupt;

    /* init COM port data and configure ports */
    for (WORD i = 0; i < NUM_COM_PORTS; i++) {
        memcpy(&com_sc[i].iorec, &iorec_init, sizeof(iorec_init));
        com_sc[i].iorec.in.buf = com_sc[i].ibuf;
        com_sc[i].io = (volatile UBYTE *)(QUART_BASE + i * QUART_STRIDE);
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
}

/*
 * Override kprintf_outc and talk directly to QUART channel 0.
 */
void kprintf_outc(int c)
{
    volatile const UBYTE * const lsr = (volatile UBYTE *)(QUART_BASE + QUART_LSR);
    volatile UBYTE * const thr = (volatile UBYTE *)(QUART_BASE + QUART_THR);
    if (c == '\n') {
        kprintf_outc('\r');
    }

    while ((*lsr & QUART_LSR_THRE) == 0) {}
    *thr = c;
}

#endif /* MACHINE_IP940 */
