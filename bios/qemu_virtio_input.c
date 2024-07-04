/*
 * QEMU VirtIO input (keyboard/mouse) drivers.
 */

#include "emutos.h"
#ifdef MACHINE_QEMU
#include "biosext.h"
#include "qemu.h"
#include "string.h"

enum virtio_input_config_select {
    VIRTIO_INPUT_CFG_UNSET      = 0x00,
    VIRTIO_INPUT_CFG_ID_NAME    = 0x01,
    VIRTIO_INPUT_CFG_ID_SERIAL  = 0x02,
    VIRTIO_INPUT_CFG_ID_DEVIDS  = 0x03,
    VIRTIO_INPUT_CFG_PROP_BITS  = 0x10,
    VIRTIO_INPUT_CFG_EV_BITS    = 0x11,
    VIRTIO_INPUT_CFG_ABS_INFO   = 0x12,
};

#define VIRTIO_INPUT_CFG_SELECT     (VIRTIO_MMIO_CONFIG + 0x00)
#define VIRTIO_INPUT_CFG_SUBSEL     (VIRTIO_MMIO_CONFIG + 0x01)
#define VIRTIO_INPUT_CFG_SIZE       (VIRTIO_MMIO_CONFIG + 0x02)
#define VIRTIO_INPUT_CFG_DATA       (VIRTIO_MMIO_CONFIG + 0x08)

struct qemu_virtio_input_driver
{
    ULONG                       handle;
    struct virtio_vq            vq;
    struct virtio_input_event   event[VIRTIO_QUEUE_MAX];
};

static struct qemu_virtio_input_driver vid_mouse;
static struct qemu_virtio_input_driver vid_keyboard;

static void
vid_handle_event(struct virtio_input_event *event)
{
    KDEBUG(("vid_handle_event: type 0x%04x code 0x%04x value 0x%08lx\n",
            read_le16(&event->type),
            read_le16(&event->code),
            read_le32(&event->value)));
    /* XXX TBD */
}

static void
vid_interrupt(ULONG arg)
{
    struct qemu_virtio_input_driver *vid = (struct qemu_virtio_input_driver *)arg;

    /* we always push entries back into the avail queue, so we know where we were at... */
    UWORD next_avail_idx = read_le16(&vid->vq.avail.idx);
    UWORD oldest_used_idx = next_avail_idx - VIRTIO_QUEUE_MAX;
    UWORD next_used_idx = read_le16(&vid->vq.used.idx);
    KDEBUG(("vid_interrupt: handle %lu next_avail %u oldest_used %u next_used %u\n", 
            vid->handle, next_avail_idx, oldest_used_idx, next_used_idx));
    while (oldest_used_idx != next_used_idx) {
        /* get the descriptor index, which maps 1:1 with our array of event bufers */
        UWORD desc_index = read_le32(&vid->vq.used.ring[oldest_used_idx % VIRTIO_QUEUE_MAX].id);

        /* pass the event off for processing */
        vid_handle_event(&vid->event[desc_index]);

        /* return the event to the avail ring */
        write_le16(&vid->vq.avail.ring[next_avail_idx % VIRTIO_QUEUE_MAX], desc_index);

        oldest_used_idx++;
        next_avail_idx++;
    }

    /* notify the device that we have updated the avail ring */
    write_le16(&vid->vq.avail.idx, next_avail_idx);
    __sync_synchronize();
    virtio_mmio_write32(vid->handle, VIRTIO_MMIO_QUEUE_NOTIFY, 0);
}

void
qemu_virtio_input_device_init(ULONG handle)
{
    /* what is this? */
    virtio_mmio_write8(handle, VIRTIO_INPUT_CFG_SELECT, VIRTIO_INPUT_CFG_ID_NAME);
    virtio_mmio_write8(handle, VIRTIO_INPUT_CFG_SUBSEL, 0);

    /* use the length of the name string to identify keyboard / mouse */
    struct qemu_virtio_input_driver *vid = NULL;
    switch (virtio_mmio_read8(handle, VIRTIO_INPUT_CFG_SIZE)) {
    case 18:    /* "QEMU Virtio Mouse" */
        vid = &vid_mouse;
        KDEBUG(("virtio%lu: mouse\n", handle));
        break;
    case 21:    /* "QEMU Virtio Keyboard" */
        vid = &vid_keyboard;
        KDEBUG(("virtio%lu: keyboard\n", handle));
        break;
    default:
        return;
    }
    vid->handle = handle;
    qemu_virtio_register_interrupt(handle, vid_interrupt, (ULONG)vid);

    /* tell the device a driver has appeared */
    ULONG status = VIRTIO_STAT_ACKNOWLEDGE | VIRTIO_STAT_DRIVER;
    virtio_mmio_write32(handle, VIRTIO_MMIO_STATUS, status);

    /* negotiate features */
    virtio_mmio_write32(handle, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
    virtio_mmio_write32(handle, VIRTIO_MMIO_DRIVER_FEATURES, 0);
    virtio_mmio_write32(handle, VIRTIO_MMIO_DRIVER_FEATURES_SEL, 1);
    virtio_mmio_write32(handle, VIRTIO_MMIO_DRIVER_FEATURES, VIRTIO_F_VERSION_1);
    status |= VIRTIO_STAT_FEATURES_OK;
    virtio_mmio_write32(handle, VIRTIO_MMIO_STATUS, status);
    if ((virtio_mmio_read32(handle, VIRTIO_MMIO_STATUS) & VIRTIO_STAT_FEATURES_OK) == 0) {
        KDEBUG(("virtio%lu: feature negotiation failed\n", handle));
        return;
    }

    /* acknowledge features */
    status |= VIRTIO_STAT_DRIVER_OK;
    virtio_mmio_write32(handle, VIRTIO_MMIO_STATUS, status);

    /* init the input buffers and stuff them into the avail ring */
    memset(&vid->vq, 0, sizeof(vid->vq));
    for (int i = 0; i < VIRTIO_QUEUE_MAX; i++) {
        write_le64(&vid->vq.desc[i].addr, (ULONG)&vid->event[i]);
        write_le32(&vid->vq.desc[i].len, sizeof(vid->event[i]));
        write_le16(&vid->vq.desc[i].flags, VIRTQ_DESC_F_WRITE);
        write_le16(&vid->vq.avail.ring[i], i);
    }
    write_le16(&vid->vq.avail.idx, VIRTIO_QUEUE_MAX);

    /* tell the device where the descriptors and rings are */
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_SEL, 0);
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_NUM, VIRTIO_QUEUE_MAX);
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_DESC_LOW, (ULONG)&vid->vq.desc[0]);
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_DESC_HIGH, 0);
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_AVAIL_LOW, (ULONG)&vid->vq.avail);
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_AVAIL_HIGH, 0);
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_USED_LOW, (ULONG)&vid->vq.used);
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_USED_HIGH, 0);
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_READY, 1);

    /* give the initial avail ring over to the driver */
    __sync_synchronize();
    virtio_mmio_write32(handle, VIRTIO_MMIO_QUEUE_NOTIFY, 0);
}

#endif /* MACHINE_QEMU */