/*
 * pt68k5_installbootc.c
 *
 * Copyright (C) 2023 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/*
 * Bootloader installer for PT68K5.
 *
 * Installs a raw binary loader to an MBR-partitioned disk / image, or to a
 * 1.44MB floppy image in a fashion that causes MONK5 to think it's a bootable
 * REX disk.
 *
 * The loader can determine where it's been loaded from by examining A2:
 *
 * 0x2000_4180 - onboard IDE interface
 * 0x1000_0300 - XTIDE card
 * 0x1000_03F4 - floppy
 *
 * When loaded from the onboard or XTIDE interfaces, the byte at 0x0000_0fff
 * will be zero if loaded from the master, or 0x10 if loaded from the slave.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

const uint32_t floppy_tracks = 80;
const uint32_t floppy_sectors = (17 * 2 * 2);
const uint32_t floppy_sector_size = 256;
const size_t floppy_size = floppy_tracks * floppy_sectors * floppy_sector_size;
const uint32_t sector_header_size = 4;
const uint32_t sector_data_size = floppy_sector_size - sector_header_size;
const uint8_t load_cmd_tag = 0x01;
const uint8_t entry_cmd_tag = 0x16;
const uint32_t load_cmd_size = 7;
const uint32_t entry_cmd_size = 5;
const uint32_t load_address = 0x00200000;   /* load at 2M */

const char *g_argv0;

#define SECTOR_SIZE 512

#pragma pack(push, 1)
typedef struct {
    uint8_t     active;
    uint8_t     _res0[3];
    uint8_t     type;
    uint8_t     _res1[3];
    uint32_t    start;
    uint32_t    size;
} mbr_partition;

typedef struct {
    uint8_t     _res0[446];
    mbr_partition   partition[4];
    uint8_t     magic[2];
} mbr;

union {
    uint8_t     raw[SECTOR_SIZE];
    mbr         mbr;
} disk_buf;

#pragma pack(pop)

struct {
    uint8_t     *exec;
    size_t      size;
} booter;

static void
error(const char *msg, ...)
{
    va_list ap;
    int err = errno;

    fprintf(stderr, "%s: ERROR: ", g_argv0);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(err));
    exit(1);
}

static void
errorx(const char *msg, ...)
{
    va_list ap;

    fprintf(stderr, "%s: ERROR: ", g_argv0);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void
usage(void)
{
    errorx(" usage %s <booter> <disk or diskimage>");
}

static off_t
fd_size(int fd)
{
    struct stat st;

    if (fstat(fd, &st) < 0) {
        error("getting file size");
    }
    return st.st_size;
}

static uint8_t *
make_rex_exec(int fd, size_t *buf_size)
{
    ssize_t file_size = (ssize_t)fd_size(fd);
    size_t payload_size = file_size + load_cmd_size + entry_cmd_size;

    uint8_t *buf = malloc(payload_size);

    if (file_size > 0xffff) {
        errorx("booter too large");
    }

    /* construct a REX executable using the 0x01/0x16 load commands in our temporary buffer */

    /*
     * MONK5 doesn't handle the entrypoint tag properly when it crosses a sector
     * boundary, so insert it at the head of the booter.
     */
    buf[0] = entry_cmd_tag;
    buf[1] = (load_address >> 24) & 0xff;
    buf[2] = (load_address >> 16) & 0xff;
    buf[3] = (load_address >> 8) & 0xff;
    buf[4] = load_address & 0xff;

    /*
     * Booter is limited to ~16k due to the typical location of the first partition,
     * so a single load command is sufficient.
     */
    buf[5] = load_cmd_tag;
    buf[6] = (load_address >> 24) & 0xff;
    buf[7] = (load_address >> 16) & 0xff;
    buf[8] = (load_address >> 8) & 0xff;
    buf[9] = load_address & 0xff;
    buf[10] = (file_size >> 8) & 0xff;
    buf[11] = file_size & 0xff;

    if (read(fd, &buf[12], file_size) != file_size) {
        error("reading");
    }

    if (buf_size) {
        *buf_size = payload_size;
    }
    return buf;
}

static void
read_booter(const char *name)
{
    int fd = open(name, O_RDONLY);

    if (fd < 0) {
        error("opening %s", name);
    }
    booter.exec = make_rex_exec(fd, &booter.size);
    close(fd);
}

static void
update_floppy_image(int fd)
{
    const size_t required_size = booter.size + ((booter.size + sector_data_size - 1) / sector_data_size) * sector_header_size;
    uint32_t track = 0;
    uint32_t sector = 1;
    uint32_t file_block = 0;
    size_t offset = 0;

    if (required_size > floppy_size) {
        errorx("booter too large for floppy");
    }

    /* copy the booter into the bootfile */
    while (offset < booter.size) {
        size_t copy_size = booter.size - offset;

        if (copy_size <= sector_data_size) {
            /* last sector in file */
            disk_buf.raw[0] = 0;
            disk_buf.raw[1] = 0;
        } else {
            /* link to next sector */
            if (sector++ == floppy_sectors) {
                track++;
                sector = 1;
            }
            disk_buf.raw[0] = track & 0xff;
            disk_buf.raw[1] = sector & 0xff;
        }
        disk_buf.raw[2] = (file_block >> 8) & 0xff;
        disk_buf.raw[3] = file_block & 0xff;

        /* copy at most one sector worth of data */
        if (copy_size > sector_data_size) {
            copy_size = sector_data_size;
        }
        memcpy(&disk_buf.raw[sector_header_size], &booter.exec[offset], copy_size);

        /* if there's a chance there's garbage following the sector, zero it out */
        if (copy_size < sector_data_size) {
            memset(&disk_buf.raw[sector_header_size + copy_size], 0, sector_data_size - copy_size);
        }

        /* write the sector out to the image */
        if (write(fd, &disk_buf.raw[0], floppy_sector_size) != floppy_sector_size) {
            error("writing");
        }

        file_block++;
        offset += copy_size;
    }
}

static void
update_hd_image(int fd)
{
    size_t offset = 0;
    uint32_t lba = 1;
    uint32_t file_block = 0;
    uint32_t required_sectors = booter.size / floppy_sector_size;
    int i;

    /* read the MBR and sanity-check */
    if (read(fd, &disk_buf.mbr, SECTOR_SIZE) != SECTOR_SIZE) {
        error("reading MBR");
    }
    if ((disk_buf.mbr.magic[0] != 0x55) ||
        (disk_buf.mbr.magic[1] != 0xaa)) {
        errorx("image file magic %02x,%02xx not supported", disk_buf.mbr.magic[0], disk_buf.mbr.magic[1]);
    }
    if (!((disk_buf.mbr.partition[0].active & 0x80) |
          (disk_buf.mbr.partition[1].active & 0x80) |
          (disk_buf.mbr.partition[2].active & 0x80) |
          (disk_buf.mbr.partition[3].active & 0x80))) {
        fprintf(stderr, "WARNING: no active partition, disk will not be bootable\n");
    }
    for (i = 0; i < 4; i++) {
        if ((disk_buf.mbr.partition[i].size != 0) &&
            (disk_buf.mbr.partition[i].start < required_sectors)) {
            errorx("booter (%u sectors) would overlap partition %d starting at %u", 
                   (unsigned int)required_sectors, i, (unsigned int)disk_buf.mbr.partition[i].start);
        }
    }

    /* copy the booter into the bootfile */
    while (offset < booter.size) {
        size_t copy_size = booter.size - offset;

        /* set file link / block number fields */
        if (copy_size <= sector_data_size) {
            /* last sector in file */
            disk_buf.raw[0] = 0;
            disk_buf.raw[1] = 0;
        } else {
            /* link to next sector */
            disk_buf.raw[0] = ((lba + 1) >> 8) & 0xff;
            disk_buf.raw[1] = (lba + 1) & 0xff;
        }
        disk_buf.raw[2] = (file_block >> 8) & 0xff;
        disk_buf.raw[3] = file_block & 0xff;
        file_block++;

        /* copy at most one sector worth of data */
        if (copy_size > sector_data_size) {
            copy_size = sector_data_size;
        }
        memcpy(&disk_buf.raw[4], &booter.exec[offset], copy_size);

        /*
         * If there's a chance there's garbage following the sector, zero it out.
         * Don't do this for lba==0 as the partition table is there.
         */
        if ((lba == 1) || (copy_size < sector_data_size)) {
            memset(&disk_buf.raw[sector_header_size + copy_size], 0, SECTOR_SIZE - copy_size);
        }

        /* write the sector out to the file */
        if (write(fd, &disk_buf.raw[0], SECTOR_SIZE) != SECTOR_SIZE) {
            error("writing LBA %d", lba);
        }

        lba++;
        offset += copy_size;
    }
    fprintf(stdout, "booter used sectors 1-%u\n", (unsigned int)lba - 1);
}

int
main(int argc, const char **argv)
{
    int imagefd = -1;

    g_argv0 = argv[0]; /* Remember the program name */
    if (argc != 3) {
        usage();
    }

    read_booter(argv[1]);

    imagefd = open(argv[2], O_RDWR);
    if (imagefd < 0) {
        error("opening %s", argv[2]);
    }
    if (fd_size(imagefd) == floppy_size) {
        update_floppy_image(imagefd);
    } else {
        update_hd_image(imagefd);
    }
    return 0;
}
