/*
 * q800.c - Quadra 800 specific functions.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"
#ifdef MACHINE_Q800
#include "asm.h"
#include "blkdev.h"
#include "clock.h"
#include "delay.h"
#include "disk.h"
#include "gemerror.h"
#include "q800.h"
#include "ikbd.h"
#include "machine.h"
#include "processor.h"
#include "screen.h"
#include "serport.h"
#include "string.h"
#include "tosvars.h"
#include "vectors.h"
#include "cookie.h"

#define ESCC_BASE       0x5000c020UL
#define SCC_CSR         (*(volatile UBYTE *)(ESCC_BASE))
#define SCC_DATA        (*(volatile UBYTE *)(ESCC_BASE + 4))

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
 * Override machine_name and return something appropriate.
 */
const char *machine_name(void)
{
    return "IP940";
}

/*
 * Override processor.S:processor_init, which does unhelpful things
 * like messing with the TTRs. I/O is in different places on the Q800.
 */
void processor_init(void)
{
    /* standard values for the '040 */
    longframe = 1;
    mcpu = 40;
    fputype = 0x00080000;

    /*
     * Set up the TTRs in a way that makes sense for us.
     *
     * The instruction cache is always enabled, and all memory is cacheable by it.
     *
     * For compatibility, the data cache is enabled in user mode, but disabled in supervisor mode.
     * XXX This could perhaps be revisited.
     */
    __asm__ volatile("    movec %0,itt0     \n"
                     : : "d"(0x00ffe000)   /* all memory cacheable, user/supervisor */
                     : "memory");
    __asm__ volatile("    movec %0,itt1     \n"
                     : : "d"(0)             /* disable ITT1 */
                     : "memory");
    __asm__ volatile("    movec %0,dtt0     \n"
                     : : "d"(0x00ffa040)    /* all memory cache-inhibited, serialized, supervisor */
                     : "memory");
    __asm__ volatile("    movec %0,dtt1     \n"
                     : : "d"(0x00ff8000)    /* all memory cache-enabled, write-through, user */
                     : "memory");

    /*
     * Enable the caches.
     */
    __asm__ volatile("    nop               \n"
                     "    cinva bc          \n"
                     "    nop               \n"
                     "    movec %0,cacr     \n"
                     : : "d"(0x80008000) : "memory");

    /*
     * Poke the ESCC emulation just enough to convince it to output
     * what we write to it. This would never work on real hardware...
     */
    SCC_CSR = 0x05;     /* select TXCTRL2 */
    SCC_CSR = 0x08;     /* set TXEN */
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
 * Override kprintf_outc and talk directly to the ESCC.
 */
void kprintf_outc(int c)
{
    if (c == '\n') {
        kprintf_outc('\r');
    }

    SCC_DATA = c;
}


#endif /* MACHINE_Q800 */