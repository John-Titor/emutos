/*
 * qemu-system-m68k -machine type=atarist
 *
 * TODO:
 * - Disable auto-repeat; can it be done in QEMU? can we do it 'smart' in the keyboard driver?
 * - deskrez.c video mode selection
 *
 * BUGS:
 * - Without CONF_STRAM_SIZE, ST RAM sizes as zero
 */

#include "emutos.h"
#include "qemu.h"
#include "biosext.h"
#include "machine.h"
#include "mfp.h"
#include "vectors.h"
#include "cookie.h"

#ifdef MACHINE_QEMU

const char * machine_name(void)
{
    return "QEMU";
}

/********************************************************************
 * QEMU control register.
 */

#define VIRT_CTRL_BASE    0xffffb500UL

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

    /* high 8 vectors */
    pending = MFP_BASE->ipra & MFP_BASE->imra;
    vec = __ffs(pending);
    if (vec) {
        vec--;
        ack = ~(1 << vec);
        MFP_BASE->ipra = ack;
        return *(vbr + 0x48 + vec);
    }

    /* low 8 vectors */
    pending = MFP_BASE->iprb & MFP_BASE->imrb;
    vec = __ffs(pending);
    if (vec) {
        vec--;
        ack = ~(1 << vec);
        MFP_BASE->iprb = ack;
        return *(vbr + 0x40 + vec);
    }

    KDEBUG(("mfp: spurious %lu\n", vec));
    return just_rte;
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
}

/********************************************************************
 * Add machine-specific cookies.
 */
void machine_add_cookies(void)
{
    qemu_pci_add_cookies();
    qemu_video_add_cookies();
}
#endif /* MACHINE_QEMU */

