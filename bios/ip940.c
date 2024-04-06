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
 */
void processor_init(void)
{
    longframe = 1;
    mcpu = 40;
    fputype = 0x00080000;
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

    /* enable the 200Hz/50Hz timers */
    TIMER_START = 0;
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
 * because we exploit the power-of-2 nature of the value here.
 *
 * Note: we use a full EXT_IOREC even though only the input
 *       side is technically exposed because a pointer to
 *       the input side is vended to applications and who
 *       knows what assumptions they make...
 */

#define NUM_COM_PORTS   4
#define COM_BUFSIZE     256

static struct
{
    WORD    code;
    UBYTE   cpr;
    UBYTE   dlm;
    UBYTE   dll;
    UBYTE   tcr;
} com_rate_table[] = {
    { B230400,  0x59, 0x00, 0x01, 0x0d },
    { B115200,  0x59, 0x00, 0x02, 0x0d },
    { B153600,  0x1f, 0x00, 0x04, 0x0e },
    { B38400,   0x09, 0x00, 0x1d, 0x0d },
    { B57600,   0x59, 0x00, 0x04, 0x0d },
    { B78600,   0x0e, 0x00, 0x1f, 0x00 },
    { B19200,   0x1c, 0x00, 0x1f, 0x00 },
    { B9600,    0x1f, 0x00, 0x38, 0x00 },
    { B4800,    0x38, 0x00, 0x3e, 0x00 },
    { B3600,    0x2e, 0x00, 0x73, 0x0e },
    { B2400,    0x4e, 0x00, 0x59, 0x00 },
    { B2000,    0x55, 0x00, 0x62, 0x00 },
    { B1800,    0x59, 0x00, 0x68, 0x00 },
    { B1200,    0x70, 0x00, 0x7c, 0x00 },
    { B600,     0x9c, 0x00, 0xb2, 0x00 },
    { B300,     0xeb, 0x00, 0xec, 0x00 },
    { B200,     0xff, 0x01, 0x46, 0x00 },
    { B150,     0xff, 0x01, 0xb3, 0x00 },
    { B134,     0xff, 0x01, 0xe7, 0x00 },
    { B110,     0xff, 0x02, 0x52, 0x00 },
    { B75,      0xff, 0x03, 0x67, 0x00 },
    { B50,      0xff, 0x05, 0x1b, 0x00 },
    { 460,      0x09, 0x00, 0x04, 0x00 }, /* 460800 */
    { 921,      0x09, 0x00, 0x02, 0x00 }, /* 921600 */
    { -1},
};

typedef struct
{
    volatile UBYTE *io;
    UBYTE       acr_shadow;
    EXT_IOREC   iorec;
    UBYTE       ibuf[COM_BUFSIZE];
} com_sc_t;
static com_sc_t com_sc[NUM_COM_PORTS];

static void
quart_write(com_sc_t *sc, UBYTE reg, UBYTE value)
{
    if (reg & QUART_INDEXED) {
        quart_write(sc, QUART_SPR, reg & ~QUART_INDEXED);
        quart_write(sc, QUART_ICR, value);
        if (reg == QUART_ACR) {
            sc->acr_shadow = value;
        }
    } else {
        *(sc->io + reg) = value;
    }
}

static UBYTE
quart_read(com_sc_t *sc, UBYTE reg)
{
    if (reg & QUART_INDEXED) {
        const WORD old_sr = set_sr(0x2700);
        UBYTE value;

        quart_write(sc, QUART_ACR, sc->acr_shadow | QUART_ACR_RDEN);
        quart_write(sc, QUART_SPR, reg & ~QUART_INDEXED);
        value = quart_read(sc, QUART_ICR);
        quart_write(sc, QUART_ACR, sc->acr_shadow & ~QUART_ACR_RDEN);
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
    return (quart_read(sc, QUART_LSR) & QUART_LSR_THRE) ? -1L : 0;
}

static LONG
com_bconout(WORD port, WORD dev, WORD b)
{
    com_sc_t * const sc = &com_sc[port];

    while (!com_bcostat(port)) {
    }
    quart_write(sc, QUART_THR, (UBYTE)b);
    return 1L;
}

static ULONG
com_rsconf(WORD port, WORD speed, WORD flow, WORD ucr, WORD rsr, WORD tsr, WORD scr)
{
    com_sc_t * const sc = &com_sc[port];
    LONG ret;

    if (speed == -2) {
        return sc->iorec.baudrate;
    }
    ret = (LONG)sc->iorec.ucr << 24;
    if (quart_read(sc, QUART_LSR) & QUART_LSR_BREAK) {
        ret |= 0x0800;
    }

    /* check for legal speed code & set */
    for (WORD i = 0; com_rate_table[i].code != -1; i++) {
        if (com_rate_table[i].code == speed) {
            quart_write(sc, QUART_FCR, QUART_FCR_FIFOEN);       /* enable FIFOs, extended mode */
            quart_write(sc, QUART_LCR, QUART_LCR_EXTEN);        /* enable extended registers */
            quart_write(sc, QUART_EFR, QUART_EFR_RTS | QUART_EFR_ENHMODE);
            quart_write(sc, QUART_LCR, QUART_LCR_DLAB);         /* enable divisor latch */
            quart_write(sc, QUART_DLM, com_rate_table[i].dlm);  /* load divisor */
            quart_write(sc, QUART_DLL, com_rate_table[i].dll);
            quart_write(sc, QUART_LCR, 0x03);                   /* XXX always n81 */
            quart_write(sc, QUART_SPR, QUART_CPR);              /* select CPR */
            quart_write(sc, QUART_ICR, com_rate_table[i].cpr);
            quart_write(sc, QUART_SPR, QUART_TCR);              /* select TCR */
            quart_write(sc, QUART_ICR, com_rate_table[i].tcr);
            quart_write(sc, QUART_MCR, QUART_MCR_INTEN | QUART_MCR_PRESCALE);
        }
    }
    /* XXX flow */
    /* XXX ucr */

    return ret;
}

static void
com_interrupt(WORD port)
{
    com_sc_t * const sc = &com_sc[port];
    IOREC *in = &com_sc[port].iorec.in;

    /* drain the receive FIFO as best we can */
    while (quart_read(sc, QUART_LSR) & QUART_LSR_RXRDY) {
        UBYTE b = quart_read(sc, QUART_RHR);
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
        //com_rsconf(i, DEFAULT_BAUDRATE, 0, 0x88, 0, 0, 0);
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
