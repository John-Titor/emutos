EmuTOS - QEMU-AtariST version
=============================

This EmuTOS version supports QEMU with custom Atari ST-ish emulation.
See https://github.com/John-Titor/qemu/tree/local/atarist for QEMU sources
and binary releases for macOS.

Built Files
-----------
emutos-qemu.img             - EmuTOS system image

Supported System Configurations
-------------------------------

A custom QEMU build with Atari support is required. The following instructions
should build a working emulator for macOS and any reasonable Linux system:

# git clone https://github.com/John-Titor/qemu.git
...
# cd qemu
# git checkout local/atarist
# mkdir build
# cd build
# ../configure --target-list=m68k-softmmu
...
# make
...
# cp qemu-system-m68k ~/bin

To run:

# qemu-system-m68k -kernel emutos-qemu.img -m 64M \
        -drive file=disk.bin,if=ide,format=raw -display sdl \
        -machine type=atarist -serial mon:stdio -rtc base=localtime

This assumes that emutos-qemu.img and disk image file disk.bin are in the
current directory; adjust paths as necessary.

Note that the argument to -m (memory size) must be at least 15M.

Creating a Disk Image
---------------------

The easiest way to create a disk image is by imaging an existing disk. The
exact method will vary between systems, but the source disk should contain
an MBR and at least one type 6 partition, formatted FAT-16. TOS disk size
limits apply; keep your disk / partitions small, and always under 2GiB.

Port Details
------------
The port attempts to replicate Atari system functionality in a compatible
fashion. Well-written Atari programs will work as expected; games, demos and
other software that attempts to use the Atari hardware directly will not.

Compatibility Hints
- - - - - - - - - -
If EmuTOS crashes with a Bus Error, check the address being accessed.
If it is above 0xffff_0000, it's almost certainly some software trying to
touch Atari hardware directly. You can check the ST memory map here to help
work out what it's doing:

https://temlib.org/AtariForumWiki/index.php/Memory_Map_for_Atari_ST,STE,TT_and_Falcon

Video
- - -
QEMU-AtariST supports similar resolutions to SuperVidel, and the extended
XBIOS video functions Vsetmode, Vmontype, Vgetsize, Vsetrgb, Vgetrgb, and
Vfixmode.

By default EmuTOS boots at 1280x1024x8 which provides a reasonable balance
of screen space and performance on most modern host systems. No additional
video drivers are required; EmuTOS supports the QEMU-AtariST framebuffer
natively.

There is no built-in support for PCI video cards, although they can be
attached and drivers could be developed / ported to support them.

Mouse / Keyboard / RTC
- - - - - - - - - - - -
The emulator translates QEMU keyboard / mouse events so any host-supported
keyboard / mouse should work.

Time reported by the RTC can be controlled with the -rtc option to QEMU.

Emulated Hardware
- - - - - - - - -
- 68040 CPU
- 14M-2G RAM
- IKBD (keyboard/mouse) via ACIA including keyboard realtime clock
- MFP serial port, timers A, B, C
- Falcon-compatible IDE controller
- Bitmapped display
- PCI bus with PCI BIOS extensions, supports most QEMU PCI devices
- ISA bus, supports most QEMU ISA devices

Unsupported at this time:
- Sound
- SCSI
- Networking

Unlikely to be supported:
- Floppy disk

Port Files
- - - - - -
EmuTOS QEMU support is largely contained in bios/qemu*.

The bulk of the AtariST-specific code in QEMU can be found in
qemu/hw/m68k/atarist*.


This ROM image has been built with GCC 13.1.0 on macOS using:
    make ELF=1 qemu
