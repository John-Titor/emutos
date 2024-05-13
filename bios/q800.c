/*
 * q800.c - Quadra 800 specific functions.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"
#ifdef MACHINE_Q800
#include "asm.h"
#include "blkdev.h"
#include "clock.h"
#include "delay.h"
#include "disk.h"
#include "gemerror.h"
#include "q800.h"
#include "ikbd.h"
#include "machine.h"
#include "processor.h"
#include "screen.h"
#include "serport.h"
#include "string.h"
#include "tosvars.h"
#include "vectors.h"
#include "cookie.h"

#endif /* MACHINE_Q800 */