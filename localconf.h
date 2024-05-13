/*
 * Defaults for the PT68K5 target
 */
#ifdef TARGET_PT68K5
# define MACHINE_PT68K5 1
#endif

/*
 * Defaults for the PT68K5 machine
 */
#ifdef MACHINE_PT68K5
# define CONF_ATARI_HARDWARE 0
# undef EMUTOS_LIVES_IN_RAM
# define EMUTOS_LIVES_IN_RAM 1
# define CONF_WITH_ALT_RAM 0
# define CONF_WITH_ADVANCED_CPU 1
# define CONF_WITH_DUART 1
# define CONF_WITH_DUART_CHANNEL_B 1
# define CONF_DUART_TIMER_C 1
# define CONF_WITH_IDE 1
# define CONF_IDE_NO_RESET 1
# define CONF_WITH_PRINTER_ICON 0
# ifndef CONF_SERIAL_CONSOLE
#  define CONF_SERIAL_CONSOLE 0
# endif
# if CONF_SERIAL_CONSOLE
#  ifndef CONF_SERIAL_CONSOLE_ANSI
#   define CONF_SERIAL_CONSOLE_ANSI 1
#  endif
#  ifndef CONF_SERIAL_CONSOLE_POLLING_MODE
#   define CONF_SERIAL_CONSOLE_POLLING_MODE 0
#  endif
#  ifndef DEFAULT_BAUDRATE
#   define DEFAULT_BAUDRATE B9600
#  endif
   /* console on COM1 like MONK5, COM2 available for KDEBUG */
#  define DUART_BASE 0x20004000UL
#  define CONF_DUART_AUTOVECTOR 5
#  define CONF_PT68K5_KBD_MOUSE 0
# else
   /* COM3/COM4 due to keyboard interrupt on primary DUART */
#  define DUART_BASE 0x20004040UL
#  define CONF_DUART_AUTOVECTOR 4
#  define CONF_PT68K5_KBD_MOUSE 1
#  define CONF_VRAM_ADDRESS 0x080a0000
# endif
# ifndef ALWAYS_SHOW_INITINFO
#  define ALWAYS_SHOW_INITINFO 1
# endif
# ifndef CONF_WITH_BUS_ERROR
#  define CONF_WITH_BUS_ERROR 1
# endif
# ifndef USE_STOP_INSN_TO_FREE_HOST_CPU
#  define USE_STOP_INSN_TO_FREE_HOST_CPU 0
# endif
# ifndef ENABLE_KDEBUG
#  define ENABLE_KDEBUG 0
# endif
# if ENABLE_KDEBUG
#  define DUART_DEBUG_PRINT 1
#  define RS232_DEBUG_PRINT 0
# endif
#endif

/*
 * Defaults for the IP940 (ROM) target.
 */
#ifdef TARGET_IP940
# define MACHINE_IP940
#endif

/*
 * Defaults for the IP940 machine.
 */
#ifdef MACHINE_IP940
# define CONF_ATARI_HARDWARE 0
# define CONF_WITH_ALT_RAM 0
# define CONF_WITH_ADVANCED_CPU 1
# define CONF_WITH_CACHE_CONTROL 0
   /* PMMU already in use mapping RAM / ROM */
# define CONF_WITH_68040_PMMU 0
# define CONF_WITH_CONFIGURED_PMMU 1
   /* does nothing on this hardware */
# define CONF_WITH_RESET 0
# define CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF 1
# define CONF_WITH_IDE 1
# define CONF_WITH_ROMDISK 1
# define CONF_WITH_BCONMAP 1
# define CONF_WITH_MACHINE_COOKIES 1
# define CONF_IDE_NO_RESET 1
# define CONF_WITH_APOLLO_68080 0
# define CONF_WITH_PRINTER_ICON 0
# define CONF_WITH_KPRINTF 1
# define CONF_SERIAL_CONSOLE 1
# if CONF_SERIAL_CONSOLE
#  ifndef CONF_SERIAL_CONSOLE_ANSI
#   define CONF_SERIAL_CONSOLE_ANSI 1
#  endif
#  ifndef DEFAULT_BAUDRATE
#   define DEFAULT_BAUDRATE B115200
#  endif
# endif
# ifndef ALWAYS_SHOW_INITINFO
#  define ALWAYS_SHOW_INITINFO 1
# endif
# ifndef CONF_WITH_BUS_ERROR
#  define CONF_WITH_BUS_ERROR 1
# endif
# ifndef USE_STOP_INSN_TO_FREE_HOST_CPU
#  define USE_STOP_INSN_TO_FREE_HOST_CPU 1
# endif
# ifndef ENABLE_KDEBUG
#  define ENABLE_KDEBUG 0
# endif
# if ENABLE_KDEBUG
#  define RS232_DEBUG_PRINT 0
# endif
#endif

/*
 * Defaults for the q800 machine
 */
#ifdef MACHINE_Q800
# define CONF_ATARI_HARDWARE 0
# define CONF_WITH_SCSI 1
# define CONF_WITH_ADVANCED_CPU 1
# define CONF_WITH_CACHE_CONTROL 1
# define CONF_WITH_RESET 0
# define CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF 1
# define CONF_WITH_APOLLO_68060 0
# define CONF_WTIH_PRINTER_ICON 0
# define CONF_WITH_KPRINTF 1
# define CONF_SERIAL_CONSOLE 1
# if CONF_SERIAL_CONSOLE
#  ifndef CONF_SERIAL_CONSOLE_ANSI
#   define CONF_SERIAL_CONSOLE_ANSI 1
#  endif
#  ifndef CONF_SERIAL_CONSOLE_POLLING_MODE
#   define CONF_SERIAL_CONSOLE_POLLING_MODE 0
#  endif
#  ifndef DEFAULT_BAUDRATE
#   define DEFAULT_BAUDRATE B115200
#  endif
# endif
# ifndef ALWAYS_SHOW_INITINFO
#  define ALWAYS_SHOW_INITINFO 1
# endif
# ifndef CONF_WITH_BUS_ERROR
#  define CONF_WITH_BUS_ERROR 1
# endif
# ifndef USE_STOP_INSN_TO_FREE_HOST_CPU
#  define USE_STOP_INSN_TO_FREE_HOST_CPU 1
# endif
# ifndef ENABLE_KDEBUG
#  define ENABLE_KDEBUG 0
# endif
# if ENABLE_KDEBUG
#  define RS232_DEBUG_PRINT 0
# endif
#endif

/*
 * Defaults for the QEMU machine
 */
#ifdef MACHINE_QEMU
# define CONF_ATARI_HARDWARE 0
# define CONF_WITH_IDE 1
# define CONF_WITH_IDE_BYTESWAP 1
# define CONF_WITH_MFP 1
# define CONF_WITH_IKBD_ACIA 1
# define CONF_WITH_IKBD_CLOCK 1
# define CONF_WITH_ADVANCED_CPU 1
# define CONF_FORCE_CPU_TYPE 40
# define CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF 1
# define CONF_WITH_ALT_RAM 1
# define CONF_WITH_TTRAM 1
# define CONF_WITH_BUS_ERROR 1
# define CONF_WITH_SHUTDOWN 1
# define CONF_WITH_CUSTOM_XBIOS_VIDEO 1
# define CONF_WITH_EXTENDED_PALETTE 1
# define CONF_WITH_XBIOS_EXTENSION 1
# define CONF_WITH_MACHINE_COOKIES 1
# define CONF_WITH_3D_OBJECTS 1
# define CONF_WITH_COLOUR_ICONS 1
# define CONF_WITH_EXTENDED_OBJECTS 1
# define CONF_WITH_GRAF_MOUSE_EXTENSION 1
# define CONF_WITH_MENU_EXTENSION 1
# define CONF_WITH_NICELINES 1
# define CONF_WITH_WINDOW_COLOURS 1
# define CONF_WITH_BIOS_EXTENSIONS 1
# define CONF_WITH_EXTENDED_MOUSE 1
# define CONF_WITH_FORMAT 0
# define CONF_WITH_PRINTER_ICON 0
/*# define TOS_VERSION 0x404*/
# ifndef CONF_STRAM_SIZE
#  define CONF_STRAM_SIZE 14*1024*1024
# endif
# ifndef CONF_SERIAL_CONSOLE
#  define CONF_SERIAL_CONSOLE 0
# endif
# ifndef ENABLE_KDEBUG
#  define ENABLE_KDEBUG 0
# endif
# ifndef QEMU_DEBUG_PRINT
#  define QEMU_DEBUG_PRINT 0
# endif
#endif

