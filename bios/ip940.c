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
#include "ip940.h"
#include "clock.h"
#include "processor.h"
#include "screen.h"
#include "tosvars.h"

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
    mcpu = 0x00400000;
    fputype = 0x00080000;
}

#endif /* MACHINE_IP940 */
