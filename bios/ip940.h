/*
 * ip940.h - IP940 specific functions.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef IP940_H
#define IP940_H

#ifdef MACHINE_IP940

#define TIMER_STOP      *(volatile UBYTE *)0x0210003b  // any access stops CPLD timers
#define TIMER_START     *(volatile UBYTE *)0x0210003f  // any access starts CPLD timers

#define QUART_BASE      0x02110003L
#define QUART_STRIDE    (8 << 2)
#define QUART_THR       (0x00<<2)
#define QUART_RHR       (0x00<<2)
#define QUART_DLL       (0x00<<2)
#define QUART_DLM       (0x01<<2)
#define QUART_FCR       (0x02<<2)
#define QUART_EFR       (0x02<<2)
#define QUART_LCR       (0x03<<2)
#define QUART_LSR       (0x05<<2)
#define QUART_LSR_TXRDY     (1<<5)
#define QUART_LSR_RXRDY     (1<<0)
#define QUART_ICR       (0x05<<2)
#define QUART_SPR       (0x07<<2)


#endif /* MACHINE_IP940 */
#endif /* IP940.H */
