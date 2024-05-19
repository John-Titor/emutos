#!/bin/bash -e
#
# Prepare a disk image for use with EmuTOS on an emulator.
#

usage() {
    echo "error: $1"
    cat >/dev/stderr <<END

usage: $0 --file <image-file-name> [--size <size-megabytes>] [<file or directory>...]

Creates a disk image in the file <disk-image-file>, <size-megabytes> in size
(default 64M) partitioned as a single FAT16 filesystem. Any files or directories
listed on the commandline are copied into the filesystem.

END
    exit 1
}

copyfiles=()
diskimage=()
disksize=64
while [[ $# -gt 0 ]]; do
    case $1 in
        --file)
            diskimage=$2
            shift
            shift
            ;;
        --size)
            disksize=$2
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
dd if=/dev/zero of=${diskimage} bs=1m count=${disksize} status=none

#
# Partition the disk.
#
systype=`uname -s`
if [ $disksize -lt 32 ]; then
    usage "disk image must be at least 32M in size"
fi
if [ $disksize -gt 512 ]; then
    usage "disk image must be no more than 512M in size"
fi
partsects=$((${disksize} * 2048 - 63))
if [ "${systype}" == "Darwin" ]; then
    cat <<END | fdisk  -eyr -f /dev/zero ${diskimage} || true
63,${partsects},0x06,*,0,1,1,1023,254,63
0,0,0x00,-,0,0,0,0,0,0
0,0,0x00,-,0,0,0,0,0,0
0,0,0x00,-,0,0,0,0,0,0
END
elif [ "${systype}" == "Linux" ]; then
    cat << END | sfdisk ${diskimage}
63,${partsects},0x06,*
END
else
    echo "Don't know how to partition a disk image on ${systype}".
    exit 1
fi

#
# Make a FAT16 filesystem on the partition.
#
partition=${diskimage}@@32256
mformat -v QEMU -T ${partsects} -h 254 -s 63 -H 0 -i ${partition}

#
# Copy files if requested.
#
for f in ${copyfiles[*]}; do
    mcopy -vbsQ -i ${partition} $f ::
done

echo "Done"
