/*
 * qemu-system-m68k -machine type=atarist
 *
 * TODO:
 * - Disable auto-repeat; can it be done in QEMU? can we do it 'smart' in the keyboard driver?
 * - deskrez.c video mode selection
 *
 * BUGS:
 * - mouse button state is weird on entering window
 * - Without CONF_STRAM_SIZE, ST RAM sizes as zero
 * - Diamond Edge: "unsupported AES function #48" -> add CONF_WITH_3D_OBJECTS
 */

#include "emutos.h"
#include "qemu.h"
#include "bios.h"
#include "xbios.h"
#include "biosext.h"
#include "bdosbind.h"
#include "tosvars.h"
#include "lineavars.h"
#include "vt52.h"
#include "vectors.h"
#include "screen.h"
#include "string.h"
#include "cookie.h"

#ifdef MACHINE_QEMU

/* mono palette */
static const ULONG st_1_palette[2] = {
    0x00000000UL,
    0x00ffffffUL,
};

/* default 2-plane ST palette */
static const ULONG st_2_palette[4] = {
    0x00ffffffUL,
    0x00ff0000UL,
    0x0000ff00UL,
    0x00000000UL,
};

/* default 4-plane ST palette */
static const ULONG st_4_palette[16] = {
    0x00ffffffUL,
    0x00ff0000UL,
    0x0000ff00UL,
    0x00ffff00UL,
    0x000000ffUL,
    0x00ff00ffUL,
    0x0000ffffUL,
    0x00404040UL,
    0x007f7f7fUL,
    0x00ff7f7fUL,
    0x007fff7fUL,
    0x00ffff7fUL,
    0x007f7fffUL,
    0x00ff7fffUL,
    0x007fffffUL,
    0x00000000UL
};

/* default 8-plane TT palette */
static const ULONG tt_8_palette[256] = {
    0x00ffffffUL, 0x00ff0000UL, 0x0000ff00UL, 0x00ffff00UL,
    0x000000ffUL, 0x00ff00ffUL, 0x0000ffffUL, 0x00aaaaaaUL,
    0x00666666UL, 0x00ff9999UL, 0x0099ff99UL, 0x00ffff99UL,
    0x009999ffUL, 0x00ff99ffUL, 0x0099ffffUL, 0x00000000UL,
    0x00ffffffUL, 0x00eeeeeeUL, 0x00ddddddUL, 0x00ccccccUL,
    0x00bbbbbbUL, 0x00aaaaaaUL, 0x00999999UL, 0x00888888UL,
    0x00777777UL, 0x00666666UL, 0x00555555UL, 0x00444444UL,
    0x00333333UL, 0x00222222UL, 0x00111111UL, 0x00000000UL,
    0x00ff0000UL, 0x00ff0011UL, 0x00ff0022UL, 0x00ff0033UL,
    0x00ff0044UL, 0x00ff0055UL, 0x00ff0066UL, 0x00ff0077UL,
    0x00ff0088UL, 0x00ff0099UL, 0x00ff00aaUL, 0x00ff00bbUL,
    0x00ff00ccUL, 0x00ff00ddUL, 0x00ff00eeUL, 0x00ff00ffUL,
    0x00ee00ffUL, 0x00dd00ffUL, 0x00cc00ffUL, 0x00bb00ffUL,
    0x00aa00ffUL, 0x009900ffUL, 0x008800ffUL, 0x007700ffUL,
    0x006600ffUL, 0x005500ffUL, 0x004400ffUL, 0x003300ffUL,
    0x002200ffUL, 0x001100ffUL, 0x000000ffUL, 0x000011ffUL,
    0x000022ffUL, 0x000033ffUL, 0x000044ffUL, 0x000055ffUL,
    0x000066ffUL, 0x000077ffUL, 0x000088ffUL, 0x000099ffUL,
    0x0000aaffUL, 0x0000bbffUL, 0x0000ccffUL, 0x0000ddffUL,
    0x0000eeffUL, 0x0000ffffUL, 0x0000ffeeUL, 0x0000ffddUL,
    0x0000ffccUL, 0x0000ffbbUL, 0x0000ffaaUL, 0x0000ff99UL,
    0x0000ff88UL, 0x0000ff77UL, 0x0000ff66UL, 0x0000ff55UL,
    0x0000ff44UL, 0x0000ff33UL, 0x0000ff22UL, 0x0000ff11UL,
    0x0000ff00UL, 0x0011ff00UL, 0x0022ff00UL, 0x0033ff00UL,
    0x0044ff00UL, 0x0055ff00UL, 0x0066ff00UL, 0x0077ff00UL,
    0x0088ff00UL, 0x0099ff00UL, 0x00aaff00UL, 0x00bbff00UL,
    0x00ccff00UL, 0x00ddff00UL, 0x00eeff00UL, 0x00ffff00UL,
    0x00ffee00UL, 0x00ffdd00UL, 0x00ffcc00UL, 0x00ffbb00UL,
    0x00ffaa00UL, 0x00ff9900UL, 0x00ff8800UL, 0x00ff7700UL,
    0x00ff6600UL, 0x00ff5500UL, 0x00ff4400UL, 0x00ff3300UL,
    0x00ff2200UL, 0x00ff1100UL, 0x00bb0000UL, 0x00bb0011UL,
    0x00bb0022UL, 0x00bb0033UL, 0x00bb0044UL, 0x00bb0055UL,
    0x00bb0066UL, 0x00bb0077UL, 0x00bb0088UL, 0x00bb0099UL,
    0x00bb00aaUL, 0x00bb00bbUL, 0x00aa00bbUL, 0x009900bbUL,
    0x008800bbUL, 0x007700bbUL, 0x006600bbUL, 0x005500bbUL,
    0x004400bbUL, 0x003300bbUL, 0x002200bbUL, 0x001100bbUL,
    0x000000bbUL, 0x000011bbUL, 0x000022bbUL, 0x000033bbUL,
    0x000044bbUL, 0x000055bbUL, 0x000066bbUL, 0x000077bbUL,
    0x000088bbUL, 0x000099bbUL, 0x0000aabbUL, 0x0000bbbbUL,
    0x0000bbaaUL, 0x0000bb99UL, 0x0000bb88UL, 0x0000bb77UL,
    0x0000bb66UL, 0x0000bb55UL, 0x0000bb44UL, 0x0000bb33UL,
    0x0000bb22UL, 0x0000bb11UL, 0x0000bb00UL, 0x0011bb00UL,
    0x0022bb00UL, 0x0033bb00UL, 0x0044bb00UL, 0x0055bb00UL,
    0x0066bb00UL, 0x0077bb00UL, 0x0088bb00UL, 0x0099bb00UL,
    0x00aabb00UL, 0x00bbbb00UL, 0x00bbaa00UL, 0x00bb9900UL,
    0x00bb8800UL, 0x00bb7700UL, 0x00bb6600UL, 0x00bb5500UL,
    0x00bb4400UL, 0x00bb3300UL, 0x00bb2200UL, 0x00bb1100UL,
    0x00770000UL, 0x00770011UL, 0x00770022UL, 0x00770033UL,
    0x00770044UL, 0x00770055UL, 0x00770066UL, 0x00770077UL,
    0x00660077UL, 0x00550077UL, 0x00440077UL, 0x00330077UL,
    0x00220077UL, 0x00110077UL, 0x00000077UL, 0x00001177UL,
    0x00002277UL, 0x00003377UL, 0x00004477UL, 0x00005577UL,
    0x00006677UL, 0x00007777UL, 0x00007766UL, 0x00007755UL,
    0x00007744UL, 0x00007733UL, 0x00007722UL, 0x00007711UL,
    0x00007700UL, 0x00117700UL, 0x00227700UL, 0x00337700UL,
    0x00447700UL, 0x00557700UL, 0x00667700UL, 0x00777700UL,
    0x00776600UL, 0x00775500UL, 0x00774400UL, 0x00773300UL,
    0x00772200UL, 0x00771100UL, 0x00440000UL, 0x00440011UL,
    0x00440022UL, 0x00440033UL, 0x00440044UL, 0x00330044UL,
    0x00220044UL, 0x00110044UL, 0x00000044UL, 0x00001144UL,
    0x00002244UL, 0x00003344UL, 0x00004444UL, 0x00004433UL,
    0x00004422UL, 0x00004411UL, 0x00004400UL, 0x00114400UL,
    0x00224400UL, 0x00334400UL, 0x00444400UL, 0x00443300UL,
    0x00442200UL, 0x00441100UL, 0x00ffffffUL, 0x00000000UL,
};

#define FRAMEBUFFER_REG_BASE    0xffffc000UL
#define FRAMEBUFFER_PAL_BASE    0xffffc400UL

#define FB_REG(_x)     *((volatile ULONG *)FRAMEBUFFER_REG_BASE + _x)
#define FB_VBL_ACK      FB_REG(0)
#define FB_VBL_PERIOD   FB_REG(1)
#define FB_DEPTH        FB_REG(2)
#define FB_WIDTH        FB_REG(3)
#define FB_HEIGHT       FB_REG(4)
#define FB_VADDR        FB_REG(5)

#define FB_PALETTE(_x)  *((volatile ULONG *)FRAMEBUFFER_PAL_BASE + _x)

static void load_palette(const ULONG *palette, UWORD depth)
{
    UWORD count = (1 << depth);
    UWORD i;

    for (i = 0; i < count; i++) {
        FB_PALETTE(i) = palette[i];
    }
}

static WORD current_video_mode;

#define SVEXT               0x4000                  /* Super Videl extended mode flag */
#define SVEXT_BASERES(res)  ((res & 0xf) << 9)

struct video_mode {
    UWORD mode;
    UWORD maxdepth;
    LONG width;
    LONG height;
};

static const struct video_mode video_mode_table [] = {
    {                VIDEL_VGA,                  32,  320,  480 },
    {                VIDEL_VGA | VIDEL_80COL,    32,  640,  480 },
    { VIDEL_COMPAT | VIDEL_VGA,                  32,  320,  400 },
    { VIDEL_COMPAT | VIDEL_VGA | VIDEL_80COL,    32,  640,  400 },
    { SVEXT | SVEXT_BASERES(0),                  32,  640,  480 },
    { SVEXT | SVEXT_BASERES(1),                  32,  800,  600 },
    { SVEXT | SVEXT_BASERES(2),                  32, 1024,  768 },
    { SVEXT | SVEXT_BASERES(3),                  16, 1280, 1024 },
    { SVEXT | SVEXT_BASERES(4),                  16, 1600, 1200 },
    { SVEXT | SVEXT_BASERES(0) | VIDEL_OVERSCAN, 32, 1280,  720 },
    { SVEXT | SVEXT_BASERES(1) | VIDEL_OVERSCAN, 16, 1680, 1050 },
    { SVEXT | SVEXT_BASERES(2) | VIDEL_OVERSCAN,  8, 1920, 1080 },
    { SVEXT | SVEXT_BASERES(3) | VIDEL_OVERSCAN,  8, 1920, 1200 },
    { SVEXT | SVEXT_BASERES(4) | VIDEL_OVERSCAN,  8, 2560, 1440 },
    { 0 }
};

#define MODE_MASK (SVEXT | SVEXT_BASERES(0xf) | VIDEL_COMPAT | VIDEL_OVERSCAN | VIDEL_VGA | VIDEL_80COL)

static const struct video_mode *find_mode(WORD mode)
{
    WORD i;

    for (i = 0; video_mode_table[i].mode != 0; i++) {
        if ((mode & MODE_MASK) == video_mode_table[i].mode) {
            return &video_mode_table[i];
        }
    }
    return NULL;
}

static LONG mode_depth(WORD mode)
{
    const struct video_mode *vm = find_mode(mode);
    LONG depth;

    if (vm == NULL) {
        return -1;
    }

    depth = (1 << (mode & 0x7));
    if (depth > vm->maxdepth) {
        return -1;
    }
    return depth;
}

static LONG mode_width(WORD mode)
{
    const struct video_mode *vm = find_mode(mode);

    if (vm) {
        return vm->width;
    }
    return -1;
}

static LONG mode_height(WORD mode)
{
    const struct video_mode *vm = find_mode(mode);

    if (vm != NULL) {
        return vm->height;
    }
    return -1;
}

static WORD set_video_mode(WORD mode)
{
    ULONG depth;
    ULONG width;
    ULONG height;

    /* get mode geometry */
    depth = mode_depth(mode);
    width = mode_width(mode);
    height = mode_height(mode);

    /* valid mode? */
    if ((depth == -1) || (width == -1) || (height == -1)) {
        return -1;
    }

    /* reconfigure display */
    FB_DEPTH = depth;
    FB_WIDTH = width;
    FB_HEIGHT = height;

    /* load default palette for indexed modes */
    switch (FB_DEPTH) {
    case 1:
        load_palette(st_1_palette, 1);
        break;
    case 2:
        load_palette(st_2_palette, 2);
        break;
    case 4:
        load_palette(st_4_palette, 4);
        break;
    case 8:
        load_palette(tt_8_palette, 8);
        break;
    }

    /* set sshiftmod */
    if (mode & VIDEL_COMPAT) {
        if (depth == 1) {
            sshiftmod = ST_HIGH;
        } else {
            if (width == 640) {
                sshiftmod = ST_MEDIUM;
            } else {
                sshiftmod = ST_LOW;
            }
        }
    } else {
        sshiftmod = FALCON_REZ;
    }
    return 0;
}

WORD qemu_vsetmode(WORD mode)
{
    WORD ret = current_video_mode;
    ULONG saved_vaddr = FB_VADDR;

    /* enquiry? */
    if (mode == -1) {
        return current_video_mode;
    }

    /* set mode */
    mode = qemu_vfixmode(mode);
    if (set_video_mode(mode) < 0) {
        return ret;
    }

    /* reconfigure display */
    FB_VADDR = saved_vaddr;

    /* check for success */
    if (FB_VADDR == 0) {
        /* failure; restore previous settings */
        set_video_mode(current_video_mode);
        FB_VADDR = saved_vaddr;
        return current_video_mode;
    }

    current_video_mode = mode;
    return ret;
}

WORD qemu_vmontype(void)
{
    return MON_VGA;
}

WORD qemu_vsetsync(WORD external)
{
    return 0;
}

LONG qemu_vgetsize(WORD mode)
{
    return mode_width(current_video_mode) * mode_depth(current_video_mode) / 8;
}

WORD qemu_vsetrgb(WORD index, WORD count, const ULONG *rgb)
{
    while ((index < 0x100) && (count-- > 0)) {
        FB_PALETTE(index++) = *rgb++;
    }
    return 0;
}

WORD qemu_vgetrgb(WORD index, WORD count, ULONG *rgb)
{
    while ((index < 0x100) && (count-- > 0)) {
        *rgb++ = FB_PALETTE(index++);
    }
    return 0;
}

WORD qemu_vfixmode(WORD mode)
{
    /* as per the VGA case from the Videl code */
    mode &= ~VIDEL_PAL;

    if (!(mode & VIDEL_VGA))            /* if mode doesn't have VGA set, */
        mode ^= (VIDEL_VERTICAL | VIDEL_VGA);   /* set it & flip vertical */
    if (mode & VIDEL_COMPAT) {
        if ((mode&VIDEL_BPPMASK) == VIDEL_1BPP)
            mode &= ~VIDEL_VERTICAL;    /* clear vertical for ST high */
        else mode |= VIDEL_VERTICAL;    /* set it for ST medium, low  */
    }
    return mode;
}

/*
 * setscreen(): implement the Setscreen() xbios call
 *
 * implementation summary:
 *  . sets the logical screen address from logLoc, iff logLoc > 0
 *  . sets the physical screen address from physLoc, iff physLoc > 0
 *  . if logLoc==0 and physLoc==0:
 *  . sets the screen resolution iff rez == 7 && mode != -1
 *  . reinitialises lineA and the VT52 console
 */
WORD setscreen(UBYTE *logLoc, const UBYTE *physLoc, WORD rez, WORD mode)
{
    WORD oldmode = 0;

    if ((LONG)logLoc > 0) {
        v_bas_ad = logLoc;
        KDEBUG(("v_bas_ad = %p\n", v_bas_ad));
    }
    if ((LONG)physLoc > 0) {
        setphys(physLoc);
    }

    /* only extended resolution changes allowed */
    if ((rez != FALCON_REZ) || (mode == -1)) {
        return 0;
    }

    mode = qemu_vfixmode(mode);
    if (!logLoc && !physLoc) {
        UBYTE *addr = (UBYTE *)Srealloc(qemu_vgetsize(mode));
        if (!addr)      /* Srealloc() failed */
            return -1;
        KDEBUG(("screen realloc'd to %p\n", addr));
        v_bas_ad = addr;
        setphys(addr);
    }
    oldmode = qemu_vsetmode(-1);

    /* Temporarily halt VBL processing */
    vblsem = 0;
    /* Re-initialize line-a, VT52 etc: */
    linea_init();
    if (v_planes < 16)
        vt52_init();
    /* Restart VBL processing */
    vblsem = 1;

    return oldmode;
}

#define MODE_1280x1024x1   (SVEXT | SVEXT_BASERES(3) | VIDEL_1BPP)
#define MODE_1280x1024x2   (SVEXT | SVEXT_BASERES(3) | VIDEL_2BPP)
#define MODE_1280x1024x4   (SVEXT | SVEXT_BASERES(3) | VIDEL_4BPP)
#define MODE_1280x1024x8   (SVEXT | SVEXT_BASERES(3) | VIDEL_8BPP)

void screen_init_mode(void)
{
    /* configure video mode */
    current_video_mode = MODE_1280x1024x8;
    if (set_video_mode(current_video_mode)) {
        panic("qemu: failed to set initial video mode\n");
    }

    /*
     * Configure the VBL interrupt.
     */
    VEC_LEVEL3 = qemu_vbl_shim;
    VEC_VBL = qemu_vbl;
    FB_VBL_PERIOD = 16666666;   /* XXX check OS header */
    if (FB_VBL_PERIOD == 0) {
        panic("failed to set VBL period\n");
    }
}

ULONG calc_vram_size(void)
{
    /* max video mode supported */
    return 2560UL * 1440 + EXTRA_VRAM_SIZE;
}

void screen_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez)
{
    *planes = mode_depth(current_video_mode);
    *hz_rez = mode_width(current_video_mode);
    *vt_rez = mode_height(current_video_mode);
    KDEBUG(("fb: depth %u, width %u height %u\n", *planes, *hz_rez, *vt_rez));
}

void get_pixel_size(WORD *width,WORD *height)
{
    *width = (V_REZ_HZ < 640) ? 556 : 278;  /* magic numbers as used */
    *height = (V_REZ_VT < 400) ? 556 : 278; /*  by TOS 3 & TOS 4     */
}

WORD setcolor(WORD colorNum, WORD color)
{
    ULONG rgb;
    WORD prev;

    /* palette 00RRGGBB */
    /* color       0RGB */
    colorNum &= 0xf;
    rgb = FB_PALETTE(colorNum);
    prev = ((rgb >> 12) & 0xf00) | ((rgb >> 8) & 0x0f0) | ((rgb >> 4) & 0x00f);

    if (color != -1) {
#if 1
        ULONG r = (color >> 7) & 0xf;
        ULONG g = (color >> 3) & 0xf;
        ULONG b = (color << 1) & 0xf;
#else
        /* if it thinks we are a Falcon / TT / STe */
        ULONG r = (color >> 8) & 0xf;
        ULONG g = (color >> 4) & 0xf;
        ULONG b = (color << 0) & 0xf;
#endif

        rgb = (r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | b;
        FB_PALETTE(colorNum) = rgb;
    }
    return prev;
}

void setpalette(const UWORD *palettePtr)
{
    UWORD i;

    for (i = 0; i < 16; i++) {
        setcolor(i, *(palettePtr + i));
    }
}

WORD get_monitor_type(void)
{
    return qemu_vmontype();
}

int rez_changeable(void)
{
    return TRUE;
}

WORD get_palette(void)
{
    /*
    if (has_videl)
    {
        WORD mode = vsetmode(-1);
        if ((mode&VIDEL_COMPAT)
         || ((mode&VIDEL_BPPMASK) == VIDEL_4BPP))
            return 4096;
        return 0;
    }
    */
    return 0;               /* same as Falcon in extreme cases */
}

const UBYTE *physbase(void)
{
    return (const UBYTE *)FB_VADDR;
}

void setphys(const UBYTE *addr)
{
    /*
     * Select the framebuffer and enable video.
     */
    FB_VADDR = (ULONG)addr;
    if (FB_VADDR == 0) {
        panic("failed to set video mode\n");
    }
}

void qemu_video_add_cookies(void)
{
    /* we have the Falcon XBIOS API, so claim to be one */
    cookie_add(COOKIE_VDO, VDO_FALCON);
}

#endif /* MACHINE_QEMU */
