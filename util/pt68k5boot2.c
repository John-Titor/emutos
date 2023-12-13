/*
 * pt68k5boot2.c
 *
 * Copyright (C) 2023 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include <emutos.h>
#include <stdarg.h>
#include <doprintf.h>

#define DEBUG 0

/* <stdint.h> types */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

/* pt68k5boot.S interface */
extern void outc(int c);
extern int probe(uint32_t addr);
extern uint32_t trap12(uint32_t op, uint32_t arg1, uint32_t arg2, const void *addr);
extern void jump_to_loaded_os(uint32_t entrypoint) NORETURN;

uint32_t ram_size;
void boot_main(uint32_t base_addr, uint32_t slave);

static inline uint16_t swap16(uint16_t val)
{
    __asm__ volatile("rolw #8,%0" : "=d"(val) : "0"(val));
    return val;
}

static inline uint32_t swap32(uint32_t val)
{
    __asm__ volatile("rolw #8,%0; swap %0; rolw #8,%0" : "=d"(val) : "0"(val));
    return val;
}

static int printf(const char *fmt, ...)
{
    int n;
    va_list ap;
    va_start(ap, fmt);
    n = doprintf(outc, fmt, ap);
    va_end(ap);
    return n;
}

static int dprintf(const char *fmt, ...)
{
#if DEBUG
    int n;
    va_list ap;
    va_start(ap, fmt);
    n = doprintf(outc, fmt, ap);
    va_end(ap);
    return n;
#else
    return 0;
#endif
}

static const uint32_t ide_base = 0x20004180lu;
static const uint32_t xtide_base = 0x10000300lu;

#define DISK_BLOCK_SIZE 512lu

#pragma pack(push, 1)

typedef struct {
    uint8_t     active;
    uint8_t     _res0[3];
    uint8_t     type;
    uint8_t     _res1[3];
    uint32_t    _le_start;
    uint32_t    _le_size;
} dos_part;

typedef struct {
    uint8_t     _res0[446];
    dos_part    partition[4];
    uint16_t    magic;
} dos_mbr;

typedef struct {
    uint8_t     flag;
    uint8_t     id[3];
    uint32_t    start;
    uint32_t    size;
} atari_part;

typedef struct {
    uint8_t     _res0[454];
    atari_part  partition[4];
    uint8_t     _res1[10];
} atari_rs;

typedef struct {
    uint8_t     jmp[3];
    uint8_t     _res0[8];
    uint16_t    _le_bps;
    uint8_t     spc;
    uint16_t    _le_nres;
    uint8_t     nfat;
    uint16_t    _le_ndirent;
    uint16_t    _le_nsec1;
    uint8_t     mdesc;
    uint16_t    _le_fatsiz1;
} fat_vbr;

typedef struct {
    char        name[11];
    uint8_t     attr;
    uint8_t     _res0[14];
    uint16_t    _le_start;
    uint32_t    _le_size;
} fat_dirent;

typedef struct {
    uint16_t    bra;
    uint16_t    tos_version;
    uint32_t    main;
    uint32_t    os_beg;
    uint32_t    os_end;
    uint32_t    pad0[7];
    uint8_t     etos_id[4];
} emutos_hdr;

#pragma pack(pop)

struct {
    /* disk module state */
    uint32_t    iobase;             /* I/O address for disk controller */
    uint32_t    cached_block;       /* cached block address or ~0ul */
    uint8_t     unit;               /* 0 for master, 0x10 for slave */
    uint8_t     swap;               /* nonzero to swap words read from disk */

    /* LBA offsets for open partiton */
    uint32_t    lba_base;           /* partition base LBA (0 for whole disk) */
    uint32_t    fat_base;           /* offset from lba_base to start of FAT 0 */
    uint32_t    dir_base;           /* offset from lba_base to start of root directory */
    uint32_t    data_base;          /* offset from lba_base to start of data area */

    /* FS data */
    uint32_t    dir_size;           /* number of entries in root directory */
    uint32_t    cluster_size;       /* size of cluster in blocks */

    /* file data */
    uint32_t    f_first_cluster;    /* first cluster in open file */
    uint32_t    f_size;             /* size of open file in blocks */
    uint32_t    f_cur_block;        /* file-relative block */
    uint32_t    f_cur_cluster;      /* cluster holding f_cur_block */

    union {
        uint16_t    fat16[DISK_BLOCK_SIZE / 2];
        dos_mbr     mbr;
        atari_rs    rs;
        fat_vbr     vbr;
        fat_dirent  dir[DISK_BLOCK_SIZE / sizeof(fat_dirent)];
        emutos_hdr  etos;
    } buf;
} disk_state;


#define REG8(_x)    *(volatile uint8_t *)(_x)
#define REG16(_x)  *(volatile uint16_t *)(_x)
#define IDE_DATA16              REG16(disk_state.iobase + 0x00)
#define IDE_DATA8               REG8(disk_state.iobase + 0x00)
#define IDE_ERROR               REG8(disk_state.iobase + 0x03)
#define IDE_ERROR_ID_NOT_FOUND      0x10
#define IDE_ERROR_UNCORRECTABLE     0x40
#define IDE_FEATURE             REG8(disk_state.iobase + 0x03)
#define IDE_SECTOR_COUNT        REG8(disk_state.iobase + 0x05)
#define IDE_LBA_0               REG8(disk_state.iobase + 0x07)
#define IDE_LBA_1               REG8(disk_state.iobase + 0x09)
#define IDE_LBA_2               REG8(disk_state.iobase + 0x0b)
#define IDE_LBA_3               REG8(disk_state.iobase + 0x0d)
#define IDE_LBA_3_DEV1              0x10
#define IDE_LBA_3_LBA               0xe0    /* incl. bits 7/5 for compat */
#define IDE_STATUS              REG8(disk_state.iobase + 0x0f)
#define IDE_STATUS_ERR              0x01
#define IDE_STATUS_DRQ              0x08
#define IDE_STATUS_DF               0x20
#define IDE_STATUS_DRDY             0x40
#define IDE_STATUS_BSY              0x80
#define IDE_COMMAND             REG8(disk_state.iobase + 0x0f)
#define IDE_CMD_NOP                 0x00
#define IDE_CMD_READ_SECTORS        0x20
#define IDE_CMD_WRITE_SECTORS       0x30
#define IDE_CMD_IDENTIFY_DEVICE     0xec

/* read a single disk block */
static int
disk_read(void *buffer, uint32_t lba)
{
    uint32_t   timeout = 0x200000;

    lba += disk_state.lba_base;
    IDE_LBA_3 = ((lba >> 24) & 0x3f) | IDE_LBA_3_LBA | disk_state.unit;
    IDE_LBA_2 = (lba >> 16) & 0xff;
    IDE_LBA_1 = (lba >> 8) & 0xff;
    IDE_LBA_0 = lba & 0xff;
    IDE_SECTOR_COUNT = 1;
    IDE_COMMAND = IDE_CMD_READ_SECTORS;

    while (--timeout) {
        uint8_t status = IDE_STATUS;

        if (status & IDE_STATUS_BSY) {
            continue;
        }
        if (status & IDE_STATUS_ERR) {
            printf("error 0x%x reading 0x%lx\n", IDE_ERROR, lba);
            return -1;
        }
        if (status & IDE_STATUS_DRQ) {
            uint16_t *bp = (uint16_t *)buffer;
            for (unsigned idx = 0; idx < DISK_BLOCK_SIZE; idx += 2) {
                if (disk_state.swap) {
                    *bp++ = swap16(IDE_DATA16);
                } else {
                    *bp++ = IDE_DATA16;
                }
            }
            return 0;
        }
    }
    printf("timeout reading 0x%lx\n", lba);
    return -1;
}

/* read and cache a single block in the internal buffer */
static int
disk_read_cached(uint32_t lba)
{
    if (disk_state.cached_block != lba) {
        if (disk_read(&disk_state.buf, lba)) {
            disk_state.cached_block = ~0ul;
            return -1;
        }
        disk_state.cached_block = lba;
    }
    return 0;
}

/* read n blocks into the given buffer; ignores the cache */
static int
disk_read_n(void *buffer, uint32_t lba, uint32_t count)
{
    uint8_t *buf = (uint8_t *)buffer;
    while(count--) {
        if (disk_read(buf, lba)) {
            return -1;
        }
        buf += DISK_BLOCK_SIZE;
        lba++;
    }
    return 0;
}

/*
 * Attempt to identify the currently-configured disk.
 */
static int
disk_select(void)
{
    /* reset disk state and read the first block */
    disk_state.swap = 0;            /* not swapped */
    disk_state.lba_base = 0;        /* not reading a partition */
    disk_state.f_cur_cluster = 0;   /* no open file */
    disk_state.f_cur_block = 0;
    disk_state.cached_block = ~0ul; /* invalidate disk block cache */
    if (disk_read_cached(0)) {
        return -1;
    }

    /* identify swapped DOS MBR or atari partitions */
    if ((disk_state.buf.mbr.magic == 0xaa55) ||
        ((disk_state.buf.rs.partition[0].id[1] == 'G') && (disk_state.buf.rs.partition[0].id[2] == 'M')) /*||
        ((disk_state.buf.rs.partition[0].id[1] == '2') && (disk_state.buf.rs.partition[0].id[2] == '3'))*/) {
        disk_state.swap = 1;
        if (disk_read_cached(0)) {
            return -1;
        }
        dprintf("disk is byte-swapped\n");
    }
    if (disk_state.buf.mbr.magic == 0x55aa) {
        int i;

        for (i = 0; i < 4; i++) {
            if (disk_state.buf.mbr.partition[i].active == 0x80) {
                switch (disk_state.buf.mbr.partition[i].type) {
                case 0x06:
                case 0x0e:
                    disk_state.lba_base = swap32(disk_state.buf.mbr.partition[i]._le_start);    /* select partition */
                    disk_state.cached_block = ~0ul;                                             /* invalidate disk block cache */
                    if (disk_read_cached(0)) {
                        return -1;
                    }
                    if ((disk_state.buf.vbr.jmp[0] != 0xeb) ||
                        (disk_state.buf.vbr.jmp[2] != 0x90)) {
                        dprintf("[%x,%x] ", (unsigned)disk_state.buf.vbr.jmp[0], (unsigned)disk_state.buf.vbr.jmp[2]);
                        printf("active partition has unsupported or no filesystem\n");
                        return -1;
                    }
                    if (swap16(disk_state.buf.vbr._le_bps) != DISK_BLOCK_SIZE) {
                        printf("block size %d not supported\n", swap16(disk_state.buf.vbr._le_bps));
                        return -1;
                    }
                    disk_state.cluster_size = disk_state.buf.vbr.spc;
                    disk_state.fat_base = swap16(disk_state.buf.vbr._le_nres);
                    disk_state.dir_base = swap16(disk_state.buf.vbr._le_nres) + disk_state.buf.vbr.nfat * swap16(disk_state.buf.vbr._le_fatsiz1);
                    disk_state.dir_size = swap16(disk_state.buf.vbr._le_ndirent);
                    disk_state.data_base = (disk_state.fat_base +
                                            (disk_state.buf.vbr.nfat * swap16(disk_state.buf.vbr._le_fatsiz1)) +
                                            (disk_state.dir_size * 32 / DISK_BLOCK_SIZE) -
                                            (2 * disk_state.cluster_size));
                    dprintf("cluster size %u\n", (unsigned int)disk_state.cluster_size);
                    dprintf("fat @ 0x%lx dir @ 0x%lx/%u data @ 0x%lx\n", 
                            disk_state.fat_base,
                            disk_state.dir_base, (unsigned int)disk_state.dir_size,
                            disk_state.data_base);
                    return 0;
                default:
                    printf("active partition has unsupported type\n");
                }
            }
        }
        printf("no bootable partition\n");
        return -1;
    }
/*
    for (i = 0; i < 4; i++) {
        if ((disk_state.buf.rs.partition[i].flag == 0x81) &&
            (disk_state.buf.rs.partition[i].id[0] == 'B') &&
            (disk_state.buf.rs.partition[i].id[1] == 'G') &&
            (disk_state.buf.rs.partition[i].id[2] == 'M')) {

            printf("!Atari");
            return -1;
        }
    }
*/

    printf(" format not supported");
    return -1;
}

/*
 * For a given cluster number, return the number of the next cluster or 0 to signal EOF.
 */
static uint32_t
disk_next_cluster(uint32_t cluster)
{
    static const uint32_t clusters_per_fat_block = DISK_BLOCK_SIZE / 2;
    uint32_t fat_lba = cluster / clusters_per_fat_block;
    uint32_t fat_ofs = cluster % clusters_per_fat_block;

    dprintf("reading FAT block 0x%lx\n", disk_state.lba_base + disk_state.fat_base + fat_lba);
    if (disk_read_cached(disk_state.fat_base + fat_lba)) {
        return 0;
    }
    dprintf("0x%lx @ 0x%lx/0x%lx", cluster, fat_lba, fat_ofs);
    cluster = swap16(disk_state.buf.fat16[fat_ofs]);
    dprintf(" -> 0x%lx\n", cluster);
    if ((cluster < 2) || (cluster > 0xffef)) {
        return 0;
    }
    return cluster;
}

/*
 * Seek the open file back to the beginning.
 */
static void
file_reset(void)
{
    disk_state.f_cur_cluster = disk_state.f_first_cluster;
    disk_state.f_cur_block = 0;
}

/*
 * Open the named file in the root directory of the currently-selected disk.
 */
static int
file_open(const char *name)
{
    uint16_t entry;
    for (entry = 0; entry < disk_state.dir_size; entry++) {
        uint16_t index = entry % (DISK_BLOCK_SIZE / sizeof(fat_dirent));
        uint16_t block = entry / (DISK_BLOCK_SIZE / sizeof(fat_dirent));
        int i;

        if (index == 0) {
            if (disk_read_cached(disk_state.dir_base + block)) {
                return -1;
            }
        }
        if (disk_state.buf.dir[index].attr & 0x18) {
            continue;   // subdirectory or volume label
        }
        if (disk_state.buf.dir[index].name[0] == 0) {
            break;      // no more used entries after this
        }
        for (i = 0;; i++) {
            if (name[i] != disk_state.buf.dir[index].name[i]) {
                break;  // name mismatch
            }
            if ((i + 1) == sizeof(disk_state.buf.dir[index].name)) {
                /* open the file */
                disk_state.f_first_cluster = swap16(disk_state.buf.dir[index]._le_start);
                disk_state.f_size = (swap32(disk_state.buf.dir[index]._le_size) + DISK_BLOCK_SIZE - 1) / DISK_BLOCK_SIZE;
                file_reset();
                return 0;
            }
        }
    }
    printf("bootfile not found\n");
    return -1;
}

/*
 * Read blocks from the open file.
 *
 * Reads are
 */
static int
file_read(uint8_t *buf, uint32_t block_count)
{
    if ((disk_state.f_cur_block + block_count) > disk_state.f_size) {
        printf("read past end of file\n");
        return -1;
    }
    for (;;) {
        uint32_t cluster_offset = disk_state.f_cur_block % disk_state.cluster_size;
        uint32_t read_count = disk_state.cluster_size - cluster_offset;
        uint32_t lba = disk_state.data_base + disk_state.f_cur_cluster * disk_state.cluster_size + cluster_offset;
        uint32_t next_cluster;

        if (read_count > block_count) {
            read_count = block_count;
        }

        if (disk_read_n(buf, lba, read_count)) {
            return -1;
        }
        block_count -= read_count;
        if (block_count == 0) {
            return 0;
        }
        disk_state.f_cur_block += read_count;
        buf += read_count * DISK_BLOCK_SIZE;

        next_cluster = disk_next_cluster(disk_state.f_cur_cluster);
        if (next_cluster == 0) {
            printf("FAT corrupt\n");
            return -1;
        }
        disk_state.f_cur_cluster = next_cluster;
    }
}

/*
 * Read a block from the open file into the cache buffer.
 */
static int
file_read_cached(void)
{
    /* invalidate cache */
    disk_state.cached_block = ~0ul;

    /* and read into cache buffer */
    if (file_read((uint8_t *)&disk_state.buf, 1)) {
        return -1;
    }
    return 0;
}

/*
 * Do video init, return base address of display memory.
 *
 * VGA mode 12 yields a 640x480x4 display with plane 0
 * at 0x080a_0000, effectively resulting in a linear
 * monochrome framebuffer.
 */
static void
video_init(void)
{
    /* standard Atari black-on-white palette */
    static const uint8_t palette[16] = {
        0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    trap12(0x80, 0, 0, 0);              /* screen off */
    trap12(0x12, 0, 0, 0);              /* VGA mode */
    trap12(0x85, 0, 0, &palette[0]);    /* load palette */
    trap12(0x89, 0, 0, 0);              /* clear video memory */
    trap12(0x81, 0, 0, 0);              /* screen on */
    trap12(0x90, 5, 5, 0);              /* position cursor */
    trap12(0x91, 15, 0, "EmuTOS loading...");
}

/*
 * Validate the EmuTOS header on the currently open file, return the load address.
 */
static uint8_t *
emutos_header_check(void)
{
    file_reset();

    /* sanity check the header and extract load address */
    if ((disk_state.f_size < ((64lu * 1024) / DISK_BLOCK_SIZE)) ||
        (disk_state.f_size > ((1024lu * 1024) / DISK_BLOCK_SIZE)) ||
        file_read_cached() ||
        (disk_state.buf.etos.etos_id[0] != 'E') ||
        (disk_state.buf.etos.etos_id[1] != 'T') ||
        (disk_state.buf.etos.etos_id[2] != 'O') ||
        (disk_state.buf.etos.etos_id[3] != 'S') ||
        (disk_state.buf.etos.os_beg < 0x800) ||
        (disk_state.buf.etos.os_beg > disk_state.buf.etos.os_end) ||
        (disk_state.buf.etos.main < disk_state.buf.etos.os_beg) ||
        (disk_state.buf.etos.main >= disk_state.buf.etos.os_end) ||
        (disk_state.buf.etos.os_end >= 0x00100000)) {
        printf("Bad EmuTOS image\n");
        return 0;
    }
    return (uint8_t *)disk_state.buf.etos.os_beg;
}


/*
 * Load an opened EmuTOS image file to its runtime address.
 */
static uint32_t
emutos_load(void)
{
    uint8_t *load_buffer = emutos_header_check();
    const emutos_hdr *eh = (emutos_hdr *)load_buffer;

    if (load_buffer == 0) {
        return 0;
    }
    printf("loading @ %p\n", load_buffer);
    file_reset();
    if (file_read(load_buffer, disk_state.f_size)) {
        return 0;
    }
    return eh->main;
}

/*
 * Try to boot EmuTOS from the currently-configured disk.
 */
static void
emutos_boot(void)
{
    uint32_t * const _memctrl = (uint32_t *)0x424;
    uint32_t * const _resvalid = (uint32_t *)0x426;
    uint32_t * const _phystop = (uint32_t *)0x42e;
    uint32_t * const _ramtop = (uint32_t *)0x5a4;
    uint32_t * const _memvalid = (uint32_t *)0x420;
    uint32_t * const _memval2 = (uint32_t *)0x43a;
    uint32_t * const _memval3 = (uint32_t *)0x51a;
    uint32_t * const _ramvalid = (uint32_t *)0x5a8;
    uint32_t * const _warm_magic = (uint32_t *)0x6fc;
    uint32_t entry;

    /* select the disk and try to open \emutosk5.rom */
    if (disk_select()) {
        printf("No disk\n");
    } else if (file_open("EMUTOSK5ROM")) {
        printf("No EmuTOS image\n");
    } else if ((entry = emutos_load())) {

        printf("booting @ 0x%lx\n\n", entry);
        *_memctrl = 0;              /* zero memctrl since we don't conform */
        *_resvalid = 0;             /* prevent reset vector being called */
        *_phystop = ram_size;       /* ST memory size */
        *_ramtop = 0;               /* no TT memory */
        *_memvalid = 0x752019f3;    /* set magic numbers to validate memory config */
        *_memval2 = 0x237698aa;
        *_memval3 = 0x5555aaaa;
        *_ramvalid = 0x1357bd13;
        *_warm_magic = 0;           /* this is a first/cold boot */

        video_init();
        jump_to_loaded_os(entry);
    }
}

static void
try_boot(uint32_t disk_base, uint8_t unit)
{
    disk_state.iobase = disk_base;
    disk_state.unit = unit;
    emutos_boot();
}

void
boot_main(uint32_t base_addr, uint32_t slave)
{
    printf("\nPT68K5 EmuTOS loader\n");

    /* simple RAM probing */
    for (ram_size = 0x00400000; ram_size <= 0x08000000; ram_size += 0x00100000) {
        printf("\rRAM size %uMiB", (unsigned int)(ram_size >> 20));
        if (!probe(ram_size)) {
            break;
        }
    }
    outc('\n');

    /* booted from floppy, try all possible HDs */
    if (base_addr == 0x100003f4) {
        printf("XTIDE master:\n");
        try_boot(xtide_base, 0);
        printf("XTIDE slave:\n");
        try_boot(xtide_base, 1);
        printf("IDE master:\n");
        try_boot(ide_base, 0);
        printf("IDE slave:\n");
        try_boot(ide_base, 1);
        printf("nothing bootable\n");
    } else {
        try_boot(base_addr, slave);
    }
}
