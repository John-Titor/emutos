#!/bin/bash -e
#
# Prepare a ROMdisk image.
#

usage() {
    echo "error: $1"
    cat >/dev/stderr << END

usage: $0 --machine <machine> --file <image-file-name> [<file or directory>...]"

END
    exit 1
}

fail() {
    echo "error: $1"
    exit 1
}

machine=()
copyfiles=()
diskimage=()

while [[ $# -gt 0 ]]; do
    case $1 in
        --machine)
            machine=$2
            shift
            shift
            ;;
        --file)
            diskimage=$2
            shift
            shift
            ;;
        --*)
            usage "unknown option $1"
            ;;
        *)
            copyfiles+=("$1")
            shift
            ;;
    esac
done

#
# Remove any existing disk image file and create a new one.
#
if [ -z ${diskimage} ]; then
    usage "Missing disk image filename."
fi
rm -f ${diskimage}

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
files_size=`du -cB 512 ${copyfiles[*]} | tail -1 | cut -f 1`
disk_nsectors=$((${files_size} + 28))

if [ ${disk_nsectors} -gt ${disk_max_sectors} ]; then
    fail "not enough ROM space for files (need ${disk_nsectors} have ${disk_max_sectors}"
fi
if [ ${disk_nsectors} -gt 4090 ]; then
    fail "cluster size > 512B not currently supported, adjust disk_nsectors calculation"
fi

#
# preemptively nuke .DS_Store turds from the source before copying
#
for f in ${copyfiles[*]}; do
    if [ -d $f ]; then
        find $f -name .DS_Store -delete
    fi
done

#
# create the image and copy files
#
rm -f ${diskimage}
export MTOOLS_NO_VFAT=1
mformat -i ${diskimage} -r 2 -C -T ${disk_nsectors}
for f in ${copyfiles[*]}; do
    mcopy -vbsQ -i ${diskimage} $f ::
done

#
# if required, convert the output format for upload.
#
case ${output_fmt} in
    srec)
        srec_temp=`mktemp ${diskimage}.XXXX`
        m68k-elf-objcopy -I binary -O srec --adjust-vma ${srec_address} --srec-forceS3 --srec-len 100 ${diskimage} ${srec_temp}
        mv ${srec_temp} ${diskimage}
        ;;
    binary)
        ;;
esac
