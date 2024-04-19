/*
 * ip940.h - IP940 specific functions.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef IP940_H
#define IP940_H

#ifdef MACHINE_IP940

#define RAM_PHYS			0x01000000UL
#define ROMDISK_PHYS		0x00080000UL
#define ROMDISK_SIZE		0x00180000UL

#define TIMER_STOP          *(volatile UBYTE *)0x0210003bUL	// any access stops CPLD timers
#define TIMER_START         *(volatile UBYTE *)0x0210003fUL	// any access starts CPLD timers
#define CPLD_REVISION		*(volatile UBYTE *)0x021000ffUL

#define QUART_BASE      	0x02110003UL
#define QUART_REG_STRIDE	(4)
#define QUART_PORT_STRIDE	(8 * QUART_REG_STRIDE)
#define QUART_REG(_port, _reg)	*(volatile UBYTE *)(QUART_BASE + ((_port) * QUART_PORT_STRIDE) + ((_reg) * QUART_REG_STRIDE))

#define QUART_THR       	(0)
#define QUART_RHR       	(0)
#define QUART_DLL       	(0)
#define QUART_DLM       	(1)
#define QUART_IER           (1)
#define QUART_IER_RXRDY         (1<<0)
#define QUART_FCR       	(2)
#define QUART_FCR_FIFOEN	    (1<<0)
#define QUART_EFR       	(2)
#define QUART_EFR_CTS   	    (1<<7)
#define QUART_EFR_RTS   	    (1<<6)
#define QUART_EFR_ENHMODE   	(1<<4)
#define QUART_LCR       	(3)
#define QUART_LCR_EXTEN 	    (0xbf)
#define QUART_LCR_DLAB  	    (1<<7)
#define QUART_LCR_BRK           (1<<6)
#define QUART_LCR_NOPAR         (0<<3)
#define QUART_LCR_PARODD        (1<<3)
#define QUART_LCR_PAREVEN       (2<<3)
#define QUART_LCR_STOP1         (0<<2)
#define QUART_LCR_STOP2         (1<<2)
#define QUART_LCR_DATA5         (0<<0)
#define QUART_LCR_DATA6         (1<<0)
#define QUART_LCR_DATA7         (2<<0)
#define QUART_LCR_DATA8         (3<<0)
#define QUART_MCR       	(4)
#define QUART_MCR_INTEN 	    (1<<3)
#define QUART_MCR_PRESCALE  	(1<<7)
#define QUART_LSR       	(5)
#define QUART_LSR_TXEMT  	    (1<<6)
#define QUART_LSR_THRE  	    (1<<5)
#define QUART_LSR_BREAK 	    (1<<4)
#define QUART_LSR_RXRDY 	    (1<<0)
#define QUART_ICR       	(5)
#define QUART_SPR       	(7)

#define QUART_INDEXED   	0x80
#define QUART_ACR       	0x80
#define QUART_ACR_RDEN  	    (1<<6)
#define QUART_CPR       	0x81
#define QUART_TCR       	0x82

#endif /* MACHINE_IP940 */
#endif /* IP940.H */
