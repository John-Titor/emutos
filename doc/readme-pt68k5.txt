EmuTOS - PT68K5 version
=======================

This EmuTOS version supports the the Peripheral Technology/CDS PT68K5 
68020-based motherboard.

Built Files
-----------
EMUTOSK5.SYS                - EmuTOS system image
pt68k5boot.img              - Booter
pt68k5installboot           - Booter installer
extras/pt68k5_gotek_img.cfg - Gotek/FlashFloppy config for floppy booting

The booter supports both the onboard IDE controller and the PTI XTIDE card.
If booted from either of these interfaces, it will expect to find EmuTOS on
the same drive. If booted from floppy, it will search both controllers looking
for a bootable partition.

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

Floppy Boot
- - - - - -
If you are unable to install the booter to the hard drive / SDCard, you can
load it from a Gotek/FlashFloppy. The OS image must still be on one of the
drives / SDCards in the system.

Create a 1.44M floppy drive image (must be exactly 1392640 bytes) and install
the booter to the image:

# dd if=/dev/zero of=EMUTOS.REX.IMA count=1392640
# ./pt68k5_installboot pt68k5boot.img EMUTOS.REX.IMA

Then copy EMUTOS.REX.IMA and extras/pt68k5_gotek_img.cfg to a USB drive and
insert into your Gotek/FlashFloppy. Boot using 'U' at the MONK5 prompt.

Port Details
------------
The port attempts to replicate Atari system functionality in a compatible
fashion. Well-written Atari programs will work as expected; games, demos and
other software that attempts to use the Atari hardware directly will not.

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

Internal IDE vs. XTIDE
- - - - - - - - - - -
The default configuration assumes the use of an SDCard (for ease of
development) on the PTI-supplied XTIDE card. Build with CONF_WITH_XTIDE=0 to
select the internal IDE interface instead. Quirks in the EmuTOS IDE
implementation make supporting both at the same time inconvenient, although
this is planned.

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
supported without complicated hacks.

The current VGA mode 12h implementation could be made to work with any VGA
card, although it would require MONK5 changes as its implementation is
specific to the ET4000.

Supported PT68K5 hardware
- - - - - - - - - - - - -

- 68020/68881 CPU/FPU (25 MHz)
- 4-128M RAM
- Hardware realtime clock
- Onboard IDE controller
- PTI XTIDE controller
- Serial console
- Mouse Systems / Logitech 3-button mouse
- Keyboard
- ET4000 video

Planned support:
- Microsoft-compatible serial mouse (implemented, untested)
- Simultaneous internal and XTIDE drives
- Other IDE cards

Unlikely to be supported:
- Floppy disk
- Sound
- SCSI

This ROM image has been built with GCC 13.1.0 using:
make ELF=1 pt68k5

