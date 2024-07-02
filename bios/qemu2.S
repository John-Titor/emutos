/*
 * Assembly shims and support for the QEMU machine.
 */

#include "asmdefs.h"

#ifdef MACHINE_QEMU

/*
 * QEMU doesn't do vectored interrupts, so we emulate them.
 */
    .globl  _qemu_mfp_int
    .extern _qemu_mfp_revector
_qemu_mfp_int:
    sub.l   #20,sp                  /* space for regs & forwarding address */
    movem.l d0-d1/a0-a1,(sp)        /* caller-saved regs */
    movec   vbr,d0                  /* get VBR */
    move.l  d0,-(sp)                /* ... and pass to revector code */
    jsr     _qemu_mfp_revector      /* get actual handler */
    addq.l  #4,sp
    move.l  d0,16(sp)               /* handler -> placeholder */
    movem.l (sp)+,d0-d1/a0-a1       /* restore regs */
    rts                             /* jump to forwarding address */

/*
 * VBL handler wrapper; we need to pacify the hardware before calling
 * the handler proper.
 */
    .globl  _qemu_vbl_shim
_qemu_vbl_shim:
    move.l  #0,0xffffc000           /* REG_VBL_ACK = 0 */
    subq.l  #8,sp                   /* scratch reg + forwarding address */
    move.l  a0,(sp)                 /* save scratch reg */
    movec   vbr,a0                  /* vector base */
    move.l  0x70(a0),a0             /* level 4 handler */
    move.l  a0,4(sp)                /* save to forwarding address */
    move.l  (sp)+,a0                /* restore scratch reg */
    rts                             /* call level 4 handler */

/*
 * Default VBL handler; just calls int_vbl().
 */
    .extern _int_vbl
    .globl  _qemu_vbl
_qemu_vbl:
    jbsr    _int_vbl                /* returns with RTS for !CONF_WITH_ATARI_VIDEO */
    rte

/*
 * PCI function wrappers.
 *
 * The _PCI cookie vectors use the PureC calling convention...
 * 
 * TODO: optimize register save/restores.
 */

#define WRAP_(_x)                 \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    jsr        _qemu_pci_##_x    ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_L(_x)                \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.l     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    addq.l     #4,sp             ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_LW(_x)               \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.w     d1,-(sp)          ;\
    move.l     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    addq.l     #6,sp             ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_WL(_x)               \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.l     d1,-(sp)          ;\
    move.w     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    addq.l     #6,sp             ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_LL(_x)               \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.l     d1,-(sp)          ;\
    move.l     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    addq.l     #8,sp             ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_LA(_x)               \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.l     a0,-(sp)          ;\
    move.l     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    addq.l     #8,sp             ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_LWA(_x)              \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.l     a0,-(sp)          ;\
    move.w     d1,-(sp)          ;\
    move.l     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    add.l      #10,sp            ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_LWL(_x)              \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.l     d2,-(sp)          ;\
    move.w     d1,-(sp)          ;\
    move.l     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    add.l      #10,sp            ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_LLW(_x)              \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.w     d2,-(sp)          ;\
    move.l     d1,-(sp)          ;\
    move.l     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    add.l      #10,sp            ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_LLL(_x)              \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.l     d2,-(sp)          ;\
    move.l     d1,-(sp)          ;\
    move.l     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    add.l      #12,sp            ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_LLA(_x)              \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.l     a0,-(sp)          ;\
    move.l     d1,-(sp)          ;\
    move.l     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    add.l      #12,sp            ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

#define WRAP_LAA(_x)              \
    .extern _qemu_pci_##_x       ;\
wrap_pci_##_x:                   ;\
    movem.l    d1/a0-a1,-(sp)    ;\
    move.l     a1,-(sp)          ;\
    move.l     a0,-(sp)          ;\
    move.l     d0,-(sp)          ;\
    jsr        _qemu_pci_##_x    ;\
    add.l      #12,sp            ;\
    movem.l    (sp)+,d1/a0-a1    ;\
    rts

WRAP_LW(find_pci_device)
WRAP_LW(find_pci_classcode)
WRAP_LWA(read_config_byte)
WRAP_LWA(read_config_word)
WRAP_LWA(read_config_longword)
WRAP_LW(fast_read_config_byte)
WRAP_LW(fast_read_config_word)
WRAP_LW(fast_read_config_longword)
WRAP_LWL(write_config_byte)
WRAP_LWL(write_config_word)
WRAP_LLL(write_config_longword)
WRAP_LAA(hook_interrupt)
WRAP_L(unhook_interrupt)
WRAP_WL(special_cycle)
WRAP_L(get_routing)
WRAP_L(set_interrupt)
WRAP_L(get_resource)
WRAP_LA(get_card_used)
WRAP_LA(set_card_used)
WRAP_LLA(read_mem_byte)
WRAP_LLA(read_mem_word)
WRAP_LLA(read_mem_longword)
WRAP_LL(fast_read_mem_byte)
WRAP_LL(fast_read_mem_word)
WRAP_LL(fast_read_mem_longword)
WRAP_LLW(write_mem_byte)
WRAP_LLW(write_mem_word)
WRAP_LLL(write_mem_longword)
WRAP_LLA(read_io_byte)
WRAP_LLA(read_io_word)
WRAP_LLA(read_io_longword)
WRAP_LL(fast_read_io_byte)
WRAP_LL(fast_read_io_word)
WRAP_LL(fast_read_io_longword)
WRAP_LLW(write_io_byte)
WRAP_LLW(write_io_word)
WRAP_LLL(write_io_longword)
WRAP_(get_machine_id)
WRAP_(get_pagesize)
WRAP_LLA(virt_to_bus)
WRAP_LLA(bus_to_virt)
WRAP_LA(virt_to_phys)
WRAP_LA(phys_to_virt)

    .globl _qemu_pci_dispatch_table
_qemu_pci_dispatch_table:
    dc.l       0
    dc.l       0x00010000
    dc.l       wrap_pci_find_pci_device
    dc.l       wrap_pci_find_pci_classcode
    dc.l       wrap_pci_read_config_byte
    dc.l       wrap_pci_read_config_word
    dc.l       wrap_pci_read_config_longword
    dc.l       wrap_pci_fast_read_config_byte
    dc.l       wrap_pci_fast_read_config_word
    dc.l       wrap_pci_fast_read_config_longword
    dc.l       wrap_pci_write_config_byte
    dc.l       wrap_pci_write_config_word
    dc.l       wrap_pci_write_config_longword
    dc.l       wrap_pci_hook_interrupt
    dc.l       wrap_pci_unhook_interrupt
    dc.l       wrap_pci_special_cycle
    dc.l       wrap_pci_get_routing
    dc.l       wrap_pci_set_interrupt
    dc.l       wrap_pci_get_resource
    dc.l       wrap_pci_get_card_used
    dc.l       wrap_pci_set_card_used
    dc.l       wrap_pci_read_mem_byte
    dc.l       wrap_pci_read_mem_word
    dc.l       wrap_pci_read_mem_longword
    dc.l       wrap_pci_fast_read_mem_byte
    dc.l       wrap_pci_fast_read_mem_word
    dc.l       wrap_pci_fast_read_mem_longword
    dc.l       wrap_pci_write_mem_byte
    dc.l       wrap_pci_write_mem_word
    dc.l       wrap_pci_write_mem_longword
    dc.l       wrap_pci_read_io_byte
    dc.l       wrap_pci_read_io_word
    dc.l       wrap_pci_read_io_longword
    dc.l       wrap_pci_fast_read_io_byte
    dc.l       wrap_pci_fast_read_io_word
    dc.l       wrap_pci_fast_read_io_longword
    dc.l       wrap_pci_write_io_byte
    dc.l       wrap_pci_write_io_word
    dc.l       wrap_pci_write_io_longword
    dc.l       wrap_pci_get_machine_id
    dc.l       wrap_pci_get_pagesize
    dc.l       wrap_pci_virt_to_bus
    dc.l       wrap_pci_bus_to_virt
    dc.l       wrap_pci_virt_to_phys
    dc.l       wrap_pci_phys_to_virt

/*
 *  2.5.  Interrupt Handlers
 *
 *  For each hardware interrupt, there is a chain of interrupt handlers,
 *  as multiple cards can share the same interrupt.
 *
 * [...]
 *
 *  When an interrupt occurs, the driver is called with a value in A0
 *  which is set by the driver when hooking into the interrupt. The
 *  meaning of this parameter depends on the driver - it can be a device
 *  handle, a pointer to some driver-internal data structure, etc..
 *
 *       ______________________________________________________________________
 *       interrupt_handler:
 *       Input:
 *         A0.L   value as passed to hook_interrupt
 *         D0.L   BIOS internal data
 *       Output:
 *         D0.L   bit 0 set if the interrupt was from this card. D0 unmodified
 *                otherwise.
 *       ______________________________________________________________________
 *
 *  The interrupt handler must not modify any register except D0 (only as
 *  specified) and A0.  If the interrupt was caused by the card which this
 *  handler is monitoring, it must make sure that the card returns the
 *  interrupt line to inactive state, and return with D0.0 set.
 *
 *  If the card did not cause the interrupt, the driver may not modify D0.
 *
 *  The interrupt handler returns by using an RTS instruction.
 */
    .extern _qemu_pci_interrupt_handlers
    .extern _qemu_pci_spurious
    .globl _qemu_pci_interrupt
_qemu_pci_interrupt:
    movem.l     d0/a0-a2,-(sp)
    lea         _qemu_pci_interrupt_handlers,a2
    clr.l       d0
pci_nextvec:
    movem.l     (a2)+,a0-a1         /* arg first, then handler */
    cmpa.l      #0,a1               /* handler=0 is a free slot */
    beq         pci_nextvec
    cmpa.l      #1,a1               /* handler=1 is the end of the list */
    beq         pci_done
    jsr         (a1)
    bra         pci_nextvec

pci_done:
    tst.l       d0
    beq         pci_return
    jsr         _qemu_pci_spurious
pci_return:
    movem.l     (sp)+,d0/a0-a2
    rte


#endif /* MACHINE_QEMU */
