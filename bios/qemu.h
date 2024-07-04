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
#define VIRTIO_DEVSIZE              0x200UL

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

/* misc system */
void qemu_shutdown(void);
void qemu_mfp_int(void);
PFVOID qemu_mfp_revector(PFVOID *vbr);
PFLONG xbios_extlookup(UWORD fn);

/************************************************************************
 * PCI
 */

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

/************************************************************************
 * VirtIO
 */

#define VIRTIO_SUCCESS          0x00000000UL
#define VIRTIO_ERROR            0xffffffffUL

#define VIRTIO_VENDOR_QEMU      0x554d4551UL
#define VIRTIO_DEVICE_INPUT     0x12UL

void qemu_virtio_init(void);
void qemu_virtio_add_cookies(void);
void qemu_virtio_register_interrupt(ULONG handle, void (* handler)(ULONG), ULONG arg);
void qemu_virtio_input_device_init(ULONG handle);

/*
 * MMIO registers
 */

#define VIRTIO_MMIO_MAGIC_VALUE         0x000
#define VIRTIO_MMIO_VERSION             0x004
#define VIRTIO_MMIO_VERSION_SUPPORTED       2
#define VIRTIO_MMIO_DEVICE_ID           0x008
#define VIRTIO_MMIO_VENDOR_ID           0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES     0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_F_VERSION_1                  (1<<0)  /* in Features[1] */
#define VIRTIO_MMIO_DRIVER_FEATURES     0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_QUEUE_SEL           0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX       0x034
#define VIRTIO_MMIO_QUEUE_NUM           0x038
#define VIRTIO_MMIO_QUEUE_READY         0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY        0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS    0x060
#define VIRTIO_MMIO_INT_VRING               (1<<0)
#define VIRTIO_MMIO_INT_CONFIG              (1<<1)
#define VIRTIO_MMIO_INTERRUPT_ACK       0x064
#define VIRTIO_MMIO_STATUS              0x070
#define VIRTIO_STAT_ACKNOWLEDGE             (1<<0)
#define VIRTIO_STAT_DRIVER                  (1<<1)
#define VIRTIO_STAT_DRIVER_OK               (1<<2)
#define VIRTIO_STAT_FEATURES_OK             (1<<3)
#define VIRTIO_STAT_NEEDS_RESET             (1<<6)
#define VIRTIO_STAT_FAILED                  (1<<7)
#define VIRTIO_MMIO_QUEUE_DESC_LOW      0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH     0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW     0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH    0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW      0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH     0x0a4
#define VIRTIO_MMIO_SHM_SEL             0x0ac
#define VIRTIO_MMIO_SHM_LEN_LOW         0x0b0
#define VIRTIO_MMIO_SHM_LEN_HIGH        0x0b4
#define VIRTIO_MMIO_SHM_BASE_LOW        0x0b8
#define VIRTIO_MMIO_SHM_BASE_HIGH       0x0bc
#define VIRTIO_MMIO_QUEUE_RESET         0x0c0
#define VIRTIO_MMIO_CONFIG_GENERATION   0x0fc
#define VIRTIO_MMIO_CONFIG              0x100

static volatile inline ULONG virtio_mmio_read32(ULONG handle, UWORD regidx)
{
    return swap32(*((volatile ULONG *)(VIRTIO_BASE + (handle * VIRTIO_DEVSIZE) + regidx)));
}

static volatile inline UWORD virtio_mmio_read16(ULONG handle, UWORD regidx)
{
    return swap16(*((volatile UWORD *)(VIRTIO_BASE + (handle * VIRTIO_DEVSIZE) + regidx)));
}

static volatile inline UBYTE virtio_mmio_read8(ULONG handle, UWORD regidx)
{
    return *((volatile UBYTE *)(VIRTIO_BASE + (handle * VIRTIO_DEVSIZE) + regidx));
}

static inline void virtio_mmio_write32(ULONG handle, UWORD regidx, ULONG val)
{
    *((volatile ULONG *)(VIRTIO_BASE + (handle * VIRTIO_DEVSIZE) + regidx)) = swap32(val);
}

static inline void virtio_mmio_write8(ULONG handle, UWORD regidx, UBYTE val)
{
    *((volatile UBYTE *)(VIRTIO_BASE + (handle * VIRTIO_DEVSIZE) + regidx)) = val;
}

/*
 * MMIO queue structures
 */

typedef struct { ULONG v0, v1; } le64;
static inline void write_le64(le64 *s, ULONG v) { s->v0 = swap32(v); s->v1 = 0; }

typedef struct { ULONG v; } le32;
static inline ULONG read_le32(le32 *s)          { return swap32(s->v);  }
static inline void write_le32(le32 *s, ULONG v) { s->v = swap32(v);     }

typedef struct { UWORD v; } le16;
static inline UWORD read_le16(le16 *s)          { return swap16(s->v);  }
static inline void write_le16(le16 *s, UWORD v) { s->v = swap16(v);     }

#define VIRTIO_QUEUE_MAX    8       /* fix the size of virtqs to "very small" */

/* input device, 5.8.6 */
struct virtio_input_event {
  le16 type;
  le16 code;
  le32 value;
};

/* descriptor table, 2.7.5 */
struct virtq_desc {
        le64 addr;
        le32 len;
        le16 flags;
#define VIRTQ_DESC_F_NEXT       1
#define VIRTQ_DESC_F_WRITE      2
        le16 next;
} __attribute__((aligned(16)));

/* available ring, 2.7.6 */
struct virtq_avail {
        le16 flags;                     /* always zero */
        le16 idx;
        le16 ring[VIRTIO_QUEUE_MAX];
} __attribute__((aligned(2)));

/* used ring, 2.7.8 */
struct virtq_used_elem {
        le32 id;
        le32 len;
};

struct virtq_used {
        le16 flags;                     /* always zero */
        le16 idx;
        struct virtq_used_elem ring[VIRTIO_QUEUE_MAX];
} __attribute__((aligned(4)));

struct virtio_vq
{
    struct virtq_desc   desc[VIRTIO_QUEUE_MAX];
    struct virtq_used   used;
    struct virtq_avail  avail;
};

/* VirtIO BIOS info / entrypoint structure */
typedef struct {
    ULONG version;
    ULONG base_address;
    ULONG device_size;
    ULONG num_devices;
    ULONG (* acquire)(ULONG handle, void (*interrupt_handler)(ULONG), ULONG arg);
    ULONG (* release)(ULONG handle);
} virtio_dispatch_table_t;

#endif /* QEMU_H */
