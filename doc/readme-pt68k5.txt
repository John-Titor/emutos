EmuTOS - PT68K5 version
=======================

This EmuTOS version supports the the Peripheral Technology/CDS PT68K5 
68020-based motherboard: https://peripheraltech.com/PT68K5.htm

Built Files
-----------
EMUTOSK5.SYS                - EmuTOS system image
pt68k5boot.img              - Booter
pt68k5installboot           - Booter installer

The booter supports both the onboard IDE controller and the PTI XTIDE card.
If booted from either of these interfaces, it will expect to find EmuTOS on
the same drive.

Supported System Configurations
-------------------------------

Three system configurations are supported; full GUI, VGA text mode, and serial
console mode. See the EmuTOS documentation for build instructions.

Full GUI Config
- - - - - - - -
Requires an ET4000 card supported by MONK5, an XT or supported AT keyboard +
adapter, and a supported serial mouse.

Connect the mouse to COM2. COM1 is not used. COM3/4 are available as Atari
serial ports.

This is the default configuration, no changes are required to build.

VGA Text Config
- - - - - - - -
Requires an ET4000 card supported by MONK5, an XT or supported AT keyboard +
adapter.

COM1/COM2 are not used. COM3/4 are available as Atari serial ports.

To build for this configuration, edit the Makefile and adjust the AES setting
for the pt68k5 target to 0.

Serial Console Config
- - - - - - - - - - -
Requires a serial terminal with ANSI emulation. Connect the terminal to COM1
as normal for MONK5. COM2 is available as an Atari serial port. COM3/4 are
not used.

To build for this configuration, edit the Makefile and adjust the AES setting
for the pt68k5 target to 0. Edit include/config.h and in the PT68K5 machine
config set CONF_SERIAL_CONSOLE to 1.

Installing to HD / SDCard
-------------------------
Partition a drive / SDCard and create a bootable FAT16 partition. Copy
EMUTOSK5.SYS to the root directory of the partition. 

Making the drive bootable requires patching the first 32k of the drive; how
you do this will depend on the system you are using. For example, on macOS,
assuming the disk device for the drive / card is /dev/disk4:

# diskutil unmountDisk disk4
# sudo dd if=/dev/disk4 of=/tmp/hd.bin count=64
# ./pt68k5_installboot pt68k5boot.img /tmp/hd.img
# sudo dd if=/tmp/hd.bin of=/dev/disk4
# diskutil eject disk4

Other unixlike systems will be similar, although the specifics of device names
and unmounting filesystems may vary.

Connect the drive to your PT68K5 and use 'U' or 'W' at the MONK5 prompt to boot
from the XTIDE or internal primary drives respectively.

Port Details
------------
The port attempts to replicate Atari system functionality in a compatible
fashion. Well-written Atari programs will work as expected; games, demos and
other software that attempts to use the Atari hardware directly will not.

Compatibility Hints
- - - - - - - - - -
If EmuTOS crashes with a Bus Error, check the address that's being accessed.
If it is above 0xffff_0000, it's almost certainly some software trying to
touch Atari hardware directly. You can check the ST memory map here to help
work out what it's doing: 

https://temlib.org/AtariForumWiki/index.php/Memory_Map_for_Atari_ST,STE,TT_and_Falcon

Some typical offenders are NVDI 5.0, which tries to touch the ST Palette
register at 0xffff_8240, and GemBench which attempts to access the IKBD
data register at 0xffff_fc02.

Serial Console vs. Keyboard
- - - - - - - - - - - - - -
If using a serial console, connect to COM1 at 9600bps as normal. COM2 can be
configured to provide KDEBUG output, or used as a normal serial port. COM3 and
COM4 are not supported in this configuration.

If using a PC keyboard and video card, COM1 is not available and COM2 is
reserved for a standard PC serial mouse. COM3 and COM4 are available as normal
serial ports in this configuration.

Note that only the standard US PC / Atari keyboard mapping is supported. Other
keymaps would be possible.

The AT2XT keyboard converter used with the PT68K5 causes problems with the
cursor / home / end keys on some AT keyboards. For best results ensure NumLock
is off; it may be necessary to only use the cursor keys in the numeric keypad.

Onboard IDE vs. XTIDE
- - - - - - - - - - -
Drive letters are assigned in order:

 - XTIDE primary
 - XTIDE secondary
 - Onboard IDE primary
 - Onboard IDE secondary

This may mean that the drive that EMUTOSK5.ROM was loaded from is not C:.

Mice
- -
Only three-button serial mice using the Mouse Systems protocol are currently
known to work. Experimental support for Microsoft-protocol mice is in the
system but has not been tested.

Video
- - -
Currently only 640x480 monochrome mode is supported, due to quirks of the
VGA architecture. EmuTOS expects a linear framebuffer, but the ET4000 like
most ISA VGA cards uses a banked memory access scheme. Only video modes with
a packed or single-plan layout that fit entirely in the 64k window can be
supported without complicated hacks or a custom VDI.

The current VGA mode 12h implementation could be made to work with any VGA
card, although it would require MONK5 changes as its implementation is
specific to the ET4000.

Supported PT68K5 hardware
- - - - - - - - - - - - -

- 68020/68881 CPU/FPU
- 16-128M RAM
- Hardware realtime clock
- Onboard IDE controller
- PTI XTIDE controller
- Serial console
- Mouse Systems / Logitech 3-button serial mouse
- Microsoft-compatible 2-buttin serial mouse
- AT/PS2 Keyboard via PTI AT2XT adapter
- ET4000 video

Planned support:
- Other IDE cards

Unlikely to be supported:
- Floppy disk
- Sound
- SCSI

This ROM image has been built with GCC 13.1.0 on macOS using:
    make ELF=1 pt68k5
