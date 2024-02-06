/*
 * QEMU PCI functionality.
 *
 * Function documentation is taken from:
 *
 *   Atari PCI BIOS and device driver specification (Draft) 1.12
 *   Michael Schwingen, Torsten Lang, Markus Fichtenbauer
 *   1998/03/30 22:07:40
 *
 * Note that the calling convention described is handled by
 * wrapper functions in qemu2.S
 */

#include "emutos.h"
#include "qemu.h"
#include "vectors.h"
#include "biosext.h"
#include "cookie.h"
#include "pcireg.h"

/* hardcode these for now, but probably want a way to determine empirically */
#define PCI_MMIO_BASE       0xd0000000UL
#define PCI_MMIO_SIZE       0x1fd00000UL
#define PCI_MMIO_END        (PCI_MMIO_BASE + PCI_MMIO_SIZE)
#define PCI_ECAM_BASE       0xffd00000UL
#define PCI_ECAM_SIZE       0x00100000UL
#define PCI_IO_BASE         0xffe00000UL
#define PCI_IO_SIZE         0x00010000UL

/* config-space-related definitions */
#define PCI_CFG_FUNCTION_SIZE   0x1000UL
#define PCI_CFG_DEVICE_SIZE     (8 * PCI_CFG_FUNCTION_SIZE)
#define PCI_CFG_BUS_SIZE        (32 * PCI_CFG_DEVICE_SIZE)
#define PCI_CFG_NUM_DEVICES     (PCI_SLOTMAX + 1)
#define PCI_CFG_NUM_FUNCTIONS   (PCI_FUNCMAX + 1)
#define PCI_CFG_NUM_RESOURCES   5       /* 5 BARs */

/* implementation constraints */
#define PCI_MAX_BUS             (PCI_ECAM_SIZE / PCI_CFG_BUS_SIZE)
#define PCI_MAX_FUNCTIONS       16
#define PCI_MAX_RESOURCES       32
#define PCI_MAX_INTERRUPTS      PCI_MAX_FUNCTIONS

/* PCI BIOS errors */
#define PCI_SUCCESSFUL          0x00000000
#define PCI_FUNC_NOT_SUPPORTED  0xfffffffe
#define PCI_BAD_VENDOR_ID       0xfffffffd
#define PCI_DEVICE_NOT_FOUND    0xfffffffc
#define PCI_BAD_REGISTER_NUMBER 0xfffffffb
#define PCI_SET_FAILED          0xfffffffa
#define PCI_BUFFER_TOO_SMALL    0xfffffff9
#define PCI_GENERAL_ERROR       0xfffffff8
#define PCI_BAD_HANDLE          0xfffffff7

#define PCI_RESULT_IS_ERROR(_x) (_x > 0xfffffff0)

/*
 *  3.11.  Get Resource Data
 *
 *  [...]
 *
 *  The resource descriptors look like this:
 *
 *       ______________________________________________________________________
 *       struct resource_descriptor:
 *         next      DS.W 1    ; length of this structure in bytes
 *                             ; Use this to get next descriptor
 *         flags     DS.W 1    ; type of resource and misc flags
 *         start     DS.L 1    ; 4 start address of resource in PCI address space
 *         length    DS.L 1    ; 8 length of resource
 *         offset    DS.L 1    ; 12 offset from PCI to physical CPU address
 *         dmaoffset DS.L 1    ; offset for DMA-Memory transfers.
 *         private   DS.B n    ; private data, used only by the PCI BIOS. Do not touch.
 *       ______________________________________________________________________
 *
 *  The flags field is bit-coded as follows:
 *
 *       ______________________________________________________________________
 *         RSC_IO      = $4000 ; This is an IO area (Memory area if bit is clear)
 *         RSC_LAST    = $8000 ; last resource for this device
 *
 *         FLG_8BIT    = $0100 ; 8-bit accesses are supported
 *         FLG_16BIT   = $0200 ; 16-bit accesses are supported
 *         FLG_32BIT   = $0400 ; 32-bit accesses are supported
 *         FLG_ENDMASK = $000F ; Bit 0..3 specify which byte ordering is used:
 *                               0 = Motorola.
 *                               1 = Intel, address-swapped
 *                               2 = Intel, lane-swapped
 *                               3..14 = reserved
 *                               15 = unknown. Access card only via BIOS functions.
 *       ______________________________________________________________________
 *
 *  The start field contains the start address on the PCI bus of the
 *  resource. If the resource is not directly accessible, the start
 *  address is 0.  The length field contains its length.
 *
 *  The offset field contains the offset from physical CPU to PCI address
 *  for the resource - ie. the value that must be added to the PCI address
 *  to get the physical address in CPU address space.
 *
 *  The dmaoffset gives the offset that must be added to a PCI address to
 *  get the physical address in CPU address space when doing DMA
 *  transfers.
 *
 *  3.11.1.  What are all these byte orders?
 *
 *  Motorola byte ordering (big endian) is what you would expect on a
 *  680x0 system: 8/16/32 bit accesses work as expected.
 *
 *  When address-swapped Intel (little endian) byte ordering is used, 32
 *  bit accesses work without modifications. On 16 bit accesses, the
 *  address needs to be XOR'd with a value of 2, on 8-bit accesses the
 *  address is XOR'd with a value of 3. The data read or written is in
 *  correct format.
 *
 *  When lane-swapped Intel (little endian) byte ordering is used, the
 *  address needs no modifications. 8-bit accesses work normal, on 16 and
 *  32 bit accesses the read or written data needs to be swapped (ror.w
 *  #8,d0 for 16 bit, ror.w d0:swap d0:ror.w d0 for 32 bit).
 */
struct resource_info
{
    UWORD   next;
    UWORD   flags;
    ULONG   start;
    ULONG   length;
    ULONG   offset;
    ULONG   dmaoffset;
};

#define RSC_IO          0x4000
#define RSC_LAST        0x8000
#define FLG_8BIT        0x0100
#define FLG_16BIT       0x0200
#define FLG_32BIT       0x0400
#define FLG_ENDMASK     0x000f

static struct resource_info pci_resources[PCI_MAX_RESOURCES];

static struct resource_info *alloc_resource(void)
{
    static UWORD next_free;

    if (next_free < PCI_MAX_RESOURCES) {
        struct resource_info *rsc = &pci_resources[next_free++];
        rsc->next = sizeof(*rsc);
        return rsc;
    }
    panic("PCI: resource slots exhausted\n");
}

static ULONG alloc_mmio(ULONG size)
{
    static ULONG next_free;
    ULONG ret;

    /* allocations must be size-aligned, so round next_free up  */
    next_free = (next_free + size - 1) & ~(size - 1);

    if ((PCI_MMIO_SIZE - next_free) < size) {
        panic("PCI: mmio space exhausted allocating 0x%08lx\n", size);
    }
    ret = PCI_MMIO_BASE + next_free;
    next_free += size;
    return ret;
}

static ULONG alloc_io(ULONG size)
{
    static ULONG next_free;
    ULONG ret;

    /* zero is an error, so skip the first 16B */
    if (next_free == 0) {
        next_free = 0x10;
    }

    /* allocations must be size-aligned, so round next_free up  */
    next_free = (next_free + size - 1) & ~(size - 1);

    if ((PCI_IO_SIZE - next_free) < size) {
        panic("PCI: IO space exhausted allocating 0x%08lx\n", size);
    }
    ret = next_free;
    next_free += size;
    return ret;
}

/*
 * Per-function metadata.
 *
 * Device handles are indices into an array of these structures.
 */
struct function_info
{
    struct resource_info    *resources;
    volatile void           *cfg_space;
    ULONG                   driver_status;
    WORD                    interrupt_index;
};

static struct function_info pci_functions[PCI_MAX_FUNCTIONS];

static struct function_info *alloc_function(void)
{
    static UWORD next_free;

    if (next_free < PCI_MAX_FUNCTIONS) {
        return &pci_functions[next_free++];
    }
    panic("PCI: function slots exhausted\n");
}

static BOOL handle_valid(LONG handle)
{
    return pci_functions[handle].cfg_space != 0;
}

static struct function_info *function_for_handle(LONG handle)
{
    if (handle >= PCI_MAX_FUNCTIONS) {
        return NULL;
    }
    if (pci_functions[handle].cfg_space) {
        return &pci_functions[handle];
    }
    return NULL;
}

static volatile void *config_ptr_for_handle(LONG handle)
{
    if (handle >= PCI_MAX_FUNCTIONS) {
        return NULL;
    }
    return pci_functions[handle].cfg_space;
}

/*
 * Function interrupt dispatch information.
 * Compact representation to simplify the interrupt handler.
 *
 * Handler indices do *not* necessarily correspond to function
 * indices; they are allocated on a first-free basis.
 *
 * A handler set to 1 indicates no more handlers; a handler
 * set to 0 is a free slot.
 */
struct {
    void    *arg;
    PFVOID  handler;
} qemu_pci_interrupt_handlers[PCI_MAX_FUNCTIONS + 1];

#define PCI_HANDLER_FREE        (PFVOID)0
#define PCI_HANDLER_UNUSED      (PFVOID)1

/*
 *  3.1.  Find PCI device
 *
 *  This function returns a device handle for a device that matches the
 *  given device and vendor ID.
 *
 *  A driver can query multiple devices with the same device and vendor ID
 *  by starting with index 0 and calling this function until
 *  PCI_DEVICE_NOT_FOUND is returned.
 *
 *       ______________________________________________________________________
 *       find_pci_device:
 *       Input:
 *         D0.L      Device ID in bits 31..16 (0 - $FFFF),
 *                   Vendor ID in bits 15..0  (0 - $FFFE)
 *         D1.W      Index (0 - # of cards with these IDs)
 *       Output:
 *         D0.L      device handle or error code
 *       ______________________________________________________________________
 *
 *  As a special case, Vendor ID $FFFF can be used to query all cards
 *  found in the system, the specified device ID is ignored in this case.
 */
LONG qemu_pci_find_pci_device(ULONG id, UWORD index)
{
    UWORD target_vendor = id & 0xffff;
    UWORD target_device = id >> 16;
    LONG handle;

    for (handle = 0; handle_valid(handle); handle++) {
        UWORD cfg_word;

        /* wildcard scan? */
        if (target_vendor == PCIV_INVALID) {
            if (index-- == 0) {
                return handle;
            }
        }

        qemu_pci_read_config_word(handle, PCIR_VENDOR, &cfg_word);
        if (cfg_word == target_vendor) {
            qemu_pci_read_config_word(handle, PCIR_DEVICE, &cfg_word);
            if (cfg_word == target_device) {
                if (index-- == 0) {
                    return handle;
                }
            }
        }
    }
    return PCI_DEVICE_NOT_FOUND;
}

/*
 *  3.2.  Find PCI class code
 *
 *  This function returns a device handle for a device that matches the
 *  given class code.
 *
 *  A driver can query multiple devices with the same class code by
 *  starting with index 0 and calling this function until
 *  PCI_DEVICE_NOT_FOUND is returned.
 *
 *       ______________________________________________________________________
 *       find_pci_classcode:
 *       Input:
 *         D0.L      class code in bits 23..0:
 *                     Base class in bit 23..16 (0 - $FF),
 *                     Sub  class in bit 15..8  (0 - $FF),
 *                     Prog. If.  in bit  7..0  (0)
 *                   mask in bits 26..24:
 *                     bit 26: if 0 = compare base class, else ignore it
 *                     bit 25: if 0 = compare sub class, else ignore it
 *                     bit 24: if 0 = compare prog. if, else ignore it
 *         D1.W      Index (0 - # of cards with these IDs)
 *       Output:
 *         D0.L      device handle or error code
 *       ______________________________________________________________________
 */
LONG qemu_pci_find_pci_classcode(ULONG class, UWORD index)
{
    LONG handle;
    ULONG mask = (((class & (1UL << 26)) ? 0xff000000UL : 0) |
                  ((class & (1UL << 25)) ? 0x00ff0000UL : 0) |
                  ((class & (1UL << 24)) ? 0x0000ff00UL : 0));
    class = (class << 8) & mask;

    for (handle = 0; handle_valid(handle); handle++) {
        ULONG classcode;

        qemu_pci_read_config_longword(handle, PCIR_REVID, &classcode);
        if ((classcode & mask) == class) {
            if (index-- == 0) {
                return handle;
            }
        }
    }
    return PCI_DEVICE_NOT_FOUND;
}

/*
 *  3.3.  Read Configuration {Byte|Word|Longword}
 *
 *  These three functions read data from the PCI configuration space of a
 *  given card.
 *
 *  The data is in little endian format, as described in the PCI
 *  specification.
 *
 *       ______________________________________________________________________
 *       read_config_byte:
 *       read_config_word:
 *       read_config_longword:
 *       Input:
 *         D0.L      device handle
 *         A0.L      pointer to space for read data
 *         D1.B      Register number (0,1,2,... for Byte access)
 *         D1.B      Register number (0,2,4,... for Word access)
 *         D1.B      Register number (0,4,8,... for Longword access)
 *       Output:
 *         D0.L      error code
 *                   read data at buffer pointed to by A0
 *       ______________________________________________________________________
 */
LONG qemu_pci_read_config_byte(LONG handle, UWORD reg, UBYTE *address)
{
    volatile UBYTE *cfg = config_ptr_for_handle(handle);

    if (!cfg) {
        return PCI_BAD_HANDLE;
    }
    if (reg >= PCI_CFG_FUNCTION_SIZE) {
        return PCI_BAD_REGISTER_NUMBER;
    }
    if (!address) {
        return PCI_BUFFER_TOO_SMALL;
    }
    *address = *(cfg + reg);
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_read_config_word(LONG handle, UWORD reg, UWORD *address)
{
    volatile UWORD *cfg = config_ptr_for_handle(handle);

    if (!cfg) {
        return PCI_BAD_HANDLE;
    }
    if ((reg >= (PCI_CFG_FUNCTION_SIZE - 1)) ||
        (reg & 1)) {
        return PCI_BAD_REGISTER_NUMBER;
    }
    if (!address) {
        return PCI_BUFFER_TOO_SMALL;
    }
    *address = swap16(*(cfg + (reg >> 1)));
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_read_config_longword(LONG handle, UWORD reg, ULONG *address)
{
    volatile ULONG *cfg = config_ptr_for_handle(handle);

    if (!cfg) {
        return PCI_BAD_HANDLE;
    }
    if ((reg >= (PCI_CFG_FUNCTION_SIZE - 3)) ||
        (reg & 3)) {
        return PCI_BAD_REGISTER_NUMBER;
    }
    if (!address) {
        return PCI_BUFFER_TOO_SMALL;
    }
    *address = swap32(*(cfg + (reg >> 2)));
    KDEBUG(("pci_read_config_longword 0x%x = 0x%08lx\n", reg, *address));
    return PCI_SUCCESSFUL;
}

/*
 *  3.4.  Read Configuration {Byte|Word|Longword} FAST
 *
 *  These three functions read data from the PCI configuration space of a
 *  given card. They do only minimal error checking and are meant to be
 *  used only when access to configuration space is needed in interrupt
 *  handlers.
 *
 *  The data is in little endian format, as described in the PCI
 *  specification.
 *
 *       ______________________________________________________________________
 *       fast_read_config_byte:
 *       fast_read_config_word:
 *       fast_read_config_longword:
 *       Input:
 *         D0.L      device handle
 *         D1.B      Register number (0,1,2,... for Byte access)
 *         D1.B      Register number (0,2,4,... for Word access)
 *         D1.B      Register number (0,4,8,... for Longword access)
 *       Output:
 *         D0        read data (8, 16 or 32 bits)
 *         SR:       carry flag may be set if an error occurred
 *                   (implementation dependent)
 *       ______________________________________________________________________
 */
UBYTE qemu_pci_fast_read_config_byte(LONG handle, UWORD reg)
{
    volatile UBYTE *cfg = config_ptr_for_handle(handle);

    return *(cfg + reg);
}

UWORD qemu_pci_fast_read_config_word(LONG handle, UWORD reg)
{
    volatile UWORD *cfg = config_ptr_for_handle(handle);

    return swap16(*(cfg + (reg >> 1)));
}

ULONG qemu_pci_fast_read_config_longword(LONG handle, UWORD reg)
{
    volatile ULONG *cfg = config_ptr_for_handle(handle);

    return swap32(*(cfg + (reg >> 2)));
}

/*
 *  3.5.  Write Configuration {Byte|Word|Longword}
 *
 *  These three functions write data to the PCI configuration space of a
 *  given card.
 *
 *  The data is in little endian format, as described in the PCI
 *  specification.
 *
 *       ______________________________________________________________________
 *       write_config_byte:
 *       write_config_word:
 *       write_config_longword:
 *       Input:
 *         D0.L      device handle
 *         D1.B      Register number (0,1,2,... for Byte access)
 *         D1.B      Register number (0,2,4,... for Word access)
 *         D1.B      Register number (0,4,8,... for Longword access)
 *         D2        data to write (8/16/32 bits)
 *       Output:
 *         D0.L      error code
 *       ______________________________________________________________________
 */
LONG qemu_pci_write_config_byte(LONG handle, UWORD reg, UWORD val)
{
    volatile UBYTE *cfg = config_ptr_for_handle(handle);

    if (!cfg) {
        return PCI_BAD_HANDLE;
    }
    if (reg >= PCI_CFG_FUNCTION_SIZE) {
        return PCI_BAD_REGISTER_NUMBER;
    }
    *(cfg + reg) = val;
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_write_config_word(LONG handle, UWORD reg, UWORD val)
{
    volatile UWORD *cfg = config_ptr_for_handle(handle);

    if (!cfg) {
        return PCI_BAD_HANDLE;
    }
    if ((reg >= (PCI_CFG_FUNCTION_SIZE - 1)) ||
        (reg & 1)) {
        return PCI_BAD_REGISTER_NUMBER;
    }
    *(cfg + (reg >> 1)) = swap16(val);
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_write_config_longword(LONG handle, UWORD reg, ULONG val)
{
    volatile ULONG *cfg = config_ptr_for_handle(handle);

    if (!cfg) {
        return PCI_BAD_HANDLE;
    }
    if ((reg >= (PCI_CFG_FUNCTION_SIZE - 3)) ||
        (reg & 3)) {
        return PCI_BAD_REGISTER_NUMBER;
    }
    KDEBUG(("PCI: cfg write 0x%x = 0x%08lx\n", reg, val));
    *(cfg + (reg >> 2)) = swap32(val);
    return PCI_SUCCESSFUL;
}

/*
 *  3.6.  Hook Interrupt Vector
 *
 *  This function hooks the driver into the interrupt chain to which a
 *  specific interrupt on the given card is routed. The interrupt is
 *  enabled on the system level, however, it is the drivers responsibility
 *  to enable the interrupt on the card as needed.
 *
 *  The driver should first hook into the interrupt chain, and then enable
 *  the interrupt on the card, in order not to cause spurious interrupts.
 *
 *       ______________________________________________________________________
 *       hook_interrupt:
 *       Input:
 *         D0.L      device handle
 *         A0.L      pointer to interrupt handler
 *         A1.L      parameter for interrupt handler.
 *       Output:
 *         D0.L      error code
 *       ______________________________________________________________________
 *
 *  The parameter (from A1) is passed to the interrupt handler unmodified
 *  - its meaning is totally driver dependent.
 */
LONG qemu_pci_hook_interrupt(LONG handle, PFVOID routine, void *parameter)
{
    struct function_info *fn = function_for_handle(handle);
    WORD index;

    if (!fn) {
        return PCI_BAD_HANDLE;
    }

    for (index = 0; index < PCI_MAX_INTERRUPTS; index++) {
        if (qemu_pci_interrupt_handlers[index].handler <= PCI_HANDLER_UNUSED) {
            break;
        }
    }
    if (index >= PCI_MAX_INTERRUPTS) {
        /* should be impossible */
        panic("PCI: interrupt slots exhausted\n");
    }
    fn->interrupt_index = index;
    qemu_pci_interrupt_handlers[index].arg = parameter;
    qemu_pci_interrupt_handlers[index].handler = routine;

    return PCI_SUCCESSFUL;
}

/*
 *  3.7.  Unhook Interrupt Vector
 *
 *  This function removes the driver from the interrupt chain to which a
 *  specific interrupt on the given card is routed. The driver must turn
 *  off interrupt generation on the card before calling this function.
 *
 *  ______________________________________________________________________
 *  unhook_interrupt:
 *  Input:
 *    D0.L      device handle
 *  Output:
 *    D0.L      error code
 *  ______________________________________________________________________
 */
LONG qemu_pci_unhook_interrupt(LONG handle)
{
    struct function_info *fn = function_for_handle(handle);

    if (!fn) {
        return PCI_BAD_HANDLE;
    }
    if (fn->interrupt_index != -1) {
        qemu_pci_interrupt_handlers[fn->interrupt_index].handler = PCI_HANDLER_FREE;
        fn->interrupt_index = -1;      /* not hooked */
    }
    return PCI_SUCCESSFUL;
}

/*
 *  3.8.  Generate Special Cycle
 *
 *  This function generates a special cycle on the PCI bus.
 *
 *       ______________________________________________________________________
 *       special_cycle:
 *       Input:
 *         D0.B      bus number
 *         D1.L      special cycle data
 *       Output:
 *         D0.L      error code
 *       ______________________________________________________________________
 */
LONG qemu_pci_special_cycle(UWORD bus, ULONG data)
{
    return PCI_FUNC_NOT_SUPPORTED;
}

/*
 *  3.9.  Get Interrupt Routing Options
 *
 *  to be defined. Not for use by device drivers.
 */
LONG qemu_pci_get_routing(LONG handle)
{
    return PCI_FUNC_NOT_SUPPORTED;
}

/*
 *  3.10.  Set Hardware Interrupt
 *
 *  to be defined. Not for use by device drivers.
 */
LONG qemu_pci_set_interrupt(LONG handle)
{
    return PCI_FUNC_NOT_SUPPORTED;
}

/*
 *  3.11.  Get Resource Data
 *
 *  This function returns data about the resources which a device uses.
 *  The returned structure may *not* be modified by the driver.
 *
 *  The driver can then use these information and directly access the
 *  device if he knows the type of byte ordering, or it can use the system
 *  routines (read|write_mem|io...) which handle this transparently.
 *
 *  The function returns a pointer to the first resource descriptor for
 *  this device. The driver can get the next descriptor by adding the
 *  length field to the address of the current descriptor.
 *
 *  The descriptors are in the same order as the base address registers in
 *  the PCI configuration space of the device.
 *
 *  If bit 15 of the flags is set, there is no next descriptor.
 *
 *  A device can have multiple resources of the same type.
 *
 *  ______________________________________________________________________
 *  get_resource:
 *  Input:
 *    D0.L      device handle
 *  Output:
 *    D0.L      pointer to array of resource descriptors or error code
 *  ______________________________________________________________________
 *
 */
LONG qemu_pci_get_resource(LONG handle)
{
    struct function_info *fn = function_for_handle(handle);

    KDEBUG(("pci_get_resource(%ld) @ %p\n", handle, fn));

    if (!fn) {
        return PCI_BAD_HANDLE;
    }
    if (!fn->resources) {
        return PCI_GENERAL_ERROR;
    }

    return (LONG)fn->resources;
}

/*
 *  4.1.  Get|Set card used flag
 *
 *       ______________________________________________________________________
 *       get_card_used:
 *       Input:
 *         D0.L      device handle
 *         A0.L      pointer to longword where call-back address is stored
 *       Output:
 *         D0.L      error code or status
 *       ______________________________________________________________________
 *
 *  The returned status is either 0 (the card is free), 1 (the card is in
 *  use), 2 (the card is in use, but the driver can be uninstalled) or 3
 *  (the card is in use, but can be taken over without further actions).
 *
 *  Case 3 means that the driver which brought the card to this state may
 *  not hook into interrupt chains or other system resources which need to
 *  be unhooked when another driver takes over the card.
 *
 *  If the return code is 2, the call-back function entry of the driver is
 *  returned in the memory pointed to by A0.
 *
 *       ______________________________________________________________________
 *       set_card_used:
 *       Input:
 *         D0.L      device handle
 *         A0.L      address of call-back entry (not pointer to address!)
 *                   or 0L, 1L or 3L
 *       Output:
 *         D0.L      error code
 *       ______________________________________________________________________
 *
 *  If the call-back entry is 0L, 1L or 3L, the card status is set to that
 *  value. Only a driver which 'owns' the card may use this.
 *
 *  If any other value is passed, it is assumed to be a pointer to the
 *  drivers call back entry point, and the card status is set to 2 (in
 *  use, can be uninstalled).
 */
LONG qemu_pci_get_card_used(LONG handle, ULONG *address)
{
    struct function_info *fn = function_for_handle(handle);

    if (!fn) {
        return PCI_BAD_HANDLE;
    }

    switch (fn->driver_status) {
    case 0:
    case 1:
    case 3:
        return fn->driver_status;
    default:
        *address = fn->driver_status;
        return 2;
    }
}

LONG qemu_pci_set_card_used(LONG handle, ULONG *callback)
{
    struct function_info *fn = function_for_handle(handle);

    if (!fn) {
        return PCI_BAD_HANDLE;
    }

    switch (fn->driver_status) {
    case 0:
    case 3:
        /* claim / take over device */
        fn->driver_status = (ULONG)callback;
        return PCI_SUCCESSFUL;
    default:
        /* device in-use / cannot be taken over */
        return PCI_GENERAL_ERROR;
    }
}

/*
 *  3.12.  Memory Read / Memory Write
 *
 *  These functions read or write 8, 16 or 32-bit values from a memory
 *  region and take care of the byte ordering - ie. the data and address
 *  are converted as if Motorola byte ordering was in use.
 *
 *  A driver can use these functions for access to registers and small
 *  buffers. For larger amounts of data, the driver can choose to use his
 *  own copy routines, provided it knows the byte order in use.
 *
 *       ______________________________________________________________________
 *       read_mem_byte:
 *       read_mem_word:
 *       read_mem_longword:
 *       Input:
 *         D0.L      device handle
 *         D1.L      address to access (in PCI memory address space)
 *         A0.L      pointer to data in memory
 *       Output:
 *         D0.L      error code
 *         read data at buffer pointed to by A0
 *       ______________________________________________________________________
 *
 *       ______________________________________________________________________
 *       write_mem_byte:
 *       write_mem_word:
 *       write_mem_longword:
 *       Input:
 *         D0.L      device handle
 *         D1.L      address to access (in PCI memory address space)
 *         D2        data to write (8/16/32 bits)
 *       Output:
 *         D0.L      error code
 *       ______________________________________________________________________
 */
LONG qemu_pci_read_mem_byte(LONG handle, ULONG offset, UBYTE *address)
{
    if ((offset < PCI_MMIO_BASE) || (offset >= PCI_MMIO_END)) {
        return PCI_GENERAL_ERROR;
    }
    *address = *(UBYTE *)(offset);
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_read_mem_word(LONG handle, ULONG offset, UWORD *address)
{
    if ((offset < PCI_MMIO_BASE) || (offset >= (PCI_MMIO_END - 1))) {
        return PCI_GENERAL_ERROR;
    }
    *address = swap16(*(UWORD *)(offset));
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_read_mem_longword(LONG handle, ULONG offset, ULONG *address)
{
    if ((offset < PCI_MMIO_BASE) || (offset >= (PCI_MMIO_END - 3))) {
        return PCI_GENERAL_ERROR;
    }
    *address = swap32(*(ULONG *)(offset));
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_write_mem_byte(LONG handle, ULONG offset, UWORD val)
{
    if ((offset < PCI_MMIO_BASE) || (offset >= PCI_MMIO_END)) {
        return PCI_GENERAL_ERROR;
    }
    *(UBYTE *)(offset) = val;
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_write_mem_word(LONG handle, ULONG offset, UWORD val)
{
    if ((offset < PCI_MMIO_BASE) || (offset >= PCI_MMIO_END)) {
        return PCI_GENERAL_ERROR;
    }
    *(UWORD *)(offset) = swap16(val);
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_write_mem_longword(LONG handle, ULONG offset, ULONG val)
{
    if ((offset < PCI_MMIO_BASE) || (offset >= PCI_MMIO_END)) {
        return PCI_GENERAL_ERROR;
    }
    *(ULONG *)(offset) = swap32(val);
    return PCI_SUCCESSFUL;
}

/*
 *  3.13.  IO Read / IO Write
 *
 *  These functions read or write 8, 16 or 32-bit values from a IO region
 *  and take care of the byte ordering - ie. the data and address are
 *  converted as if Motorola byte ordering was in use.
 *
 *       ______________________________________________________________________
 *       read_io_byte:
 *       read_io_word:
 *       read_io_longword:
 *       Input:
 *         D0.L      device handle
 *         D1.L      address to access (in PCI IO address space)
 *         A0.L      pointer to data in memory
 *       Output:
 *         D0.L      error code
 *       ______________________________________________________________________
 *
 *       ______________________________________________________________________
 *       write_io_byte:
 *       write_io_word:
 *       write_io_longword:
 *       Input:
 *         D0.L      device handle
 *         D1.L      address to access (in PCI IO address space)
 *         D2        data to write (8/16/32 bits)
 *       Output:
 *         D0.L      error code
 *       ______________________________________________________________________
 */
LONG qemu_pci_read_io_byte(LONG handle, ULONG offset, UBYTE *address)
{
    if (offset >= PCI_IO_SIZE) {
        return PCI_GENERAL_ERROR;
    }
    *address = *(UBYTE *)(PCI_IO_BASE + offset);
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_read_io_word(LONG handle, ULONG offset, UWORD *address)
{
    if (offset >= (PCI_IO_SIZE - 1)) {
        return PCI_GENERAL_ERROR;
    }
    *address = swap16(*(UWORD *)(PCI_IO_BASE + offset));
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_read_io_longword(LONG handle, ULONG offset, ULONG *address)
{
    if (offset >= (PCI_IO_SIZE - 3)) {
        return PCI_GENERAL_ERROR;
    }
    *address = swap32(*(ULONG *)(PCI_IO_BASE + offset));
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_write_io_byte(LONG handle, ULONG offset, UWORD val)
{
    if (offset >= PCI_IO_SIZE) {
        return PCI_GENERAL_ERROR;
    }
    *(UBYTE *)(PCI_IO_BASE + offset) = val;
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_write_io_word(LONG handle, ULONG offset, UWORD val)
{
    if (offset >= (PCI_IO_SIZE - 1)) {
        return PCI_GENERAL_ERROR;
    }
    *(UWORD *)(PCI_IO_BASE + offset) = swap16(val);
    return PCI_SUCCESSFUL;
}

LONG qemu_pci_write_io_longword(LONG handle, ULONG offset, ULONG val)
{
    if (offset >= (PCI_IO_SIZE - 3)) {
        return PCI_GENERAL_ERROR;
    }
    *(ULONG *)(PCI_IO_BASE + offset) = swap32(val);
    return PCI_SUCCESSFUL;
}

/*
 *  3.14.  FAST Memory/IO Read
 *
 *  These functions are alternatives for the normal memory/io read
 *  functions. They return the read value in D0 and return no error code,
 *  which makes them easier to use in C.
 *
 *       ______________________________________________________________________
 *       fast_read_mem_byte:
 *       fast_read_mem_word:
 *       fast_read_mem_longword:
 *       fast_read_io_byte:
 *       fast_read_io_word:
 *       fast_read_io_longword:
 *       Input:
 *         D0.L      device handle
 *         D1.L      address to access (in PCI IO address space)
 *       Output:
 *         D0.L      read data
 *       ______________________________________________________________________
 */
UBYTE qemu_pci_fast_read_mem_byte(LONG handle, ULONG offset)
{
    return *(UBYTE *)offset;
}

UWORD qemu_pci_fast_read_mem_word(LONG handle, ULONG offset)
{
    return swap16(*(UWORD *)offset);
}

ULONG qemu_pci_fast_read_mem_longword(LONG handle, ULONG offset)
{
    return swap32(*(ULONG *)offset);
}

UBYTE qemu_pci_fast_read_io_byte(LONG handle, ULONG offset)
{
    return *(UBYTE *)(PCI_IO_BASE + offset);
}

UWORD qemu_pci_fast_read_io_word(LONG handle, ULONG offset)
{
    return swap16(*(UWORD *)(PCI_IO_BASE + offset));
}

ULONG qemu_pci_fast_read_io_longword(LONG handle, ULONG offset)
{
    return swap32(*(ULONG *)(PCI_IO_BASE + offset));
}

/*
 *  5.  Machine  ID
 *
 *  This function can be used to get a unique machine ID for the computer
 *  the driver is running on:
 *
 *  ______________________________________________________________________
 *  get_machine_id:
 *  Input:
 *    none
 *  Output:
 *    D0.L      Machine ID, or 0 (no ID available), or error code
 *  ______________________________________________________________________
 *
 *  Positive, non-zero values are machine IDs. They contain a manufacturer
 *  code in bit 24..31 and a unique serial number which is set by the
 *  manufacturer during production in bit 0..23.
 */
LONG qemu_pci_get_machine_id(void)
{
    /* we don't have a machine ID */
    return 0;
}

/*
 *  6.1.  Get Pagesize
 *
 *       ______________________________________________________________________
 *       get_pagesize:
 *       Input:
 *         none
 *       Output:
 *         D0.L      active pagesize or 0 if paging is not active
 *       ______________________________________________________________________
 */
LONG qemu_pci_get_pagesize(void)
{
    /* paging not enabled */
    return 0;
}

/*
 *  6.2.  Convert virtual to PCI bus address
 *
 *  ______________________________________________________________________
 *  virt_to_bus:
 *  Input:
 *    D0.L      device handle
 *    D1.L      address in virtual CPU space
 *    A0.L      pointer to mem-struct for results
 *  Output:
 *    D0.L      error code
 *
 *  If D0.L indicates no error, mem-struct is filled as follows:
 *  mem-struct:
 *    DS.L 1   ;PCI bus address
 *    DS.L 1   ;length of contiguous mapped area, 0 if no DMA is possible
 *             ;at this address
 *  ______________________________________________________________________
 */
LONG qemu_pci_virt_to_bus(LONG handle, ULONG address, PCI_CONV_ADR *conv)
{
    /* mapping is 1:1, so just adjust length */
    conv->len = PCI_MMIO_END - conv->adr;

    return PCI_SUCCESSFUL;
}

/*
 *  6.3.  Convert PCI bus to virtual address
 *
 *  This function is the reverse of virt_to_bus. It might be slow, so the
 *  driver should avoid using it if it can determine the address by other
 *  means.
 *
 *       ______________________________________________________________________
 *       bus_to_virt:
 *       Input:
 *         D0.L      device handle
 *         D1.L      PCI bus address
 *         A0.L      pointer to mem-struct for results
 *       Output:
 *         D0.L      error code
 *
 *       If D0.L indicates no error, mem-struct is filled as follows:
 *       mem-struct:
 *         DS.L 1   ;CPU (virtual) address
 *         DS.L 1   ;length of contiguous mapped area
 *       ______________________________________________________________________
 */
LONG qemu_pci_bus_to_virt(LONG handle, ULONG address, PCI_CONV_ADR *conv)
{
    /* mapping is 1:1, so just adjust length */
    conv->len = PCI_MMIO_END - conv->adr;

    return PCI_SUCCESSFUL;
}

/*
 *  6.4.  Convert virtual to physical CPU address
 *
 *  ______________________________________________________________________
 *  virt_to_phys:
 *  Input:
 *    D0.L      address in virtual CPU space
 *    A0.L      pointer to mem-struct for results
 *  Output:
 *    D0.L      error code
 *
 *  If D0.L indicates no error, mem-struct is filled as follows:
 *  mem-struct:
 *    DS.L 1   ;physical CPU address
 *    DS.L 1   ;length of contiguous mapped area, 0 if not mapped
 *  ______________________________________________________________________
 */
LONG qemu_pci_virt_to_phys(ULONG address, PCI_CONV_ADR *conv)
{
    /* mapping is 1:1, so just adjust length */
    conv->len = PCI_MMIO_END - conv->adr;

    return PCI_SUCCESSFUL;
}

/*
 *  6.5.  Convert physical CPU to virtual address
 *
 *  This function is the reverse of virt_to_bus. It might be slow, so the
 *  driver should avoid using it if it can determine the address by other
 *  means.
 *
 *       ______________________________________________________________________
 *       phys_to_virt:
 *       Input:
 *         D0.L      physical CPU address
 *         A0.L      pointer to mem-struct for results
 *       Output:
 *         D0.L      error code
 *
 *       If D0.L indicates no error, mem-struct is filled as follows:
 *       mem-struct:
 *         DS.L 1   ;CPU (virtual) address
 *         DS.L 1   ;length of contiguous mapped area
 *       ______________________________________________________________________
 */
LONG qemu_pci_phys_to_virt(ULONG address, PCI_CONV_ADR *conv)
{
    /* mapping is 1:1, so just adjust length */
    conv->len = PCI_MMIO_END - conv->adr;

    return PCI_SUCCESSFUL;
}

static void print_function(LONG handle)
{
    struct function_info *fn = function_for_handle(handle);

    KDEBUG(("pci%ld: @ %p vendor 0x%04x device 0x%04x class 0x%02x subclass 0x%02x\n",
            handle, fn,
            qemu_pci_fast_read_config_word(handle, PCIR_VENDOR),
            qemu_pci_fast_read_config_word(handle, PCIR_DEVICE),
            qemu_pci_fast_read_config_byte(handle, PCIR_CLASS),
            qemu_pci_fast_read_config_byte(handle, PCIR_SUBCLASS)));

    if (fn->resources) {
        struct resource_info *rsc = fn->resources;
        KDEBUG(("  resources @ %p\n", rsc));
        do {
            KDEBUG(("  %s @ 0x%08lx/0x%08lx\n", 
                    ((rsc->flags & RSC_IO) ? " IO" : "MEM"),
                    rsc->start,
                    rsc->length));
        } while (!((rsc++)->flags & RSC_LAST));
    }
}

static void configure_function(LONG handle)
{
    ULONG index;
    ULONG mask;
    UWORD reg;
    struct function_info *fn = function_for_handle(handle);
    struct resource_info *last_rsc = NULL;

    /*
     * Allocate MMIO resources.
     */
    for (index = PCIR_BARS; index < PCIR_CIS; index += 4) {

        /* simple 32-bit BAR configuration */
        qemu_pci_write_config_longword(handle, index, 0xffffffffUL);
        qemu_pci_read_config_longword(handle, index, &mask);

        if (mask) {
            ULONG size = 0;
            ULONG addr = 0;
            BOOL io_alloc = FALSE;

            if ((mask & 3) == 1) {
                /* I/O - QEMU doesn't seem to do the high-word shenanigans */
                size = ~(mask & 0xfffffffcUL) + 1;
                addr = alloc_io(size);
                io_alloc = TRUE;
                qemu_pci_write_config_longword(handle, index, addr);
            } else if ((mask & 0x7) == 0) {
                /* 32-bit memory */
                size = ~(mask & 0xfffffff0UL) + 1;
                addr = alloc_mmio(size);
                qemu_pci_write_config_longword(handle, index, addr);
            } else {
                /* don't try to deal with 64-bit BARs, ideally we won't see them */
                panic("PCI: BAR 0x%08lx unsupported\n", mask);
            }

            /* if we allocated something */
            if (size > 0) {
                /* get a new resource */
                last_rsc = alloc_resource();
                if (!fn->resources) {
                    fn->resources = last_rsc;
                }

                /* record our allocation */
                last_rsc->flags = ((io_alloc ? RSC_IO : 0) |
                                   FLG_8BIT | FLG_16BIT | FLG_32BIT |
                                   2);   /* lane-swapped */
                if (io_alloc) {
                    last_rsc->start = PCI_IO_BASE + addr;
                    last_rsc->offset = PCI_IO_BASE;
                } else {
                    last_rsc->start = addr;
                    last_rsc->offset = 0;
                }
                last_rsc->length = size;
                last_rsc->dmaoffset = 0;
            }
        }
    }

    /*
     * Check for a VGA device and if so, map/enable the option ROM.
     */
    ULONG classcode = 0;
    qemu_pci_read_config_longword(handle, PCIR_REVID, &classcode);
    classcode >>= 16;
    if ((classcode == ((PCIC_DISPLAY << 8) | (PCIS_DISPLAY_VGA))) ||
        (classcode == ((PCIC_OLD << 8) | PCIS_OLD_VGA))) {

        qemu_pci_write_config_longword(handle, PCIR_BIOS, 0xfffff800UL);
        qemu_pci_read_config_longword(handle, PCIR_BIOS, &mask);

        if (mask && (mask != 0xfffff800UL)) {
            ULONG size = ~(mask & 0xfffff800UL) + 1;
            ULONG addr = alloc_mmio(size);
            qemu_pci_write_config_longword(handle, PCIR_BIOS, addr | 1);

            KDEBUG(("exrom 0x%08lx\n", mask));

            /* get a new resource */
            last_rsc = alloc_resource();
            if (!fn->resources) {
                fn->resources = last_rsc;
            }

            /* record our allocation */
            last_rsc->flags = FLG_8BIT | FLG_16BIT | FLG_32BIT | 2;
            last_rsc->start = addr;
            last_rsc->length = size;
            last_rsc->offset = 0;
            last_rsc->dmaoffset = 0;
        }

        /* set last-resource flag */
        if (last_rsc) {
            last_rsc->flags |= RSC_LAST;
        }
    }

    /* enable I/O decode, mastering, interrupts */
    qemu_pci_read_config_word(handle, PCIR_COMMAND, &reg);
    reg |= PCIM_CMD_PORTEN | PCIM_CMD_MEMEN | PCIM_CMD_BUSMASTEREN;
    reg &= ~PCIM_CMD_INTxDIS;
    qemu_pci_write_config_word(handle, PCIR_COMMAND, reg);
}

static void scan_and_configure(void)
{
    UWORD bus;
    UWORD dev;
    UWORD func;
    LONG handle;

    /* scan supportable bus numbers */
    for (bus = 0; bus < PCI_MAX_BUS; bus++) {
        /* scan devices on the bus */
        for (dev = 0; dev < PCI_CFG_NUM_DEVICES; dev++) {
            /* scan functions within the device */
            for (func = 0; func < PCI_CFG_NUM_FUNCTIONS; func++) {
                /* form a pointer to configuration space */
                UBYTE *cfg = (UBYTE *)PCI_ECAM_BASE + (dev * PCI_CFG_NUM_FUNCTIONS + func) * PCI_CFG_FUNCTION_SIZE;
                UBYTE hdrtype = *(UBYTE *)(cfg + PCIR_HDRTYPE);
                struct function_info *fn;

                /* check vendor - all 1s if no device/function present */
                if (*(UWORD *)cfg == PCIV_INVALID) {
                    /* no function present at this BDF */
                    continue;
                }

                /* check header type; only support type 0 */
                if (hdrtype & PCIM_HDRTYPE) {
                    continue;
                }

                fn = alloc_function();
                fn->cfg_space = cfg;

                /* check header type MF bit; if not set, do not scan other functions */
                if (!(hdrtype & PCIM_MFDEV)) {
                    break;
                }
            }
        }
    }
    for (handle = 0; handle_valid(handle); handle++) {
        configure_function(handle);
        print_function(handle);
    }
}

void qemu_pci_init(void)
{
    UWORD   i;

    /* initialize interrupt dispatch table */
    for (i = 0; i <= PCI_MAX_INTERRUPTS; i++) {
        qemu_pci_interrupt_handlers[i].handler = PCI_HANDLER_UNUSED;
    }

    /* scan busses and allocate resources */
    scan_and_configure();

    /* hook the PCI interrupt vector */
    VEC_LEVEL5 = qemu_pci_interrupt;
}

void qemu_pci_add_cookies(void)
{
    /* install the _PCI cookie */
    cookie_add(0x5f504349, (ULONG)qemu_pci_dispatch_table);
}

void qemu_pci_spurious(void)
{
    panic("PCI: spurious interrupt\n");
}
