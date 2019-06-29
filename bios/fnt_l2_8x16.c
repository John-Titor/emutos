/*
 * fnt_l2_8x16.c - a font in standard format
 *
 * Copyright (C) 2002-2019 The EmuTOS development team
 *
 * Automatically generated by fntconv.c
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "config.h"
#include "portab.h"
#include "bios.h"
#include "fonthdr.h"

static const UWORD dat_table[] =
{
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1104,
    0x0000, 0x0000, 0x1800, 0x3800, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x4000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x6600, 0x0000, 0x0c00, 0x6636, 0x0036, 0x0c00, 0x3618,
    0x0000, 0x0000, 0x0600, 0x0036, 0x0000, 0x0000, 0x001b, 0x0000,
    0x180c, 0x1866, 0x0000, 0x0c00, 0x6c06, 0x0066, 0x3606, 0x186c,
    0x000c, 0x3606, 0x181b, 0x6600, 0x6c1c, 0x0c33, 0x6606, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0018,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x05a0,
    0x7c00, 0x7c7c, 0x007c, 0x7c7c, 0x7c7c, 0x0000, 0x0000, 0x0b28,
    0x0000, 0x0000, 0x1800, 0x7c00, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000,
    0x6000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x000e, 0x18e0, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x3c00, 0x0000, 0x1800, 0x661c, 0x001c, 0x1800, 0x1c18,
    0x3800, 0x0000, 0x0c00, 0x061c, 0x0000, 0x0000, 0x0636, 0x0000,
    0x3018, 0x3c3c, 0x6602, 0x1800, 0x380c, 0x0066, 0x1c0c, 0x3c38,
    0x0018, 0x1c0c, 0x3c36, 0x6600, 0x3836, 0x1866, 0x660c, 0x0000,
    0x0606, 0x1800, 0x0006, 0x0600, 0x0006, 0x0000, 0x0006, 0x1800,
    0x0006, 0x0006, 0x181b, 0x0000, 0x001c, 0x061b, 0x0006, 0x0018,
    0x0000, 0x0030, 0x0c7c, 0xfeee, 0x0100, 0x0008, 0x7838, 0x05a0,
    0xba02, 0x3a3a, 0x82b8, 0xb8ba, 0xbaba, 0x0078, 0x0000, 0x0dd8,
    0x0018, 0x6666, 0x3e66, 0x6c18, 0x0660, 0x6600, 0x0000, 0x0006,
    0x3c18, 0x3c7e, 0x0c7e, 0x1c7e, 0x3c3c, 0x0000, 0x0000, 0x003c,
    0x3818, 0x7c3c, 0x787e, 0x7e3e, 0x667e, 0x06cc, 0x60c6, 0x663c,
    0x7c3c, 0xf83e, 0x7e66, 0x66c6, 0x6666, 0x7e1e, 0x6078, 0x1000,
    0x7000, 0x6000, 0x0600, 0x0e00, 0x6018, 0x0cc0, 0x3800, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0018, 0x1830, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0018, 0x0060, 0x006c, 0x3e1c, 0x003e, 0x3e7e, 0x7e00, 0x7e00,
    0x6c00, 0x0038, 0x1876, 0x0c00, 0x0036, 0x0030, 0x0c6c, 0x3618,
    0xf800, 0x6600, 0x6666, 0x3c3c, 0x3c18, 0x7e00, 0x7e18, 0x6678,
    0x7800, 0x6618, 0x6600, 0x0000, 0xf81c, 0x0000, 0x0018, 0x7e18,
    0x0c0c, 0x3c66, 0x66ec, 0x0c00, 0x360c, 0x0066, 0x360c, 0x3cde,
    0x060c, 0x360c, 0x3c36, 0x6600, 0x6c36, 0x0c36, 0x660c, 0x0000,
    0x0018, 0x3c38, 0x1c38, 0xfec6, 0x013c, 0x000e, 0x4040, 0x05a0,
    0xc606, 0x0606, 0xc6c0, 0xc0c6, 0xc6c6, 0x0040, 0x0000, 0x0628,
    0x0018, 0x6666, 0x7e66, 0x6c18, 0x0c30, 0x6618, 0x0000, 0x0006,
    0x7e18, 0x7e7e, 0x0c7e, 0x3c7e, 0x7e7e, 0x0000, 0x0e00, 0xe07e,
    0x7c3c, 0x7e7e, 0x7c7e, 0x7e7e, 0x667e, 0x06cc, 0x60c6, 0x667e,
    0x7e7e, 0xfc7e, 0x7e66, 0x66c6, 0x6666, 0x7e1e, 0x6078, 0x3800,
    0x3800, 0x6000, 0x0600, 0x1e00, 0x6018, 0x0cc0, 0x3800, 0x0000,
    0x0000, 0x0000, 0x1800, 0x0000, 0x0000, 0x0018, 0x1830, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x003c, 0x0060, 0x0064, 0x7e36, 0x007e, 0x7e7e, 0x7e00, 0x7e7e,
    0x4400, 0x0038, 0x0072, 0x1800, 0x001c, 0x0033, 0x1800, 0x1c18,
    0xfc18, 0x003c, 0x186c, 0x7e7e, 0x7e7e, 0x7e7e, 0x7e3c, 0x3c7c,
    0x7c66, 0x663c, 0x3c3c, 0x3c00, 0xfc66, 0x6666, 0x6666, 0x7e3c,
    0x1818, 0x663c, 0x66e8, 0x1800, 0x1c18, 0x0066, 0x1c18, 0x6676,
    0x0618, 0x1c18, 0x666c, 0x6600, 0x381c, 0x186c, 0x6618, 0x1800,
    0x003c, 0x242c, 0x34ba, 0xfed6, 0x0366, 0x180f, 0x7040, 0x05a0,
    0xc606, 0x0606, 0xc6c0, 0xc0c6, 0xc6c6, 0x0070, 0x0000, 0x07d0,
    0x0018, 0x66ff, 0x606c, 0x3818, 0x1c38, 0x3c18, 0x0000, 0x0006,
    0x6638, 0x660c, 0x1c60, 0x7006, 0x6666, 0x1818, 0x1c7e, 0x7066,
    0xe67e, 0x6666, 0x6e60, 0x6060, 0x6618, 0x06d8, 0x60ee, 0x6666,
    0x6666, 0xcc60, 0x1866, 0x66c6, 0x6666, 0x0c18, 0x6018, 0x3800,
    0x1c00, 0x6000, 0x0600, 0x1800, 0x6000, 0x00c0, 0x1800, 0x0000,
    0x0000, 0x0000, 0x1800, 0x0000, 0x0000, 0x0018, 0x1830, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x007e, 0x0060, 0x8168, 0x6032, 0x0060, 0x6018, 0x0c00, 0x0c7e,
    0x6c00, 0x0018, 0x0034, 0x0000, 0x0000, 0x0031, 0x0000, 0x0000,
    0xcc3c, 0x3c7e, 0x3c60, 0x6666, 0x667e, 0x607e, 0x603c, 0x3c6e,
    0x6e66, 0x667e, 0x7e7e, 0x7e00, 0xcc66, 0x6666, 0x6666, 0x1866,
    0x0000, 0x0000, 0x0060, 0x0000, 0x0000, 0x0000, 0x0000, 0x0006,
    0x1f00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1800,
    0x0066, 0x24e6, 0x6792, 0xfc92, 0x03c3, 0x3c09, 0x4040, 0x0db0,
    0xc606, 0x0606, 0xc6c0, 0xc0c6, 0xc6c6, 0x7c40, 0x0000, 0x2e10,
    0x0018, 0x66ff, 0x600c, 0x3818, 0x1818, 0x3c18, 0x0000, 0x000c,
    0x6638, 0x660c, 0x1c60, 0x6006, 0x6666, 0x1818, 0x387e, 0x3866,
    0xc266, 0x6666, 0x6660, 0x6060, 0x6618, 0x06d8, 0x60ee, 0x7666,
    0x6666, 0xcc60, 0x1866, 0x66c6, 0x3c66, 0x0c18, 0x3018, 0x6c00,
    0x0c3c, 0x7c3c, 0x3e3c, 0x183e, 0x7c38, 0x0ccc, 0x186c, 0x3c3c,
    0x7c3e, 0x7c3e, 0x7e66, 0x66c6, 0x6666, 0x7e18, 0x1830, 0x6218,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0066, 0x0068, 0x4260, 0x6018, 0x0060, 0x6018, 0x0c00, 0x0c0c,
    0x383c, 0x0018, 0x0030, 0x3e00, 0x003e, 0x3efa, 0x7e00, 0x7e7e,
    0xcc7e, 0x7e66, 0x7e60, 0x6666, 0x6660, 0x6060, 0x6018, 0x1866,
    0x6676, 0x7666, 0x6666, 0x6642, 0xcc66, 0x6666, 0x6666, 0x1866,
    0x7c3c, 0x3c3c, 0x3c60, 0x3c3c, 0x3c3c, 0x3c3c, 0x3c38, 0x383e,
    0x063c, 0x3c3c, 0x3c3c, 0x3c18, 0x7c66, 0x6666, 0x6666, 0x7e00,
    0x00c3, 0x2483, 0xc1d6, 0xfcba, 0x0691, 0x3c08, 0x4038, 0x0db0,
    0x8202, 0x3a3a, 0xbab8, 0xb882, 0xbaba, 0x7e78, 0x0000, 0x39e0,
    0x0018, 0x6666, 0x7c18, 0x7018, 0x1818, 0xff7e, 0x007e, 0x000c,
    0x6618, 0x0c18, 0x3c7c, 0x600c, 0x3c7e, 0x1818, 0x7000, 0x1c0c,
    0xda66, 0x7e60, 0x667c, 0x7c6e, 0x7e18, 0x06f0, 0x60fe, 0x7666,
    0x6666, 0xcc70, 0x1866, 0x66c6, 0x3c3c, 0x1818, 0x3018, 0x6c00,
    0x043e, 0x7e7c, 0x7e7e, 0x7e7e, 0x7e38, 0x0cdc, 0x18fe, 0x7e7e,
    0x7e7e, 0x7e7e, 0x7e66, 0x66c6, 0x6666, 0x7e38, 0x1838, 0xf218,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x0066, 0x0078, 0x3c60, 0x703c, 0x0070, 0x7018, 0x1800, 0x180c,
    0x003e, 0x001c, 0x0030, 0x7e00, 0x007e, 0x7ef8, 0x7e00, 0x7e7e,
    0xcc66, 0x6666, 0x6660, 0x6060, 0x6060, 0x7c60, 0x6018, 0x1866,
    0x6676, 0x7666, 0x6666, 0x6666, 0xcc66, 0x6666, 0x667e, 0x1866,
    0x7e3e, 0x3e3e, 0x3e60, 0x7c7c, 0x7c7e, 0x7e7e, 0x7e38, 0x387e,
    0x3e7e, 0x7e7e, 0x7e7e, 0x7e18, 0x7e66, 0x6666, 0x6666, 0x7e00,
    0x0081, 0xe783, 0xc1c6, 0xf838, 0x0691, 0x3c08, 0x0000, 0x1db8,
    0x0000, 0x7c7c, 0x7c7c, 0x7c00, 0x7c7c, 0x0600, 0x0000, 0x3800,
    0x0018, 0x6666, 0x3e18, 0x7018, 0x1818, 0xff7e, 0x007e, 0x0018,
    0x6e18, 0x0c18, 0x3c7e, 0x7c0c, 0x3c3e, 0x1818, 0xe000, 0x0e0c,
    0xd666, 0x7c60, 0x667c, 0x7c6e, 0x7e18, 0x06f0, 0x60d6, 0x7e66,
    0x6666, 0xfc38, 0x1866, 0x66d6, 0x183c, 0x1818, 0x1818, 0xc600,
    0x0006, 0x6660, 0x6666, 0x7e66, 0x6618, 0x0cf8, 0x18fe, 0x6666,
    0x6666, 0x6660, 0x1866, 0x66d6, 0x3c66, 0x0cf0, 0x181e, 0xbe3c,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x0066, 0x0070, 0x6660, 0x3866, 0x0038, 0x3818, 0x187e, 0x1818,
    0x0006, 0x0038, 0x0030, 0x6000, 0x0060, 0x6030, 0x0c00, 0x0c0c,
    0xfc66, 0x6666, 0x6660, 0x6060, 0x607c, 0x7c7c, 0x7c18, 0x1866,
    0xf67e, 0x7e66, 0x6666, 0x663c, 0xfc66, 0x6666, 0x663c, 0x187c,
    0x6606, 0x0606, 0x0660, 0x6060, 0x6066, 0x6666, 0x6618, 0x1866,
    0x7e66, 0x6666, 0x6666, 0x6600, 0x6666, 0x6666, 0x6666, 0x1800,
    0x00e7, 0x81e6, 0x67d6, 0xfaba, 0x8c9d, 0x3c78, 0x1e1c, 0x399c,
    0x8202, 0xb83a, 0x3a3a, 0xba02, 0xba3a, 0x060e, 0x07f0, 0x0000,
    0x0018, 0x00ff, 0x0630, 0xde00, 0x1818, 0x3c18, 0x0000, 0x0018,
    0x7618, 0x180c, 0x6c06, 0x7e18, 0x6606, 0x0000, 0x707e, 0x1c18,
    0xd67e, 0x6660, 0x6660, 0x6066, 0x6618, 0x06d8, 0x60d6, 0x7e66,
    0x7e66, 0xf81c, 0x1866, 0x66d6, 0x1818, 0x3018, 0x1818, 0xc600,
    0x003e, 0x6660, 0x6666, 0x1866, 0x6618, 0x0cf0, 0x18d6, 0x6666,
    0x6666, 0x6070, 0x1866, 0x66d6, 0x3c66, 0x18f0, 0x181e, 0x9c24,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x007e, 0x00e0, 0x5a60, 0x1c66, 0x001c, 0x1c18, 0x307e, 0x3018,
    0x003e, 0x0018, 0x0030, 0x7000, 0x0070, 0x7030, 0x1800, 0x1818,
    0xf87e, 0x7e7e, 0x7e60, 0x6060, 0x607c, 0x607c, 0x7c18, 0x1866,
    0xf67e, 0x7e66, 0x6666, 0x6618, 0xf866, 0x6666, 0x6618, 0x1866,
    0x603e, 0x3e3e, 0x3e60, 0x6060, 0x6066, 0x6666, 0x6618, 0x1866,
    0x6666, 0x6666, 0x6666, 0x667e, 0x6066, 0x6666, 0x6666, 0x1800,
    0x0024, 0xc32c, 0x3492, 0xf292, 0x8c81, 0x3cf8, 0x1012, 0x799e,
    0xc606, 0xc006, 0x0606, 0xc606, 0xc606, 0x7e10, 0x0ff8, 0x0000,
    0x0018, 0x00ff, 0x0636, 0xde00, 0x1818, 0x3c18, 0x0000, 0x0030,
    0x6618, 0x180c, 0x6c06, 0x6618, 0x6606, 0x0000, 0x387e, 0x3818,
    0xdc7e, 0x6660, 0x6660, 0x6066, 0x6618, 0x06d8, 0x60c6, 0x6e66,
    0x7c66, 0xd80e, 0x1866, 0x66fe, 0x3c18, 0x3018, 0x0c18, 0x0000,
    0x007e, 0x6660, 0x667e, 0x1866, 0x6618, 0x0cf8, 0x18d6, 0x6666,
    0x6666, 0x603c, 0x1866, 0x66fe, 0x1866, 0x1838, 0x1838, 0x0066,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x007e, 0x00e0, 0x5a60, 0x0e3c, 0x000e, 0x0e18, 0x3000, 0x3030,
    0x007e, 0x0018, 0x0030, 0x3c00, 0x003c, 0x3c30, 0x1800, 0x1818,
    0xd87e, 0x7e7e, 0x7e60, 0x6060, 0x6060, 0x6060, 0x6018, 0x1866,
    0x666e, 0x6e66, 0x6666, 0x663c, 0xd866, 0x6666, 0x6618, 0x1866,
    0x607e, 0x7e7e, 0x7e60, 0x6060, 0x607e, 0x7e7e, 0x7e18, 0x1866,
    0x6666, 0x6666, 0x6666, 0x667e, 0x6066, 0x6666, 0x6666, 0x1800,
    0x0024, 0x6638, 0x1cba, 0xf6d6, 0xd8c3, 0x7e70, 0x1c1c, 0x718e,
    0xc606, 0xc006, 0x0606, 0xc606, 0xc606, 0x660c, 0x1fec, 0x0000,
    0x0000, 0x0066, 0x7e66, 0xcc00, 0x1818, 0x6618, 0x1800, 0x1830,
    0x6618, 0x3066, 0x7e06, 0x6630, 0x6606, 0x1818, 0x1c00, 0x7018,
    0xc066, 0x6666, 0x6660, 0x6066, 0x6618, 0x66cc, 0x60c6, 0x6e66,
    0x6066, 0xcc06, 0x1866, 0x3cfe, 0x3c18, 0x6018, 0x0c18, 0x0000,
    0x0066, 0x6660, 0x6660, 0x1866, 0x6618, 0x0cd8, 0x18d6, 0x6666,
    0x6666, 0x600e, 0x1866, 0x3cfe, 0x3c66, 0x3018, 0x1830, 0x0042,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x0066, 0x0060, 0x6660, 0x0618, 0x0006, 0x0618, 0x6000, 0x6030,
    0x0066, 0x0018, 0x0030, 0x0e00, 0x000e, 0x0e30, 0x3000, 0x3030,
    0xcc66, 0x6666, 0x6660, 0x6666, 0x6660, 0x6060, 0x6018, 0x1866,
    0x666e, 0x6e66, 0x6666, 0x6666, 0xcc66, 0x6666, 0x6618, 0x1866,
    0x6066, 0x6666, 0x6660, 0x6060, 0x6060, 0x6060, 0x6018, 0x1866,
    0x6666, 0x6666, 0x6666, 0x6600, 0x6066, 0x6666, 0x6666, 0x1800,
    0x0024, 0x3c30, 0x0c38, 0xe6c6, 0x5866, 0xff00, 0x1014, 0x718e,
    0xc606, 0xc006, 0x0606, 0xc606, 0xc606, 0x6602, 0x1804, 0x0000,
    0x0000, 0x0066, 0x7c66, 0xcc00, 0x1c38, 0x6600, 0x1800, 0x1860,
    0x6618, 0x3066, 0x7e66, 0x6630, 0x660e, 0x1818, 0x0e00, 0xe000,
    0xe266, 0x6666, 0x6e60, 0x6066, 0x6618, 0x66cc, 0x60c6, 0x6666,
    0x606a, 0xcc06, 0x1866, 0x3cee, 0x6618, 0x6018, 0x0618, 0x0000,
    0x0066, 0x6660, 0x6660, 0x187e, 0x6618, 0x0ccc, 0x18c6, 0x6666,
    0x6666, 0x6006, 0x1866, 0x3cee, 0x3c7e, 0x3018, 0x1830, 0x00c3,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x0066, 0x0060, 0x3c60, 0x064c, 0x0006, 0x0618, 0x6000, 0x6060,
    0x0066, 0x0018, 0x0030, 0x0600, 0x0006, 0x0630, 0x3000, 0x3030,
    0xcc66, 0x6666, 0x6660, 0x6666, 0x6660, 0x6060, 0x6018, 0x186e,
    0x6e66, 0x6666, 0x6666, 0x6642, 0xcc66, 0x6666, 0x6618, 0x1866,
    0x6066, 0x6666, 0x6660, 0x6060, 0x6060, 0x6060, 0x6018, 0x1866,
    0x6666, 0x6666, 0x6666, 0x6618, 0x6066, 0x6666, 0x667e, 0x1800,
    0x003c, 0x1800, 0x007c, 0xeeee, 0x703c, 0x1000, 0x1012, 0x6186,
    0xba02, 0xb83a, 0x023a, 0xba02, 0xba3a, 0x7e1c, 0x1804, 0x0000,
    0x0018, 0x0000, 0x1800, 0xfe00, 0x0c30, 0x0000, 0x1800, 0x1860,
    0x7e7e, 0x7e7e, 0x0c7e, 0x7e30, 0x7e3c, 0x1818, 0x0000, 0x0018,
    0x7e66, 0x7e7e, 0x7c7e, 0x607e, 0x667e, 0x7ec6, 0x7ec6, 0x667e,
    0x607c, 0xc67e, 0x187e, 0x18c6, 0x6618, 0x7e1e, 0x0678, 0x00fe,
    0x007e, 0x7e7e, 0x7e7e, 0x183e, 0x663c, 0x0cce, 0x3cc6, 0x667e,
    0x7e7e, 0x607e, 0x1e7e, 0x18c6, 0x663e, 0x7e18, 0x1830, 0x00ff,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x0066, 0x007e, 0x427e, 0x7e6c, 0x007e, 0x7e18, 0x7e00, 0x7e7e,
    0x007e, 0x003c, 0x0078, 0x7e00, 0x007e, 0x7e3c, 0x7e00, 0x7e7e,
    0xc666, 0x6666, 0x667e, 0x7e7e, 0x7e7e, 0x7e7e, 0x7e3c, 0x3c7c,
    0x7c66, 0x667e, 0x7e7e, 0x7e00, 0xc67e, 0x7e7e, 0x7e18, 0x1866,
    0x607e, 0x7e7e, 0x7ef0, 0x7e7e, 0x7e7e, 0x7e7e, 0x7e3c, 0x3c7e,
    0x7e66, 0x667e, 0x7e7e, 0x7e18, 0x607e, 0x7e7e, 0x7e3e, 0x1e00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x3000, 0x3800, 0x0000, 0x4182,
    0x7c00, 0x7c7c, 0x007c, 0x7c00, 0x7c7c, 0x3c00, 0x1004, 0x0000,
    0x0018, 0x0000, 0x1800, 0x7600, 0x0660, 0x0000, 0x1800, 0x1860,
    0x3c7e, 0x7e3c, 0x0c3c, 0x3c30, 0x3c38, 0x1818, 0x0000, 0x0018,
    0x3c66, 0x7c3c, 0x787e, 0x603c, 0x667e, 0x3cc6, 0x7ec6, 0x663c,
    0x6036, 0xc67c, 0x183c, 0x1882, 0x6618, 0x7e1e, 0x0678, 0x00fe,
    0x003e, 0x7c3e, 0x3e3e, 0x1806, 0x663c, 0x0cc6, 0x3cc6, 0x663c,
    0x7c3e, 0x607c, 0x0e3e, 0x1882, 0x6606, 0x7e18, 0x1830, 0x0000,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3e,
    0x0066, 0x007e, 0x817e, 0x7c38, 0x007c, 0x7c18, 0x7e00, 0x7e7e,
    0x003e, 0x003c, 0x0078, 0x7c00, 0x007c, 0x7c1c, 0x7e00, 0x7e7e,
    0xc666, 0x6666, 0x667e, 0x3c3c, 0x3c7e, 0x7e7e, 0x7e3c, 0x3c78,
    0x7866, 0x663c, 0x3c3c, 0x3c00, 0xc63c, 0x3c3c, 0x3c18, 0x18ec,
    0x603e, 0x3e3e, 0x3ef0, 0x3e3e, 0x3e3e, 0x3e3e, 0x3e3c, 0x3c3e,
    0x3e66, 0x663c, 0x3c3c, 0x3c00, 0x603e, 0x3e3e, 0x3e06, 0x0e00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x2000, 0x1000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e3c, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0030, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x007e, 0x0000, 0x7c00, 0x0000, 0x0000,
    0x6006, 0x0000, 0x0000, 0x0000, 0x007e, 0x000e, 0x18e0, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x000c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0c00, 0x0000, 0x0000,
    0x000c, 0x0c00, 0x0000, 0x0000, 0x0c00, 0x0c00, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x000c, 0x0000, 0x1800, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1800,
    0x0000, 0x0000, 0x0000, 0x000c, 0x0000, 0x1800, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x007e, 0x0c00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1754, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x007c, 0x0000, 0x7800, 0x0000, 0x0000,
    0x6006, 0x0000, 0x0000, 0x0000, 0x007c, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0006, 0x0000, 0x0000, 0x0000, 0x0000, 0x3800, 0x0000, 0x0000,
    0x0006, 0x0600, 0x0000, 0x0000, 0x3800, 0x3800, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0038, 0x0000, 0x0c00, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3000,
    0x0000, 0x0000, 0x0000, 0x0038, 0x0000, 0x0c00, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x007c, 0x3800,
};

const Fonthead fnt_l2_8x16 = {
    1,  /* font_id */
    10,  /* point */
    "MiNT ISO-8859-2 8x16",  /*   char name[32] */
    0,  /* first_ade */
    255,  /* last_ade */
    13,  /* top */
    11,  /* ascent */
    8,  /* half */
    2,  /* descent */
    2,  /* bottom */
    8,  /* max_char_width */
    8,  /* max_cell_width */
    1,  /* left_offset */
    7,  /* right_offset */
    1,  /* thicken */
    1,  /* ul_size */
    0x5555,  /* lighten */
    0x5555,  /* skew */
    F_STDFORM | F_DEFAULT | F_MONOSPACE,  /* flags */
    0,  /* hor_table */
    off_8x16_table,  /* off_table */
    dat_table,  /* dat_table */
    256,  /* form_width */
    16,  /* form_height */
    0    /* Fonthead * next_font */
};
