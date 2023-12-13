/*
 * pt68k5.h - PT68K5 specific functions.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef PT68K5_H
#define PT68K5_H

#ifdef MACHINE_PT68K5

ULONG pt68k5_getdt(void);
void pt68k5_setdt(ULONG time);
void pt68k5_kbd_init(void);
void pt68k5_screen_init(void);

#endif /* MACHINE_PT68K5 */
#endif /* PT68K5.H */
