/*
 * pt68k5.c - PT68K5 specific functions.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/*
 * All interrupts are autovectored, and assigned as follows:
 *
 * 1 - unavailable (not routed)
 * 2 - MC68230 PIT
 * 3 - spare (SCSI)
 * 4 - MC68681 DUART COM3/4
 * 5 - MC68681 DUART COM1/2, keyboard data ready
 * 6 - WD37C65 floppy controller
 * 7 - spare (NMI, use with caution)
 *
 * Oddly, the ATA /INTRQ signal is not routed from either the onboard
 * interface, or the PTI XTIDE card, meaning that they are not suitable
 * for use with any device with unpredictable latency.
 *
 * Interrupts 3-7 may be allocated to ISA IRQs by jumper setting.
 */

#include "emutos.h"
#ifdef MACHINE_PT68K5
#include "asm.h"
#include "delay.h"
#include "screen.h"
#include "pt68k5.h"
#include "bios.h"
#include "../bdos/bdosstub.h"
#include "ikbd.h"
#include "clock.h"
#include "machine.h"
#include "vectors.h"
#include "tosvars.h"
#include "duart68681.h"


/* MK48T02 battery-backed RAM and RTC */
#define RTC_REG(_x)     *(volatile UBYTE *)(0x20000000UL + _x)
#define RTC_CONTROL     0x7f8
#define RTC_CONTROL_WRITE   0x80
#define RTC_CONTROL_READ    0x40
#define RTC_SECOND      0x7f9
#define RTC_MINUTE      0x7fa
#define RTC_HOUR        0x7fb
#define RTC_DAY         0x7fc
#define RTC_DATE        0x7fd
#define RTC_MONTH       0x7fe
#define RTC_YEAR        0x7ff

#define FROM_BCD(_x)    ((((_x) >> 4)) * 10 + ((_x) & 0x0f))
#define TO_BCD(_x)      (((_x) / 10 << 4) | ((_x) % 10))

LONG gettime(void)
{
    UWORD second;
    UWORD minute;
    UWORD hour;
    UWORD mday;
    UWORD month;
    UWORD year;
    UWORD date;
    UWORD time;

    RTC_REG(RTC_CONTROL) |= RTC_CONTROL_READ;
    second = FROM_BCD(RTC_REG(RTC_SECOND));
    minute = FROM_BCD(RTC_REG(RTC_MINUTE));
    hour = FROM_BCD(RTC_REG(RTC_HOUR));
    mday = FROM_BCD(RTC_REG(RTC_DATE));
    month = FROM_BCD(RTC_REG(RTC_MONTH));
    year = FROM_BCD(RTC_REG(RTC_YEAR)) + 20;
    RTC_REG(RTC_CONTROL) &= ~RTC_CONTROL_READ;

    time = (hour << 11) | (minute << 5) | (second >> 1);
    date = (year << 9) | (month << 5) | mday;

    return MAKE_ULONG(date, time);
}

void settime(LONG time)
{
    /* Update GEMDOS time and date */
    current_time = LOWORD(time);
    current_date = HIWORD(time);

    RTC_REG(RTC_CONTROL) |= RTC_CONTROL_WRITE;
    RTC_REG(RTC_SECOND) = TO_BCD((current_time << 1) & 0x3f);
    RTC_REG(RTC_MINUTE) = TO_BCD((current_time >> 5) & 0x3f);
    RTC_REG(RTC_HOUR) = TO_BCD(current_time >> 11);
    RTC_REG(RTC_DATE) = TO_BCD(current_date & 0x1f);
    RTC_REG(RTC_MONTH) = TO_BCD((current_date >> 5) & 0x0f);
    RTC_REG(RTC_YEAR) = TO_BCD((current_date >> 9) - 20);
    RTC_REG(RTC_CONTROL) &= RTC_CONTROL_WRITE;
}

/*
 * Use the COM1/2 DUART for mouse port & VBL emulation; the generic
 * DUART driver is managing the COM3/4 DUART and uses that timer for
 * the 200Hz tick.
 */
#define DUART_REG(_x)   *(volatile UBYTE *)(0x20004000UL + (_x))

/*
 * XT keyboard interface registers.
 */
#define KBD_DATA        *(volatile UBYTE *)0x20004101
#define KBD_ACK         *(volatile UBYTE *)0x20004100

/* scancode translation table, no mappings for: * kp (, kp ) */
static const UBYTE xt_to_ikbd[128][2] = {
    /* norm, numlock */
    { 0x00, 0x00 },
    { 0x01, 0x01 },     /* esc */
    { 0x02, 0x02 },     /* 1 */
    { 0x03, 0x03 },     /* 2 */
    { 0x04, 0x04 },     /* 3 */
    { 0x05, 0x05 },     /* 4 */
    { 0x06, 0x06 },     /* 5 */
    { 0x07, 0x07 },     /* 6 */
    { 0x08, 0x08 },     /* 7 */
    { 0x09, 0x09 },     /* 8 */
    { 0x0a, 0x0a },     /* 9 */
    { 0x0b, 0x0b },     /* 0 */
    { 0x0c, 0x0c },     /* - */
    { 0x0d, 0x0d },     /* = */
    { 0x0e, 0x0e },     /* backspace */
    { 0x0f, 0x0f },     /* tab */
    { 0x10, 0x10 },     /* q */
    { 0x11, 0x11 },     /* w */
    { 0x12, 0x12 },     /* e */
    { 0x13, 0x13 },     /* r */
    { 0x14, 0x14 },     /* t */
    { 0x15, 0x15 },     /* y */
    { 0x16, 0x16 },     /* u */
    { 0x17, 0x17 },     /* i */
    { 0x18, 0x18 },     /* o */
    { 0x19, 0x19 },     /* p */
    { 0x1a, 0x1a },     /* [ */
    { 0x1b, 0x1b },     /* ] */
    { 0x1c, 0x1c },     /* enter */
    { 0x1d, 0x1d },     /* lctrl */
    { 0x1e, 0x1e },     /* a */
    { 0x1f, 0x1f },     /* s */
    { 0x20, 0x20 },     /* d */
    { 0x21, 0x21 },     /* f */
    { 0x22, 0x22 },     /* g */
    { 0x23, 0x23 },     /* h */
    { 0x24, 0x24 },     /* j */
    { 0x25, 0x25 },     /* k */
    { 0x26, 0x26 },     /* l */
    { 0x27, 0x27 },     /* ; */
    { 0x28, 0x28 },     /* ' */
    { 0x29, 0x29 },     /* ` */
    { 0x2a, 0x2a },     /* lshift */
    { 0x2b, 0x2b },     /* \ */
    { 0x2c, 0x2c },     /* z */
    { 0x2d, 0x2d },     /* x */
    { 0x2e, 0x2e },     /* c */
    { 0x2f, 0x2f },     /* v */
    { 0x30, 0x30 },     /* b */
    { 0x31, 0x31 },     /* n */
    { 0x32, 0x32 },     /* m */
    { 0x33, 0x33 },     /* , */
    { 0x34, 0x34 },     /* . */
    { 0x35, 0x35 },     /* / */
    { 0x36, 0x36 },     /* rshift */
    { 0x65, 0x65 },     /* kp * */
    { 0x38, 0x38 },     /* lalt */
    { 0x39, 0x39 },     /* space */
    { 0x3a, 0x3a },     /* caps lock */
    { 0x3b, 0x3b },     /* f1 */
    { 0x3c, 0x3c },     /* f2 */
    { 0x3d, 0x3d },     /* f3 */
    { 0x3e, 0x3e },     /* f4 */
    { 0x3f, 0x3f },     /* f5 */
    { 0x40, 0x40 },     /* f6 */
    { 0x41, 0x41 },     /* f7 */
    { 0x42, 0x42 },     /* f8 */
    { 0x43, 0x43 },     /* f9 */
    { 0x44, 0x44 },     /* f10 */
    { 0x00, 0x00 },     /* num lock -> special handling on press */
    { 0x62, 0x62 },     /* scroll lock -> help */
    { 0x47, 0x67 },     /* home,        kp 7 */
    { 0x48, 0x68 },     /* up,          kp 8 */
    { 0x00, 0x69 },     /* pgup,        kp 9 */
    { 0x4a, 0x4a },     /*              kp - */
    { 0x4b, 0x6a },     /* left,        kp 4 */
    { 0x00, 0x6b },     /*              kp 5 */
    { 0x4d, 0x6c },     /* right,       kp 6 */
    { 0x4e, 0x4e },     /*              kp + */
    { 0x61, 0x6d },     /* end -> undo, kp 1 */
    { 0x50, 0x6e },     /* down,        kp 2 */
    { 0x00, 0x6f },     /* pgdn,        kp 3 */
    { 0x52, 0x70 },     /* ins,         kp 0 */
    { 0x53, 0x71 },     /* del,         kp . */
};

static void pt68k5_kbd_intr(void)
{
    static UBYTE flags;
    UBYTE release;

#define FLG_NUMLOCK 0x01

    /* get the scan code from the shift register */
    UBYTE code = KBD_DATA;

    /* ack the byte and allow the keyboard to send another */
    KBD_ACK = 0;

    KDEBUG(("kbd 0x%02x\n", code));

    /* it's possible we might see one of these if the AT2XT adapter is mis-configured - ignore it */
    if (code == 0xe0) {
        return;
    }

    /* NumLock affects how we should handle the keypad codes */
    if (code == 0x45) {
        flags ^= FLG_NUMLOCK;
        KDEBUG(("flags 0x%02x\n", flags));
        return;
    }

    /* mask off the release bit */
    release = code & 0x80;
    code &= 0x7f;

    /* translate the code */
    code = xt_to_ikbd[code][flags];

    /* if the code translated to something, forward it */
    if (code) {
        KDEBUG(("kbd_int 0x%02x\n", code | release));
        kbd_int(code | release);
    }

}

static void pt68k5_mouse_mousesys_intr(SBYTE data)
{
    static SBYTE phase;
    static SBYTE packet[3];

    KDEBUG(("ms %02x %02x\n", phase, data));

    /*
     * Map left/middle/right bits from Mouse Systems sync byte to mousevec button bits.
     * Middle mouse button would be reported as joystick 0 up if we were to support it.
     */
    static const UBYTE packet_table[8] = {
        0xf8 | 0x3, /* LMR : LR */
        0xf8 | 0x2, /* LM- : L- */
        0xf8 | 0x3, /* L-R : LR */
        0xf8 | 0x2, /* L-- : L- */
        0xf8 | 0x1, /* -MR : -R */
        0xf8 | 0x0, /* -M- : -- */
        0xf8 | 0x1, /* --R : -R */
        0xf8 | 0x0, /* --- : -- */
    };

    /*
     * Mouse systems 5-byte protocol.
     */
    switch(phase) {
    default:
        /* expecting a sync byte */
        if ((data & 0xf8) != 0x80) {
            /* not a sync byte */
            return;
        }
        packet[0] = packet_table[data & 0x7];
        phase = 1;
        break;
    case 1:
    case 3:
        /* we got the first X delta byte */
        packet[1] = data;
        phase++;
        break;
    case 2:
    case 4:
        /* we have a Y delta byte, send a packet */
        packet[2] = -data;
        KDEBUG(("mv %d %d\n", packet[1], packet[2]));
        call_mousevec(packet);
        phase++;
        break;
    }
}

static void pt68k5_mouse_microsoft_intr(SBYTE data)
{
    static SBYTE phase;
    static SBYTE packet[3];

    /*
     * Microsoft 3-byte protocol. Should work with Logitech 4-byte extension
     * but the middle-button data will be ignored.
     */
    switch (phase) {
    default:
        /* expecting a sync byte */
        if ((data & 0x40) != 0x40) {
            /* not a sync byte */
            return;
        }
        packet[0] = 0xf8 | ((data >> 4) & 0x3);
        packet[1] = (data << 6) & 0xc0;
        packet[2] = (data << 4) & 0xc0;
        phase = 1;
        break;
    case 1:
        packet[1] = (packet[1] & 0xc0) | (data & 0x3f);
        phase = 2;
        break;
    case 2:
        packet[2] = (packet[2] & 0xc0) | (data & 0x3f);
        call_mousevec(packet);
        phase = 0;
        break;
    }
}

static void (*mousevec)(SBYTE);

__attribute__((interrupt))
static void pt68k5_kbdif_intr(void)
{
    UBYTE isr = DUART_REG(DUART_ISR);

    /*
     * Check for a level change on IP2, with the current pin state being low (asserted).
     * It is likely that we will get a spurious level change interrupt every time
     * we clear the keyboard receive shift register.
     */
    if (isr & DUART_IMR_INPUT_CHG) {
        UBYTE ipcr = DUART_REG(DUART_IPCR);

        if ((ipcr & 0x44) == 0x40) {
            pt68k5_kbd_intr();
        }
    }

    if (isr & DUART_IMR_RXRDY_B) {
        mousevec(DUART_REG(DUART_RHRB));
    }

    if (isr & DUART_IMR_COUNTER_READY) {
        /* call pseudo-VBL handler */
        int_vbl();

        /* ack the timer interrupt */
        (void)DUART_REG(DUART_STOPCTR);
    }
}

#if CONF_PT68K5_KBD_MOUSE
void pt68k5_kbd_init(void)
{
    /* can't do this with OVERRIDEABLE for layering reasons */
    UBYTE count;

    DUART_REG(DUART_OPCR) = 0;
    DUART_REG(DUART_CLROPR) = 0xff;     /* might need a ~100ms delay here to allow mouse to reset */
    DUART_REG(DUART_IMR) = 0;
    (void)DUART_REG(DUART_ISR);
    (void)DUART_REG(DUART_IPCR);

    /* disable channel A, it's not used */
    DUART_REG(DUART_CRA) = DUART_CR_TX_DISABLED | DUART_CR_RX_DISABLED;

    /* standard baud rate set, timer mode, x16 prescaler, change interrupt on IP2 */
    DUART_REG(DUART_ACR) = 0x74;

    /* reset channel B */
    DUART_REG(DUART_CRB) = DUART_CR_TX_DISABLED | DUART_CR_RX_DISABLED;
    DUART_REG(DUART_CRB) = DUART_CR_RESET_TX;
    DUART_REG(DUART_CRB) = DUART_CR_RESET_RX;
    DUART_REG(DUART_CRB) = DUART_CR_RESET_ERROR;
    DUART_REG(DUART_CRB) = DUART_CR_BKCHGINT;

    /* configure timer for ~60Hz to emulate vertical blanking interrupt, as IRQ9 is not available    */
    DUART_REG(DUART_CTLR) = 0x55;
    DUART_REG(DUART_CTUR) = 0x07;

    /* configure channel B for Microsoft serial mouse @ 1200n71 */
    DUART_REG(DUART_CSRB) = 0x66;
    DUART_REG(DUART_CRB) = DUART_CR_RESET_MR;
    DUART_REG(DUART_MRB) = DUART_MR_PM_NONE | DUART_MR_BC_7;
    DUART_REG(DUART_MRB) = DUART_MR_CM_NORMAL | DUART_MR_SB_STOP_BITS_1;
    DUART_REG(DUART_CRB) = DUART_CR_RX_ENABLED;

    /* turn on RTS / DTR to power up the mouse - expect MS mouse to send 0x45 at power-on */
    DUART_REG(DUART_SETOPR) = 0x0a;   /* OP1 = RTS, OP3 = DTR */

    /* give the mouse a little time to sign on */
    for (count = 0; count < 10; count++) {
        delay_loop(loopcount_1_msec * 10);
        if ((DUART_REG(DUART_ISR) & DUART_IMR_RXRDY_B) && (DUART_REG(DUART_RHRB) == 'M')) {
            KDEBUG(("Microsoft-compatible mouse detected\n"));
            mousevec = pt68k5_mouse_microsoft_intr;
            break;
        }
    }
    if (mousevec != pt68k5_mouse_microsoft_intr) {
        mousevec = pt68k5_mouse_mousesys_intr;

        /* configure channel B for Mouse Systems serial mouse @ 1200n81 */
        DUART_REG(DUART_CRB) = DUART_CR_TX_DISABLED | DUART_CR_RX_DISABLED;
        DUART_REG(DUART_CRB) = DUART_CR_RESET_RX;
        DUART_REG(DUART_CRB) = DUART_CR_BKCHGINT;
        DUART_REG(DUART_CSRB) = 0x66;
        DUART_REG(DUART_CRB) = DUART_CR_RESET_MR;
        DUART_REG(DUART_MRB) = DUART_MR_PM_NONE | DUART_MR_BC_8;
        DUART_REG(DUART_MRB) = DUART_MR_CM_NORMAL | DUART_MR_SB_STOP_BITS_1;
        DUART_REG(DUART_CRB) = DUART_CR_RX_ENABLED;
    }

    /* attach interrupt handler */
    VEC_LEVEL5 = pt68k5_kbdif_intr;

    /* enable interrupts */
    DUART_REG(DUART_IMR) = DUART_IMR_RXRDY_B | DUART_IMR_COUNTER_READY | DUART_IMR_INPUT_CHG;

    /* discard any pending keyboard byte */
    (void)*(volatile UBYTE *)0x20004100;

    /* disable key repeat, as the keyboard already does it for us */
    conterm &= ~2;

}
#endif /* CONF_PT68K5_KBD_MOUSE */

void screen_init_mode(void)
{
    /* more or less correct */
    sshiftmod = ST_HIGH;
    defshiftmod = ST_HIGH;
}

void screen_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez)
{
    *planes = 1;
    *hz_rez = 640;
    *vt_rez = 480;
}

/*
 * Override machine_name and return something appropriate.
 */
const char *
machine_name(void)
{
    return "PT68K5";
}


#endif /* MACHINE_PT68K5 */

