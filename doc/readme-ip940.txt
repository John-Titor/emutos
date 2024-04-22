EmuTOS - IP940 version
======================

This EmuTOS version supports the the Perceptics IP940 CPU mezzanine card
on a custom baseboard. See https://github.com/John-Titor/IP940 for more
details on the IP940, and https://www.retrobrewcomputers.org/doku.php?id=builderpages:plasmo:68040:ip940:ip940base
for the baseboard.

Built Files
-----------
emutos940.s19               - EmuTOS ROM image ready for upload

The image is flashed to ROM on the IP940 card; see installation instructions
below.

Supported System Configurations
-------------------------------
Only serial console mode is supported, with the IP940 mounted on the v0
prototype baseboard. An ANSI-compatible serial terminal is required.

The EmuTOS console uses the same serial port as the bootloader, and runs
at 115200bps. The other serial ports are free for application use.

8M IP940 Note
- - - - - - -

The image builds by default for a 12M system. To build for an 8M system
edit util/ip940_loader.S and adjust the value of phystop.

Installing
----------

Upload
- - - -

Uploading is supported on boards equipped with flash ROM. Boot the flasher,
and upload emutos940.s19. Optionally upload a ROMdisk generated using
tools/mkromdisk.sh.

Once flashed, reset and the system will boot EmuTOS to the built-in shell.
If a CF card is attached and partitioned, partitions will show up as drives
C, D, etc. If a ROMdisk was flashed, it will show up as the next drive
letter after the CF card.

Flash
- - -

Flash the IP940 bootrom, emutos940.s19 and optionally a ROMdisk generated
using tools/mkromdisk.sh to the IP940 ROMs. Once insatlled, the system will
boot EmuTOS to the built-in shell. If a CF card is attached and partitioned,
partitions will show up as drives C, D, etc. If a ROMdisk was flashed, it will
show up as the next drive letter after the CF card.

Port Details
------------
The port attempts to replicate Atari system functionality in a compatible
fashion. Well-written text-mode Atari programs will work as expected; games,
demos and other software that attempts to use the Atari hardware directly will
not.

Due to the memory layout of the IP940, the system runs with the MMU enabled
at all times. The MMU tables are in ROM, so there is no risk of accidental
corruption. The caches are enabled, and the data cache runs in copyback mode
for maximum performance. This may cause problems with poorly-written self-
modifying code.

ROMdisk
- - - -
The first 512K of ROM is reserved for the bootrom and EmuTOS. A FAT12
filesystem of up to 1.5M total size can be flashed starting at 0x00800000,
and this will be mounted after any CF filesystems.

The script in tools/mkromdisk.sh can be used on macOS / Linux systems to
create such a filesystem. The script customises the filesystem parameters to
minimise wasted space.

It's recommended to use an executable packer such as UPX in order to make
the most of the ROMdisk capacity.

When uploading a new ROMdisk to the IP940 flasher it is not necessary to
re-flash EmuTOS, and vice versa.

Compatibility Hints
- - - - - - - - - -
If EmuTOS crashes with a Bus Error, check the address that's being accessed.
If it is above 0xffff_0000, it's almost certainly some software trying to
touch Atari hardware directly. You can check the ST memory map here to help
work out what it's doing:

https://temlib.org/AtariForumWiki/index.php/Memory_Map_for_Atari_ST,STE,TT_and_Falcon


This ROM image has been built with GCC 13.1.0 on macOS using:
    make ELF=1 ip940
