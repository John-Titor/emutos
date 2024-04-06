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
 * Note that COM_BUFSIZE must be 256; both for TOS compatibility and
 * because we exploit the power-of-2 nature of the value here.
 *
 * XXX we don't need a full EXT_IOREC; only the input IOREC is
 *     exposed. We don't need an output IOREC at all given the 
 *     size of the output FIFOs.
 */

#define NUM_COM_PORTS   4
#define COM_BUFSIZE     256

typedef struct
{
    volatile UBYTE *io;
    EXT_IOREC   iorec;
    UBYTE       ibuf[RS232_BUFSIZE];
} com_sc_t;
static com_sc_t com_sc[NUM_COM_PORTS];

static UBYTE
quart_read(com_sc_t *sc, UBYTE reg)
{
    return *(sc->io + (reg << 2));
}

static void
quart_write(com_sc_t *sc, UBYTE reg, UBYTE value)
{
    *(sc->io + (reg << 2)) = value;
}

static LONG
com_bconstat(WORD port)
{
    IOREC * const in = &com_sc[port].iorec.in;

    return (in->head == in->tail) ? 0 : -1L;
}

static LONG
com_bconin(WORD port)
{
    while (!com_bconstat(port)) {
    }

    {
        WORD old_sr = set_sr(0x2700);
        IOREC * const in = &com_sc[port].iorec.in;
        LONG value;

        in->head = (in_head + 1) % COM_BUFSIZE;
        value = *(UBYTE *)(in->buf + in->head);

        set_sr(old_sr);
        return value;
    }
}

static LONG
com_bcostat(WORD port)
{
    com_sc_t * const sc = &com_sc[port];
    return (quart_read(sc, QUART_LSR) & QUART_LSR_TXRDY) ? -1L : 0;
}

static LONG
com_bconout(WORD port, WORD dev, WORD b)
{
    com_sc_t * const sc = &com_sc[port];

    while (!com_bcostat(port)) {
    }
    quart_write(sc, QUART_THR, (UBYTE)b);
}

static ULONG
com_rsconf(WORD port, WORD baud, WORD ctrl, WORD ucr, WORD rsr, WORD tsr, WORD scr)
{
    com_sc_t *sc = &com_sc[port];
    return 0;
}

static void
com_interrupt(WORD port)
{
    com_sc_t * const sc = &com_sc[port];
    IOREC *in = &com_sc[port].iorec.in;

    /* drain the receive FIFO as best we can */
    while (com_read(sc, QUART_LSR) & QUART_LSR_RXRDY) {
        UBYTE b = com_read(sc, QUART_RHR);
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

static const EXT_IOREC iorec_init = {
    { NULL, RS232_BUFSIZE, 0, 0, RS232_BUFSIZE/4, 3*RS232_BUFSIZE/4 },
    { NULL, RS232_BUFSIZE, 0, 0, RS232_BUFSIZE/4, 3*RS232_BUFSIZE/4 },
    DEFAULT_BAUDRATE, FLOW_CTRL_NONE, 0x88, 0xff, 0xea };

/*
 * Override init_serport() and install our own bconmap table.
 */
void init_serport(void)
{
    static MAPTAB maptable[NUM_COM_PORTS];
    MAPTAB *maptabptr;

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
    volatile UBYTE *lsr = (volatile UBYTE *)(QUART_BASE + QUART_LSR);
    volatile UBYTE *thr = (volatile UBYTE *)(QUART_BASE + QUART_THR);
    if (c == '\n') {
        kprintf_outc('\r');
    }

    while ((*lsr & QUART_LSR_TXRDY) == 0) {}
    *thr = c;
}

#endif /* MACHINE_IP940 */
