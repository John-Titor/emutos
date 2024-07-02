/*
 * qemu-system-m68k -machine type=atarist
 */

#ifndef QEMU_H
#define QEMU_H

/* magic numbers shared with the qemu side */

#define PCI_MMIO_BASE           0xd0000000UL
#define PCI_MMIO_SIZE           0x20000000UL
#define PCI_MMIO_END            (PCI_MMIO_BASE + PCI_MMIO_SIZE)
#define PCI_ECAM_BASE           0xf0000000UL
#define PCI_ECAM_SIZE           0x00100000UL
#define PCI_IO_BASE             0xf0100000UL
#define PCI_IO_SIZE             0x00010000UL

#define FRAMEBUFFER_REG_BASE    0xffffc000UL
#define FRAMEBUFFER_PAL_BASE    0xffffc400UL

#define VIRTIO_BASE             0xf0400000UL
#define VIRTIO_NDEV                 8
#define VIRTIO_DEVSIZE              0x200

#define GF_TTY_BASE             0xffffb400UL
#define VIRT_CTRL_BASE          0xffffb500UL

/* missing utility functions */

__attribute__((unused))
static inline UWORD swap16(UWORD val)
{
    __asm__ volatile("rolw #8,%0" : "=d"(val) : "0"(val) : "cc");
    return val;
}

__attribute__((unused))
static inline ULONG swap32(ULONG val)
{
	/*
    __asm__ volatile("rolw #8,%0; swap %0; rolw #8,%0" : "=d"(val) : "0"(val) : "cc");
    return val;
    */
    return ((val>>24) & 0x000000ffUL) |
           ((val<<8)  & 0x00ff0000UL) |
           ((val>>8)  & 0x0000ff00UL) |
           ((val<<24) & 0xff000000UL);
}

/* returns 1 + the least significant bit set in x, or 0 if no bits are set */
__attribute__((unused))
static ULONG __ffs (ULONG x)
{
  ULONG cnt;

  asm ("bfffo %1{#0:#0},%0" : "=d" (cnt) : "dm" (x & -x));

  return (32 - cnt) % 32;
}

/* video */
void qemu_screen_init_mode(void);
void qemu_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez);
WORD qemu_vsetmode(WORD mode);
WORD qemu_vmontype(void);
WORD qemu_vsetsync(WORD external);
LONG qemu_vgetsize(WORD mode);
WORD qemu_vsetrgb(WORD index,WORD count,const ULONG *rgb);
WORD qemu_vgetrgb(WORD index,WORD count,ULONG *rgb);
WORD qemu_vfixmode(WORD mode);

void qemu_vbl(void);
void qemu_vbl_shim(void);
void qemu_video_add_cookies(void);

/* PCI */

struct pci_resource_info
{
    UWORD   next;
    UWORD   flags;
    ULONG   start;
    ULONG   length;
    ULONG   offset;
    ULONG   dmaoffset;

    /* private extension data */
    UBYTE   bar;
    UBYTE   pad[3];
};

#define PCI_RSC_IO          0x4000
#define PCI_RSC_LAST        0x8000
#define PCI_RSC_FLG_8BIT    0x0100
#define PCI_RSC_FLG_16BIT   0x0200
#define PCI_RSC_FLG_32BIT   0x0400
#define PCI_RSC_FLG_ENDMASK 0x000f

typedef struct pci_conv_adr     /* structure of address conversion */
{
    ULONG adr;              /* calculated address (CPU<->PCI) */
    ULONG len;              /* length of memory range */
} PCI_CONV_ADR;

extern PFVOID qemu_pci_dispatch_table[];

/* PCI BIOS errors */
#define PCI_SUCCESSFUL          0x00000000UL
#define PCI_FUNC_NOT_SUPPORTED  0xfffffffeUL
#define PCI_BAD_VENDOR_ID       0xfffffffdUL
#define PCI_DEVICE_NOT_FOUND    0xfffffffcUL
#define PCI_BAD_REGISTER_NUMBER 0xfffffffbUL
#define PCI_SET_FAILED          0xfffffffaUL
#define PCI_BUFFER_TOO_SMALL    0xfffffff9UL
#define PCI_GENERAL_ERROR       0xfffffff8UL
#define PCI_BAD_HANDLE          0xfffffff7UL

#define PCI_RESULT_IS_ERROR(_x) (_x > 0xfffffff0UL)

LONG qemu_pci_find_pci_device(ULONG id, UWORD index);
LONG qemu_pci_find_pci_classcode(ULONG class, UWORD index);
LONG qemu_pci_read_config_byte(LONG handle, UWORD reg, UBYTE *address);
LONG qemu_pci_read_config_word(LONG handle, UWORD reg, UWORD *address);
LONG qemu_pci_read_config_longword(LONG handle, UWORD reg, ULONG *address);
UBYTE qemu_pci_fast_read_config_byte(LONG handle, UWORD reg);
UWORD qemu_pci_fast_read_config_word(LONG handle, UWORD reg);
ULONG qemu_pci_fast_read_config_longword(LONG handle, UWORD reg);
LONG qemu_pci_write_config_byte(LONG handle, UWORD reg, UWORD val);
LONG qemu_pci_write_config_word(LONG handle, UWORD reg, UWORD val);
LONG qemu_pci_write_config_longword(LONG handle, UWORD reg, ULONG val);
LONG qemu_pci_hook_interrupt(LONG handle, PFVOID routine, void *parameter);
LONG qemu_pci_unhook_interrupt(LONG handle);
LONG qemu_pci_special_cycle(UWORD bus, ULONG data);
LONG qemu_pci_get_routing(LONG handle);
LONG qemu_pci_set_interrupt(LONG handle);
LONG qemu_pci_get_resource(LONG handle);
LONG qemu_pci_get_card_used(LONG handle, ULONG *address);
LONG qemu_pci_set_card_used(LONG handle, ULONG *callback);
LONG qemu_pci_read_mem_byte(LONG handle, ULONG offset, UBYTE *address);
LONG qemu_pci_read_mem_word(LONG handle, ULONG offset, UWORD *address);
LONG qemu_pci_read_mem_longword(LONG handle, ULONG offset, ULONG *address);
UBYTE qemu_pci_fast_read_mem_byte(LONG handle, ULONG offset);
UWORD qemu_pci_fast_read_mem_word(LONG handle, ULONG offset);
ULONG qemu_pci_fast_read_mem_longword(LONG handle, ULONG offset);
LONG qemu_pci_write_mem_byte(LONG handle, ULONG offset, UWORD val);
LONG qemu_pci_write_mem_word(LONG handle, ULONG offset, UWORD val);
LONG qemu_pci_write_mem_longword(LONG handle, ULONG offset, ULONG val);
LONG qemu_pci_read_io_byte(LONG handle, ULONG offset, UBYTE *address);
LONG qemu_pci_read_io_word(LONG handle, ULONG offset, UWORD *address);
LONG qemu_pci_read_io_longword(LONG handle, ULONG offset, ULONG *address);
UBYTE qemu_pci_fast_read_io_byte(LONG handle, ULONG offset);
UWORD qemu_pci_fast_read_io_word(LONG handle, ULONG offset);
ULONG qemu_pci_fast_read_io_longword(LONG handle, ULONG offset);
LONG qemu_pci_write_io_byte(LONG handle, ULONG offset, UWORD val);
LONG qemu_pci_write_io_word(LONG handle, ULONG offset, UWORD val);
LONG qemu_pci_write_io_longword(LONG handle, ULONG offset, ULONG val);
LONG qemu_pci_get_machine_id(void);
LONG qemu_pci_get_pagesize(void);
LONG qemu_pci_virt_to_bus(LONG handle, ULONG address, PCI_CONV_ADR *conv);
LONG qemu_pci_bus_to_virt(LONG handle, ULONG address, PCI_CONV_ADR *conv);
LONG qemu_pci_virt_to_phys(ULONG address, PCI_CONV_ADR *conv);
LONG qemu_pci_phys_to_virt(ULONG address, PCI_CONV_ADR *conv);

LONG qemu_pci_get_next_cap(LONG handle, UBYTE capptr);
void qemu_pci_init(void);
void qemu_pci_interrupt(void);
void qemu_pci_spurious(void);
void qemu_pci_add_cookies(void);

/* VirtIO */

#define VIO_SUCCESS         0x00000000UL
#define VIO_NOT_FOUND       0xfffffffcUL

ULONG qemu_vio_find_device(ULONG device_id, ULONG index);
void qemu_vio_init(void);
void qemu_vio_add_cookies(void);

/* misc system */
void qemu_shutdown(void);
void qemu_mfp_int(void);
PFVOID qemu_mfp_revector(PFVOID *vbr);
PFLONG xbios_extlookup(UWORD fn);

/* VirtIO */

typedef struct {
    ULONG version;
    ULONG (* find_device)(ULONG device_id, ULONG index);
} virtio_dispatch_table_t;

#endif /* QEMU_H */
