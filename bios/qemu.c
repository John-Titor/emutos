/*
 * qemu-system-m68k -machine type=atarist
 *
 * TODO:
 * - Disable auto-repeat; can it be done in QEMU? can we do it 'smart' in the keyboard driver?
 *
 * BUGS:
 * - Without CONF_STRAM_SIZE, ST RAM sizes as zero
 */

#include "emutos.h"
#ifdef MACHINE_QEMU
#include "qemu.h"
#include "bios.h"
#include "biosext.h"
#include "machine.h"
#include "mfp.h"
#include "vectors.h"
#include "cookie.h"

typedef struct
{
        UBYTE   dum1;
        volatile UBYTE  gpip;   /* general purpose .. register */
        UBYTE   dum2;
        volatile UBYTE  aer;    /* active edge register              */
        UBYTE   dum3;
        volatile UBYTE  ddr;    /* data direction register           */
        UBYTE   dum4;
        volatile UBYTE  iera;   /* interrupt enable register A       */
        UBYTE   dum5;
        volatile UBYTE  ierb;   /* interrupt enable register B       */
        UBYTE   dum6;
        volatile UBYTE  ipra;   /* interrupt pending register A      */
        UBYTE   dum7;
        volatile UBYTE  iprb;   /* interrupt pending register B      */
        UBYTE   dum8;
        volatile UBYTE  isra;   /* interrupt in-service register A   */
        UBYTE   dum9;
        volatile UBYTE  isrb;   /* interrupt in-service register B   */
        UBYTE   dum10;
        volatile UBYTE  imra;   /* interrupt mask register A         */
        UBYTE   dum11;
        volatile UBYTE  imrb;   /* interrupt mask register B         */
        UBYTE   dum12;
        volatile UBYTE  vr;     /* vector register                   */
        UBYTE   dum13;
        volatile UBYTE  tacr;   /* timer A control register          */
        UBYTE   dum14;
        volatile UBYTE  tbcr;   /* timer B control register          */
        UBYTE   dum15;
        volatile UBYTE  tcdcr;  /* timer C + D control register      */
        UBYTE   dum16;
        volatile UBYTE  tadr;   /* timer A data register             */
        UBYTE   dum17;
        volatile UBYTE  tbdr;   /* timer B data register             */
        UBYTE   dum18;
        volatile UBYTE  tcdr;   /* timer C data register             */
        UBYTE   dum19;
        volatile UBYTE  tddr;   /* timer D data register             */
        UBYTE   dum20;
        volatile UBYTE  scr;    /* synchronous character register    */
        UBYTE   dum21;
        volatile UBYTE  ucr;    /* USART control register            */
        UBYTE   dum22;
        volatile UBYTE  rsr;    /* receiver status register          */
        UBYTE   dum23;
        volatile UBYTE  tsr;    /* transmitter status register       */
        UBYTE   dum24;
        volatile UBYTE  udr;    /* USART data register               */
        UBYTE   dum25;
        volatile UBYTE  ierc;   /* interrupt enable register C       */
        UBYTE   dum26;
        volatile UBYTE  ierd;   /* interrupt enable register D       */
        UBYTE   dum27;
        volatile UBYTE  iprc;   /* interrupt pending register C      */
        UBYTE   dum28;
        volatile UBYTE  iprd;   /* interrupt pending register D      */
        UBYTE   dum29;
        volatile UBYTE  isrc;   /* interrupt in-service register C   */
        UBYTE   dum30;
        volatile UBYTE  isrd;   /* interrupt in-service register D   */
        UBYTE   dum31;
        volatile UBYTE  imrc;   /* interrupt mask register C         */
        UBYTE   dum32;
        volatile UBYTE  imrd;   /* interrupt mask register D         */
} XMFP;

#define XMFP_BASE       ((XMFP *)MFP_BASE)

const char * machine_name(void)
{
    return "QEMU";
}

/********************************************************************
 * QEMU control register.
 */

BOOL can_shutdown(void)
{
    return TRUE;
}

void shutdown(void)
{
    /* exit */
    *(volatile ULONG *)(VIRT_CTRL_BASE + 4) = 2;
    for (;;) {}
}

/********************************************************************
 * MFP vector dispatch
 *
 * QEMU doesn't support vectored interrupts, so we have to emulate
 * them here.
 */
PFVOID qemu_mfp_revector(PFVOID *vbr)
{
    ULONG   pending;
    ULONG   vec;
    UBYTE   ack;

    /* high 8 internal vectors */
    pending = MFP_BASE->ipra & MFP_BASE->imra;
    vec = __ffs(pending);
    if (vec) {
        vec--;
        ack = ~(1 << vec);
        MFP_BASE->ipra = ack;
        return *(vbr + 0x48 + vec);
    }

    /* low 8 internal vectors */
    pending = MFP_BASE->iprb & MFP_BASE->imrb;
    vec = __ffs(pending);
    if (vec) {
        vec--;
        ack = ~(1 << vec);
        MFP_BASE->iprb = ack;
        return *(vbr + 0x40 + vec);
    }

    /* ISA interrupts 0-7 */
    pending = XMFP_BASE->iprc & XMFP_BASE->imrc;
    vec = __ffs(pending);
    if (vec) {
        vec--;
        ack = ~(1 << vec);
        XMFP_BASE->iprc = ack;
        return *(vbr + 0x50 + vec);
    }

    /* ISA interrupts 8-15 */
    pending = XMFP_BASE->iprd & XMFP_BASE->imrd;
    vec = __ffs(pending);
    if (vec) {
        vec--;
        ack = ~(1 << vec);
        XMFP_BASE->iprd = ack;
        return *(vbr + 0x58 + vec);
    }

    KDEBUG(("mfp: spurious %lu\n", vec));
    return just_rte;
}

/*
 * Override setexc so that we can en/disable ISA interrupts, Milan-style.
 */
LONG setexc(WORD num, LONG vector)
{
    LONG oldvector;
    LONG *addr = (LONG *) (4L * num);
    UBYTE mask;
    oldvector = *addr;

    if(vector != -1) {
        *addr = vector;
    }

    /*
     * mask / unmask ISA interrupts
     */
    switch (num) {
    case 0x50 ... 0x57:
        mask = 1 << (num - 0x50);
        if (vector != -1) {
            XMFP_BASE->imrc |= mask;
        } else {
            XMFP_BASE->imrc &= ~mask;
        }
        break;
    case 0x58 ... 0x5f:
        mask = 1 << (num - 0x58);
        if (vector != -1) {
            XMFP_BASE->imrd |= mask;
        } else {
            XMFP_BASE->imrd &= ~mask;
        }
        break;
    }

    return oldvector;
}

/********************************************************************
 * XBIOS extensions
 */

static const PFLONG qemu_xbios_video[] = {
    (PFLONG)qemu_vsetmode,
    (PFLONG)qemu_vmontype,
    (PFLONG)qemu_vsetsync,
    (PFLONG)qemu_vgetsize,
    (PFLONG)0,
    (PFLONG)qemu_vsetrgb,
    (PFLONG)qemu_vgetrgb,
    (PFLONG)qemu_vfixmode,
};

PFLONG xbios_extlookup(UWORD fn)
{
    switch(fn) {
    case 0x58 ... 0x5f:
        return qemu_xbios_video[fn - 0x58];
        break;
    default:
        break;
    }
    return (PFLONG)0;
}

/********************************************************************
 * Override machine_init() to do our own machine-specific setup.
 */
void machine_init(void)
{
    VEC_LEVEL6 = qemu_mfp_int;

    qemu_pci_init();
    qemu_virtio_init();
}

/********************************************************************
 * Add machine-specific cookies.
 */
void machine_add_cookies(void)
{
    qemu_pci_add_cookies();
    qemu_virtio_add_cookies();
    qemu_video_add_cookies();
}

/********************************************************************
 * Override kprintf_outc() and write to the goldfish tty.
 */
void kprintf_outc(int c)
{
    if (c == '\n') {
        kprintf_outc('\r');
    }
    /* send direct to the Goldfish TTY */
    *(volatile LONG *)GF_TTY_BASE = c;
}

#endif /* MACHINE_QEMU */
