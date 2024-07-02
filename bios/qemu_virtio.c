/*
 * QEMU VirtIO functionality.
 */

#include "emutos.h"
#ifdef MACHINE_QEMU
#include "qemu.h"
#include "cookie.h"
#include "pcireg.h"

#define VIO_MAX_DEVS        8
#define VIO_MAX_TYPE        0x40

static const struct
{
    UWORD   from;
    UWORD   to;
} vio_id_map[] = {
    {0x1000, 0x1041},   /* network card */
    {0x1001, 0x1042},   /* block device */
    {0x1002, 0x1045},   /* memory ballooning (traditional) */
    {0x1003, 0x1043},   /* console */
    {0x1004, 0x1048},   /* SCSI host */
    {0x1005, 0x1044},   /* entropy source */
    {0x1009, 0x1049},   /* 9P transport */
    {0x0000, 0x1040}
};

/* VIO over PCI */

#define VIO_PCI_VENDOR      0x1AF4
#define VIO_PCI_MINDEV      0x1040
#define VIO_PCI_MAXDEV      0x107f

#define VIO_PCI_CAP_COMMON_CFG  1
#define VIO_PCI_CAP_NOTIFY_CFG  2
#define VIO_PCI_CAP_ISR_CFG     3
#define VIO_PCI_CAP_DEVICE_CFG  4
#define VIO_PCI_CAP_PCI_CFG     5

typedef struct
{
    LONG    pci_handle;
    UWORD   device_id;
    ULONG   common_cfg;
    ULONG   notify_cfg;
    ULONG   isr_cfg;
    ULONG   device_cfg;
    ULONG   pci_cfg;
    ULONG   notify_multiplier;
} vio_pci_dev_t;

static vio_pci_dev_t vio_pci_devs[VIO_MAX_DEVS];

static UWORD vio_pci_num_devs;

static ULONG
vio_ptr_for_cap(LONG pci_handle, LONG capptr)
{
    UBYTE bar = qemu_pci_fast_read_config_byte(pci_handle, capptr + 4);
    ULONG offset = qemu_pci_fast_read_config_longword(pci_handle, capptr + 8);
    const struct pci_resource_info *rsc = (const struct pci_resource_info *)qemu_pci_get_resource(pci_handle);

    /* scan resources for a matching BAR */
    do {
        if (rsc->bar == bar) {
            return rsc->start + offset;
        }
    } while (!((rsc++)->flags & PCI_RSC_LAST));

    return 0;
}

void
qemu_vio_init(void)
{
    KDEBUG(("qemu_vio_init()\n"));

    /* scan for PCI VirtIO devices */
    for (UWORD pci_index = 0; vio_num_devs < VIO_MAX_DEVS; pci_index++) {
        ULONG pci_handle = qemu_pci_find_pci_device(0xffffUL, pci_index);
        if (pci_handle == PCI_DEVICE_NOT_FOUND) {
            break;
        }

        /* check for VirtIO vendor ID */
        if (qemu_pci_fast_read_config_word(pci_handle, PCIR_VENDOR) == VIO_PCI_VENDOR) {
            UWORD pci_device = qemu_pci_fast_read_config_word(pci_handle, PCIR_DEVICE);

            /* translate transitional IDs */
            for (UWORD i = 0; vio_id_map[i].from != 0; i++) {
                if (pci_device == vio_id_map[i].from) {
                    pci_device = vio_id_map[i].to;
                    break;
                }
            }

            /* if recognised, track */
            if ((pci_device > VIO_PCI_MINDEV) && (pci_device < VIO_PCI_MAXDEV)) {
                vio_dev_t *vdev = &vio_pci_devs[vio_num_devs];
                LONG capptr = 0;

                vdev->pci_handle = pci_handle;
                vdev->device_id = pci_device - VIO_PCI_MINDEV;

                /* scan for caps with config offsets */
                for (;;) {
                    capptr = qemu_pci_get_next_cap(pci_handle, capptr);
                    if (capptr == 0) {
                        break;
                    }
                    if (qemu_pci_fast_read_config_byte(pci_handle, capptr) == PCIY_VENDOR) {
                        ULONG cfg_addr = vio_ptr_for_cap(pci_handle, capptr);
                        UBYTE cap_type = qemu_pci_fast_read_config_byte(pci_handle, capptr + 3);

                        switch (cap_type) {
                        case VIO_PCI_CAP_COMMON_CFG:
                            if (vdev->common_cfg == 0) {
                                vdev->common_cfg = cfg_addr;
                            }
                            break;
                        case VIO_PCI_CAP_NOTIFY_CFG:
                            if (vdev->notify_cfg == 0) {
                                vdev->notify_cfg = cfg_addr;
                                vdev->notify_multiplier = qemu_pci_fast_read_config_longword(pci_handle, capptr + 16);
                            }
                            break;
                        case VIO_PCI_CAP_ISR_CFG:
                            if (vdev->isr_cfg == 0) {
                                vdev->isr_cfg = cfg_addr;
                            }
                            break;
                        case VIO_PCI_CAP_DEVICE_CFG:
                            if (vdev->device_cfg == 0) {
                                vdev->device_cfg = cfg_addr;
                            }
                            break;
                        case VIO_PCI_CAP_PCI_CFG:
                            if (vdev->pci_cfg == 0) {
                                vdev->pci_cfg = cfg_addr;
                            }
                            break;
                        default:
                            KDEBUG(("virtio %d: unhandled cap type %d\n", vio_num_devs, cap_type));
                        }
                    }
                }

                KDEBUG(("virtio%d: type %d\n", vio_num_devs, vdev->device_id));
                KDEBUG(("  common @ 0x%08lx\n", vdev->common_cfg));
                KDEBUG(("  notify @ 0x%08lx\n", vdev->notify_cfg));
                KDEBUG(("    multiplier 0x%08lx\n", vdev->notify_multiplier));
                KDEBUG(("  isr    @ 0x%08lx\n", vdev->isr_cfg));
                KDEBUG(("  device @ 0x%08lx\n", vdev->device_cfg));
                KDEBUG(("  pcicfg @ 0x%08lx\n", vdev->pci_cfg));
                vio_num_devs++;
            }
        }
    }
}

/*
 * API
 */
ULONG
qemu_vio_find_device(ULONG device_id, ULONG index)
{
    for (UWORD i = 0; i < vio_num_devs; i++) {
        if (device_id == vio_pci_devs[i].device_id) {
            if (index-- == 0) {
                return i;
            }
        }
    }
    return VIO_NOT_FOUND;
}

static const virtio_dispatch_table_t qemu_vio_dispatch_table = {
    0x56494f30,
    qemu_vio_find_device
};

void
qemu_vio_add_cookies(void)
{
    /* install the _VIO cookie */
    cookie_add(0x5f56494f, (ULONG)&qemu_vio_dispatch_table);
}


#endif /* MACHINE_QEMU */
