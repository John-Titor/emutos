/*
 * fnt_tr_6x6.c - 6x6 font for Turkish language
 *
 * Copyright (C) 2021 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"
#include "bios.h"
#include "fonthdr.h"

static const UWORD dat_table[] =
{
    0x0082, 0x0421, 0xcfb6, 0x0de3, 0x04e3, 0x8150, 0xf987, 0xbcc3,
    0xcc3e, 0x73e0, 0x381f, 0x8442, 0x00cd, 0x947b, 0x260c, 0x3184,
    0x8800, 0x0006, 0x704f, 0x3c33, 0xc73e, 0x71c3, 0x0c18, 0x061c,
    0x71cf, 0x1ef3, 0xef9e, 0x89c0, 0x9242, 0x289c, 0xf1cf, 0x1efa,
    0x28a2, 0x8a2f, 0x9ec1, 0xe200, 0x6008, 0x0008, 0x0180, 0x8081,
    0x2060, 0x0000, 0x0000, 0x0020, 0x0000, 0x0000, 0x0e31, 0xc400,
    0x3800, 0x0600, 0x0208, 0x2325, 0x0678, 0x0000, 0x0081, 0x1428,
    0xc000, 0x43d2, 0x9800, 0x0014, 0x00c1, 0x0686, 0x2306, 0x51e7,
    0x0d00, 0x07be, 0x3087, 0x1c10, 0x0680, 0x00c7, 0x2cc3, 0x0c18,
    0x4042, 0x1a50, 0x879e, 0x4042, 0x1240, 0x4214, 0x49e4, 0x0421,
    0xa880, 0x3501, 0x0850, 0x879c, 0x4042, 0x1a50, 0x8000, 0x4042,
    0x1440, 0x4214, 0x87c4, 0x0421, 0xa008, 0x0901, 0x0850, 0x0014,
    0x01c2, 0x0662, 0xaf2a, 0x1a17, 0x8682, 0x0150, 0xc880, 0x84c2,
    0x0c02, 0x5367, 0x203f, 0x42f4, 0x00cd, 0xbea3, 0x4d0c, 0x60c3,
    0x0800, 0x000c, 0x98c0, 0x8252, 0x0802, 0x8a23, 0x0c31, 0xe326,
    0x8a28, 0xa08a, 0x0820, 0x8880, 0x9443, 0x6ca2, 0x8a28, 0xa022,
    0x28a2, 0x5221, 0x1860, 0x6700, 0x61cf, 0x1c79, 0xc21e, 0xb181,
    0x2421, 0x4f1c, 0xf1e7, 0x0e72, 0x28a2, 0x4a27, 0x8c30, 0xce88,
    0x4000, 0x0800, 0x071c, 0x5347, 0x8cb0, 0x0000, 0x0081, 0x1429,
    0xe000, 0xa971, 0x0c70, 0x0000, 0x0003, 0x887b, 0x6308, 0x0330,
    0x9b00, 0x08c0, 0x4be1, 0x8c21, 0xae80, 0x01c8, 0xb6c3, 0x0400,
    0x2085, 0x2c00, 0x0a20, 0x23e5, 0x3e21, 0xc700, 0x7802, 0x1c52,
    0xc712, 0x488a, 0x9489, 0xc836, 0x2085, 0x2c00, 0x0f1e, 0x2085,
    0x0020, 0x8500, 0x7802, 0x0852, 0xc880, 0x3082, 0x1401, 0x8380,
    0x0362, 0x3bdf, 0x6e1c, 0xb297, 0x84de, 0xe150, 0xc88f, 0xbec3,
    0xef8e, 0x7320, 0xb760, 0x6294, 0x00c9, 0x1470, 0x8618, 0x60c7,
    0xbe01, 0xe018, 0xa847, 0x1c93, 0xcf04, 0x71e0, 0x0060, 0x018c,
    0xbbef, 0x208b, 0xcf26, 0xf880, 0x9842, 0xaaa2, 0xf22f, 0x1c22,
    0x28aa, 0x2142, 0x1830, 0x6d80, 0x3028, 0xa08b, 0xe7a2, 0xc881,
    0x3823, 0xe8a2, 0x8a24, 0x9822, 0x28aa, 0x3221, 0x1830, 0x6b9c,
    0xf8c0, 0x1e00, 0x0208, 0x0088, 0x18b8, 0xc30c, 0x3042, 0x0a50,
    0xc7bf, 0xa953, 0x86b8, 0xc322, 0x00c6, 0x1c49, 0xc01c, 0x02df,
    0xb679, 0xeb40, 0x3083, 0x0401, 0xae8c, 0x00c8, 0x9bcf, 0x6cd8,
    0x71c7, 0x1c71, 0xcfa0, 0xfa0f, 0xa070, 0x821c, 0x8127, 0x2271,
    0xc88c, 0x5a28, 0xa288, 0x873c, 0x71c7, 0x1c71, 0xc3a0, 0x71c7,
    0x1c61, 0x8218, 0x8bc7, 0x1c71, 0xc73e, 0x5a28, 0xa288, 0x8622,
    0x008d, 0x8662, 0xacaa, 0xe2df, 0xdc93, 0xa358, 0xd9cc, 0x06d8,
    0x698c, 0xdbef, 0xa440, 0x2168, 0x00c0, 0x3e29, 0x6e80, 0x60c3,
    0x0830, 0x0330, 0xc848, 0x02f8, 0x2888, 0x8823, 0x0c31, 0xe30c,
    0xb228, 0xa08a, 0x0822, 0x8888, 0x9442, 0x29a2, 0x822a, 0x0222,
    0x2536, 0x5084, 0x1818, 0x6000, 0x03e8, 0xa08a, 0x021e, 0x8881,
    0x2422, 0xa8a2, 0x8a24, 0x0622, 0x252a, 0x31e2, 0x0c30, 0xc132,
    0xf0c3, 0x082a, 0xa21c, 0x0157, 0x0cb0, 0xc30c, 0x3000, 0x0000,
    0x0000, 0x1006, 0x0ca0, 0xc31c, 0x00c3, 0x8848, 0x831c, 0x02f7,
    0x9b08, 0x08c0, 0x0007, 0x9c01, 0xa68c, 0x01e7, 0x3614, 0xb158,
    0x8a28, 0xa28a, 0x2a20, 0xe3ce, 0x3c20, 0x8208, 0x99a8, 0xa28a,
    0x288c, 0x6a28, 0xa288, 0x80b6, 0x0820, 0x8208, 0x2e20, 0xfbef,
    0xbe20, 0x8208, 0x7a28, 0xa28a, 0x2880, 0x6a28, 0xa288, 0x819e,
    0x0087, 0x0421, 0xc9b6, 0x4210, 0x3c18, 0xe75c, 0xd9cc, 0x06f8,
    0x6d8c, 0xd867, 0x3c71, 0xeef0, 0x0000, 0x14f2, 0x6d00, 0x3184,
    0x8830, 0x0320, 0x704f, 0xbc13, 0xc708, 0x71c3, 0x0418, 0x0600,
    0x822f, 0x1ef3, 0xe81e, 0x89c7, 0x127a, 0x289c, 0x81c9, 0xbc21,
    0xe222, 0x888f, 0x9e09, 0xe000, 0x01ef, 0x1c79, 0xc202, 0x89c1,
    0x2272, 0x289c, 0xf1e4, 0x1c11, 0xe236, 0x4827, 0x8e31, 0xc03e,
    0x4003, 0x082a, 0xa208, 0x0250, 0x8678, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0001, 0x9878, 0x0008, 0x00c1, 0x1e79, 0xc308, 0x0310,
    0x0d08, 0x0a40, 0x03e0, 0x0001, 0xf280, 0x1000, 0x2c3c, 0x63f2,
    0xfbef, 0xbefb, 0xeb9e, 0x8208, 0x2020, 0x8208, 0x7968, 0xa28a,
    0x2712, 0x4a28, 0xa279, 0xcf3c, 0xfbef, 0xbefb, 0xe79e, 0x8208,
    0x2020, 0x8208, 0x0a28, 0xa28a, 0x2708, 0x3228, 0xa289, 0xc702,
    0x0082, 0x0000, 0x0000, 0x01e3, 0x1810, 0xb64c, 0xf9cf, 0xbe1b,
    0xef8c, 0xf860, 0x0758, 0xac00, 0x00c0, 0x0020, 0x0680, 0x0000,
    0x0060, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0800, 0x000c,
    0x7800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0060, 0x0000,
    0x0000, 0x0000, 0x0000, 0x003e, 0x0000, 0x0000, 0x003c, 0x000e,
    0x0000, 0x0000, 0x8020, 0x0000, 0x0000, 0x03c0, 0x0030, 0x0000,
    0x3806, 0x3050, 0x0000, 0x000f, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0007, 0x0000, 0x0008, 0x00c0, 0x0084, 0x8330, 0x01ef,
    0x8000, 0x0780, 0x0000, 0x0003, 0x0280, 0x200f, 0x8004, 0xf05c,
    0x8a28, 0xa28a, 0x200c, 0xfbef, 0xbe71, 0xc71c, 0x0127, 0x1c71,
    0xc000, 0xb1c7, 0x9c00, 0x0630, 0x79e7, 0x9e79, 0xe00c, 0x71c7,
    0x1c71, 0xc71c, 0xf227, 0x1c71, 0xc000, 0x41e7, 0x9e78, 0x023c,
};

const Fonthead fnt_tr_6x6 = {
    1,                  /*   WORD font_id       */
    8,                  /*   WORD point         */
    "6x6 Turkish font", /*   char name[32]      */
    0,                  /*   UWORD first_ade    */
    255,                /*   UWORD last_ade     */
    4,                  /*   UWORD top          */
    4,                  /*   UWORD ascent       */
    3,                  /*   UWORD half         */
    1,                  /*   UWORD descent      */
    1,                  /*   UWORD bottom       */
    5,                  /*   UWORD max_char_width*/
    6,                  /*   UWORD max_cell_width*/
    0,                  /*   UWORD left_offset  */
    3,                  /*   UWORD right_offset */
    1,                  /*   UWORD thicken      */
    1,                  /*   UWORD ul_size      */
    0x5555,             /*   UWORD lighten      */
    0xAAAA,             /*   UWORD skew         */
    F_STDFORM | F_MONOSPACE,/*   UWORD flags        */

    0,                  /*   UBYTE *hor_table   */
    off_6x6_table,      /*   UWORD *off_table   */
    dat_table,          /*   UWORD *dat_table   */
    192,                /*   UWORD form_width   */
    6,                  /*   UWORD form_height  */

    0                   /*   Fonthead * next_font    */
};
