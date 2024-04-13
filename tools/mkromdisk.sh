#!/bin/bash
#
# Script to create a ROMdisk image
#

function usage_fail {
    echo "error: $1"
    echo "usage: mkromdisk <machine> <directory>"
    exit 1
}

function fail {
    echo "error: $1"
    exit 1
}

#
# parse arguments
#
if [ $# -ne 2 ]; then
    usage_fail "wrong number of arguments"
fi
machine=$1
source_dir="$2"
case ${machine} in
    ip940)
        # 512K of ROM for bootloader and EmuTOS, remainder for disk
        disk_max_sectors=3072

        # s-records for the flasher
        output_fmt=srec
        srec_address=0x00080000
        ;;
    *)
        usage_fail "${machine} not a recognized machine"
        ;;
esac
if [ ! -d "${source_dir}" ]; then
    usage_fail "${source_dir} is not a directory"
fi

#
# Size the disk image.
#
# Assuming FAT12:
#   1 boot sector
#  24 FAT sectors max (4096 entries * 12b * 2)
#   2 sectors for the root directory (32 entries max)
# ---
#  27 sectors overhead + 1 spare
#
# XXX assumes the disk is small enough that the cluster size
#     is 512B - need some sort of sanity check.
#
files_size=`du -sB 512 ${source_dir} | cut -f 1`
disk_nsectors=$((${files_size} + 28))

if [ ${disk_nsectors} -gt ${disk_max_sectors} ]; then
    fail "not enough ROM space for files (need ${disk_nsectors} have ${disk_max_sectors}"
fi

#
# create the image and copy files
#
# preemptively nuke .DS_Store turds from the source before copying
#
disk_file=romdisk.${machine}
rm -f ${disk_file}
export MTOOLS_NO_VFAT=1
mformat -i ${disk_file} -r 2 -C -T ${disk_nsectors}
find ${source_dir} -name .DS_Store -delete
mcopy -i ${disk_file} -bsvQ ${source_dir}/* ::/

#
# if required, convert the output format for upload
#
case ${output_fmt} in
    srec)
        srec_temp=`mktemp mkromdisk.XXXX`
        m68k-elf-objcopy -I binary -O srec --adjust-vma ${srec_address} --srec-forceS3 --srec-len 100 ${disk_file} ${srec_temp}
        mv ${srec_temp} ${disk_file}
        ;;
    binary)
        ;;
esac
