/*
 * QEMU VirtIO functionality.
 *
 * Conforms to and spec references sections in OASIS VirtIO 1.2, MMIO mode only.
 *
 * Internal drivers:
 *  - Always use a fixed virtq size of 8 entries.
 *  - Do not negotiate VIRTIO_F_EVENT_IDX.
 *  - Always request notification for every update.
 */

#include "emutos.h"
#ifdef MACHINE_QEMU
#include "qemu.h"
#include "biosext.h"
#include "cookie.h"
#include "string.h"
#include "vectors.h"

enum {
    VD_NONE,
    VD_USER,
    VD_INPUT,
};

typedef struct
{
    BOOL                present;
    int                 owner;
    void                (* interrupt_callback)(ULONG handle);
    ULONG               interrupt_arg;
} virtio_dev_t;

static virtio_dev_t vio_devs[VIRTIO_NDEV];

static void
qemu_virtio_dump_dev(ULONG handle)
{
    KDEBUG(("virtio%lu:\n", handle));
    KDEBUG(("  MagicValue:          0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_MAGIC_VALUE)));
    KDEBUG(("  Version:             0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_VERSION)));
    KDEBUG(("  DeviceID:            0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_DEVICE_ID)));
    KDEBUG(("  VendorID:            0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_VENDOR_ID)));
    KDEBUG(("  DeviceFeatures0:     0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_DEVICE_FEATURES)));
    KDEBUG(("  DeviceFeaturesSel0:  0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_DEVICE_FEATURES_SEL)));
    virtio_mmio_write32(handle, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
    KDEBUG(("  DeviceFeatures1:     0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_DEVICE_FEATURES)));
    KDEBUG(("  DriverFeatures:      0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_DRIVER_FEATURES)));
    KDEBUG(("  DriverFeaturesSel:   0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_DRIVER_FEATURES_SEL)));
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_SEL, 0);
    KDEBUG(("  QueueSel:            0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_SEL)));
    KDEBUG(("  QueueNumMax:         0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_NUM_MAX)));
    KDEBUG(("  QueueNum:            0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_NUM)));
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_SEL, 1);
    KDEBUG(("  QueueSel:            0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_SEL)));
    KDEBUG(("  QueueNumMax1:        0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_NUM_MAX)));
    KDEBUG(("  QueueNum1:           0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_NUM)));
    KDEBUG(("  QueueReady:          0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_READY)));
    KDEBUG(("  QueueNotify:         0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_NOTIFY)));
    KDEBUG(("  InterruptStatus:     0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_INTERRUPT_STATUS)));
    KDEBUG(("  InterruptACK:        0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_INTERRUPT_ACK)));
    KDEBUG(("  Status:              0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_STATUS)));
    KDEBUG(("  QueueDescLow:        0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_DESC_LOW)));
    KDEBUG(("  QueueDescHigh:       0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_DESC_HIGH)));
    KDEBUG(("  QueueDriverLow:      0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_AVAIL_LOW)));
    KDEBUG(("  QueueDriverHigh:     0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_AVAIL_HIGH)));
    KDEBUG(("  QueueDeviceLow:      0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_USED_LOW)));
    KDEBUG(("  QueueDeviceHigh:     0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_QUEUE_USED_HIGH)));
    KDEBUG(("  ShmLenLow:           0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_SHM_LEN_LOW)));
    KDEBUG(("  ShmLenHigh:          0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_SHM_LEN_HIGH)));
    KDEBUG(("  ShmBaseLow:          0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_SHM_BASE_LOW)));
    KDEBUG(("  ShmBaseHigh:         0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_SHM_BASE_HIGH)));
    KDEBUG(("  ConfigGeneration:    0x%lx\n", virtio_mmio_read32(handle, VIRTIO_MMIO_CONFIG_GENERATION)));
}

__attribute__((interrupt))
static void
qemu_virtio_interrupt(void)
{
    KDEBUG(("qemu_virtio_interrupt()\n"));
    for (ULONG handle = 0; handle < VIRTIO_NDEV; handle++) {
        ULONG status = virtio_mmio_read32(handle, VIRTIO_MMIO_INTERRUPT_STATUS);
        virtio_mmio_write32(handle, VIRTIO_MMIO_INTERRUPT_ACK, status);
        if (status & VIRTIO_MMIO_INT_CONFIG) {
            if (virtio_mmio_read32(handle, VIRTIO_MMIO_STATUS) & VIRTIO_STAT_NEEDS_RESET) {
                KDEBUG(("virtio%lu: device requested reset\n", handle));
                virtio_mmio_write32(handle, VIRTIO_MMIO_STATUS, 0);
            } else {
                KDEBUG(("virtio%lu: config changed\n", handle));
            }
        }
        if (status & VIRTIO_MMIO_INT_VRING) {
            if (vio_devs[handle].interrupt_callback == 0) {
                panic("virtio%lu: interrupt but no handler\n", handle);
            }
            vio_devs[handle].interrupt_callback(vio_devs[handle].interrupt_arg);
        }
    }
}

void
qemu_virtio_init(void)
{
    KDEBUG(("qemu_virtio_init()\n"));

    for (WORD handle = 0; handle < VIRTIO_NDEV; handle++) {

        /* check magic / version */
        if ((virtio_mmio_read32(handle, VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976) ||
            (virtio_mmio_read32(handle, VIRTIO_MMIO_VERSION) != VIRTIO_MMIO_VERSION_SUPPORTED) ||
            (virtio_mmio_read32(handle, VIRTIO_MMIO_DEVICE_ID) == 0)) {
            continue;
        }

        /* require VERSION_1 feature */
        virtio_mmio_write32(handle, VIRTIO_MMIO_DEVICE_FEATURES_SEL, 1);
        if ((virtio_mmio_read32(handle, VIRTIO_MMIO_DEVICE_FEATURES) & VIRTIO_F_VERSION_1) == 0) {
            continue;
        }

        /* flag as present */
        vio_devs[handle].present = TRUE;

        /* reset */
        virtio_mmio_write32(handle, VIRTIO_MMIO_STATUS, 0);

        /* acknowledge device existence */
        virtio_mmio_write32(handle, VIRTIO_MMIO_STATUS, VIRTIO_STAT_ACKNOWLEDGE);

        /* check for internal support */
        switch (virtio_mmio_read32(handle, VIRTIO_MMIO_DEVICE_ID)) {
        case VIRTIO_DEVICE_INPUT:  /* input device */
            qemu_virtio_input_device_init(handle);
            vio_devs[handle].owner = VD_INPUT;
            break;
        default:
            qemu_virtio_dump_dev(handle);
            break;
        }
    }
}


/*
 * External API.
 */

void
qemu_virtio_register_interrupt(ULONG handle, void (* handler)(ULONG), ULONG arg)
{
    /* claim the vector back from MiNT? */
    VEC_LEVEL2 = qemu_virtio_interrupt;
    vio_devs[handle].interrupt_callback = handler;
    vio_devs[handle].interrupt_arg = arg;
}

static BOOL
qemu_virtio_handle_valid(ULONG handle)
{
    return (handle < VIRTIO_NDEV) && vio_devs[handle].present;
}

static ULONG
virtio_acquire(ULONG handle, void (*interrupt_handler)(ULONG), ULONG arg)
{
    if (!qemu_virtio_handle_valid(handle) ||
        vio_devs[handle].owner != VD_NONE) {
        return VIRTIO_ERROR;
    }
    vio_devs[handle].owner = VD_USER;
    qemu_virtio_register_interrupt(handle, interrupt_handler, arg);
    return VIRTIO_SUCCESS;
}

static ULONG
virtio_release(ULONG handle)
{
    if (!qemu_virtio_handle_valid(handle) ||
        (vio_devs[handle].owner != VD_USER)) {
        return VIRTIO_ERROR;
    }
    vio_devs[handle].owner = VD_NONE;
    vio_devs[handle].interrupt_callback = NULL;
    return VIRTIO_SUCCESS;
}

static const virtio_dispatch_table_t qemu_virtio_dispatch_table = {
    .version = 0x56494f30,
    .base_address = VIRTIO_BASE,
    .device_size = VIRTIO_DEVSIZE,
    .num_devices = VIRTIO_NDEV,
    .acquire = virtio_acquire,
    .release = virtio_release,
};

void
qemu_virtio_add_cookies(void)
{
    /* install the _VIO cookie */
    cookie_add(0x5f56494f, (ULONG)&qemu_virtio_dispatch_table);
}


#endif /* MACHINE_QEMU */
