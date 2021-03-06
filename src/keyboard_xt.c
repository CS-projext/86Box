/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box distribution.
 *
 *		Implementation of the XT-style keyboard.
 *
 * Version:	@(#)keyboard_xt.c	1.0.15	2019/01/24
 *
 * Authors:	Sarah Walker, <http://pcem-emulator.co.uk/>
 *		Miran Grca, <mgrca8@gmail.com>
 *		Fred N. van Kempen, <decwiz@yahoo.com>
 *
 *		Copyright 2008-2018 Sarah Walker.
 *		Copyright 2016-2018 Miran Grca.
 *		Copyright 2017,2018 Fred N. van kempen.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "86box.h"
#include "floppy/fdd.h"
#include "machine/machine.h"
#include "io.h"
#include "pic.h"
#include "pit.h"
#include "ppi.h"
#include "mem.h"
#include "rom.h"
#include "timer.h"
#include "device.h"
#include "sound/sound.h"
#include "sound/snd_speaker.h"
#include "video/video.h"
#include "keyboard.h"


#define STAT_PARITY     0x80
#define STAT_RTIMEOUT   0x40
#define STAT_TTIMEOUT   0x20
#define STAT_LOCK       0x10
#define STAT_CD         0x08
#define STAT_SYSFLAG    0x04
#define STAT_IFULL      0x02
#define STAT_OFULL      0x01


typedef struct {
    int want_irq;        
    int blocked;
    int tandy;

    uint8_t pa, pb, pd;
    uint8_t key_waiting;
    uint8_t type;
} xtkbd_t;


/*XT keyboard has no escape scancodes, and no scancodes beyond 53*/
const scancode scancode_xt[512] = {
    { {-1},       {-1}       }, { {0x01, -1}, {0x81, -1} },
    { {0x02, -1}, {0x82, -1} }, { {0x03, -1}, {0x83, -1} },
    { {0x04, -1}, {0x84, -1} }, { {0x05, -1}, {0x85, -1} },
    { {0x06, -1}, {0x86, -1} }, { {0x07, -1}, {0x87, -1} },
    { {0x08, -1}, {0x88, -1} }, { {0x09, -1}, {0x89, -1} },
    { {0x0a, -1}, {0x8a, -1} }, { {0x0b, -1}, {0x8b, -1} },
    { {0x0c, -1}, {0x8c, -1} }, { {0x0d, -1}, {0x8d, -1} },
    { {0x0e, -1}, {0x8e, -1} }, { {0x0f, -1}, {0x8f, -1} },
    { {0x10, -1}, {0x90, -1} }, { {0x11, -1}, {0x91, -1} },
    { {0x12, -1}, {0x92, -1} }, { {0x13, -1}, {0x93, -1} },
    { {0x14, -1}, {0x94, -1} }, { {0x15, -1}, {0x95, -1} },
    { {0x16, -1}, {0x96, -1} }, { {0x17, -1}, {0x97, -1} },
    { {0x18, -1}, {0x98, -1} }, { {0x19, -1}, {0x99, -1} },
    { {0x1a, -1}, {0x9a, -1} }, { {0x1b, -1}, {0x9b, -1} },
    { {0x1c, -1}, {0x9c, -1} }, { {0x1d, -1}, {0x9d, -1} },
    { {0x1e, -1}, {0x9e, -1} }, { {0x1f, -1}, {0x9f, -1} },
    { {0x20, -1}, {0xa0, -1} }, { {0x21, -1}, {0xa1, -1} },
    { {0x22, -1}, {0xa2, -1} }, { {0x23, -1}, {0xa3, -1} },
    { {0x24, -1}, {0xa4, -1} }, { {0x25, -1}, {0xa5, -1} },
    { {0x26, -1}, {0xa6, -1} }, { {0x27, -1}, {0xa7, -1} },
    { {0x28, -1}, {0xa8, -1} }, { {0x29, -1}, {0xa9, -1} },
    { {0x2a, -1}, {0xaa, -1} }, { {0x2b, -1}, {0xab, -1} },
    { {0x2c, -1}, {0xac, -1} }, { {0x2d, -1}, {0xad, -1} },
    { {0x2e, -1}, {0xae, -1} }, { {0x2f, -1}, {0xaf, -1} },
    { {0x30, -1}, {0xb0, -1} }, { {0x31, -1}, {0xb1, -1} },
    { {0x32, -1}, {0xb2, -1} }, { {0x33, -1}, {0xb3, -1} },
    { {0x34, -1}, {0xb4, -1} }, { {0x35, -1}, {0xb5, -1} },
    { {0x36, -1}, {0xb6, -1} }, { {0x37, -1}, {0xb7, -1} },
    { {0x38, -1}, {0xb8, -1} }, { {0x39, -1}, {0xb9, -1} },
    { {0x3a, -1}, {0xba, -1} }, { {0x3b, -1}, {0xbb, -1} },
    { {0x3c, -1}, {0xbc, -1} }, { {0x3d, -1}, {0xbd, -1} },
    { {0x3e, -1}, {0xbe, -1} }, { {0x3f, -1}, {0xbf, -1} },
    { {0x40, -1}, {0xc0, -1} }, { {0x41, -1}, {0xc1, -1} },
    { {0x42, -1}, {0xc2, -1} }, { {0x43, -1}, {0xc3, -1} },
    { {0x44, -1}, {0xc4, -1} }, { {0x45, -1}, {0xc5, -1} },
    { {0x46, -1}, {0xc6, -1} }, { {0x47, -1}, {0xc7, -1} },
    { {0x48, -1}, {0xc8, -1} }, { {0x49, -1}, {0xc9, -1} },
    { {0x4a, -1}, {0xca, -1} }, { {0x4b, -1}, {0xcb, -1} },
    { {0x4c, -1}, {0xcc, -1} }, { {0x4d, -1}, {0xcd, -1} },
    { {0x4e, -1}, {0xce, -1} }, { {0x4f, -1}, {0xcf, -1} },
    { {0x50, -1}, {0xd0, -1} }, { {0x51, -1}, {0xd1, -1} },
    { {0x52, -1}, {0xd2, -1} }, { {0x53, -1}, {0xd3, -1} },
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*054*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*058*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*05c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*060*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*064*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*068*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*06c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*070*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*074*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*078*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*07c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*080*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*084*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*088*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*08c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*090*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*094*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*098*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*09c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0a0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0a4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0a8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0ac*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0b0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0b4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0b8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0bc*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0c0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0c4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0c8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0cc*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0d0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0d4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0d8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0dc*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0e0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0e4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0e8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0ec*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0f0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0f4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0f8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*0fc*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*100*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*104*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*108*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*10c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*110*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*114*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*118*/
    { {0x1c, -1}, {0x9c, -1} }, { {0x1d, -1}, {0x9d, -1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*11c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*120*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*124*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*128*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*12c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*130*/
    { {-1},             {-1} }, { {0x35, -1}, {0xb5, -1} },
    { {-1},             {-1} }, { {0x37, -1}, {0xb7, -1} },	/*134*/
    { {0x38, -1}, {0xb8, -1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*138*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*13c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*140*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {0x46, -1}, {0xc6, -1} }, { {0x47, -1}, {0xc7, -1} },	/*144*/
    { {0x48, -1}, {0xc8, -1} }, { {0x49, -1}, {0xc9, -1} },
    { {-1},             {-1} }, { {0x4b, -1}, {0xcb, -1} },	/*148*/
    { {-1},             {-1} }, { {0x4d, -1}, {0xcd, -1} },
    { {-1},             {-1} }, { {0x4f, -1}, {0xcf, -1} },	/*14c*/
    { {0x50, -1}, {0xd0, -1} }, { {0x51, -1}, {0xd1, -1} },
    { {0x52, -1}, {0xd2, -1} }, { {0x53, -1}, {0xd3, -1} },	/*150*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*154*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*158*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*15c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*160*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*164*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*168*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*16c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*170*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*174*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*148*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*17c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*180*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*184*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*88*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*18c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*190*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*194*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*198*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*19c*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1a0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1a4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1a8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1ac*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1b0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1b4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1b8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1bc*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1c0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1c4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1c8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1cc*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1d0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1d4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1d8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1dc*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1e0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1e4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1e8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1ec*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1f0*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1f4*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} },	/*1f8*/
    { {-1},             {-1} }, { {-1},             {-1} },
    { {-1},             {-1} }, { {-1},             {-1} }	/*1fc*/
};


static uint8_t	key_queue[16];
static int	key_queue_start,
		key_queue_end;


static void
kbd_poll(void *priv)
{
    xtkbd_t *kbd = (xtkbd_t *)priv;

    keyboard_delay += (1000LL * TIMER_USEC);

    if (!(kbd->pb & 0x40) && (romset != ROM_TANDY))
	return;

    if (kbd->want_irq) {
	kbd->want_irq = 0;
	kbd->pa = kbd->key_waiting;
	kbd->blocked = 1;
	picint(2);
    }

    if (key_queue_start != key_queue_end && !kbd->blocked) {
	kbd->key_waiting = key_queue[key_queue_start];
#if ENABLE_KEYBOARD_LOG
	pclog("XTkbd: reading %02X from the key queue at %i\n",
				kbd->pa, key_queue_start);
#endif
	key_queue_start = (key_queue_start + 1) & 0x0f;
	kbd->want_irq = 1;
    }
}


static void
kbd_adddata(uint16_t val)
{
    key_queue[key_queue_end] = val;
#if ENABLE_KEYBOARD_LOG
    pclog("XTkbd: %02X added to key queue at %i\n",
				val, key_queue_end);
#endif
    key_queue_end = (key_queue_end + 1) & 0x0f;
}


void
kbd_adddata_process(uint16_t val, void (*adddata)(uint16_t val))
{
    uint8_t num_lock = 0, shift_states = 0;

    if (!adddata)
	return;

    keyboard_get_states(NULL, &num_lock, NULL);
    shift_states = keyboard_get_shift() & STATE_SHIFT_MASK;

    switch(val) {
	case FAKE_LSHIFT_ON:
		if (num_lock) {
			if (!shift_states) {
				/* Num lock on and no shifts are pressed, send non-inverted fake shift. */
				adddata(0x2a);
			}
		} else {
			if (shift_states & STATE_LSHIFT) {
				/* Num lock off and left shift pressed. */
				adddata(0xaa);
			}
			if (shift_states & STATE_RSHIFT) {
				/* Num lock off and right shift pressed. */
				adddata(0xb6);
			}
		}
		break;
	case FAKE_LSHIFT_OFF:
		if (num_lock) {
			if (!shift_states) {
				/* Num lock on and no shifts are pressed, send non-inverted fake shift. */
				adddata(0xaa);
			}
		} else {
			if (shift_states & STATE_LSHIFT) {
				/* Num lock off and left shift pressed. */
				adddata(0x2a);
			}
			if (shift_states & STATE_RSHIFT) {
				/* Num lock off and right shift pressed. */
				adddata(0x36);
			}
		}
		break;
	default:
		adddata(val);
		break;
    }
}


static void
kbd_adddata_ex(uint16_t val)
{
	kbd_adddata_process(val, kbd_adddata);
}


static void
kbd_write(uint16_t port, uint8_t val, void *priv)
{
    xtkbd_t *kbd = (xtkbd_t *)priv;

    switch (port) {
	case 0x61:
		if (!(kbd->pb & 0x40) && (val & 0x40)) {
			key_queue_start = key_queue_end = 0;
			kbd->want_irq = 0;
			kbd->blocked = 0;
			kbd_adddata(0xaa);
		}
		kbd->pb = val;
		ppi.pb = val;

		timer_process();
		timer_update_outstanding();

		speaker_update();
		speaker_gated = val & 1;
		speaker_enable = val & 2;
		if (speaker_enable) 
			was_speaker_enable = 1;
		pit_set_gate(&pit, 2, val & 1);

		if (val & 0x80) {
			kbd->pa = 0;
			kbd->blocked = 0;
			picintc(2);
		}
		break;
    }
}


static uint8_t
kbd_read(uint16_t port, void *priv)
{
    xtkbd_t *kbd = (xtkbd_t *)priv;
    uint8_t ret = 0xff;

    switch (port) {
	case 0x60:
		if ((kbd->type <= 1) && (kbd->pb & 0x80))
			ret = kbd->pd;
		else if (((kbd->type == 2) || (kbd->type == 3)) && (kbd->pb & 0x80))
			ret = 0xff;	/* According to Ruud on the PCem forum, this is supposed to return 0xFF on the XT. */
		else
			ret = kbd->pa;
		break;

	case 0x61:
		ret = kbd->pb;
		break;

	case 0x62:
		if (!kbd->type)
			ret = 0x00;
		else if (kbd->type == 1) {
			if (kbd->pb & 0x04)
				ret = ((mem_size-64) / 32) & 0x0f;
			else
				ret = ((mem_size-64) / 32) >> 4;
		} else {
			if (kbd->pb & 0x08)
				ret = kbd->pd >> 4;
			else {
				/* LaserXT = Always 512k RAM;
				   LaserXT/3 = Bit 0: set = 512k, clear = 256k. */
#if defined(DEV_BRANCH) && defined(USE_LASERXT)
				if (kbd->type == 5)
					ret = (mem_size == 512) ? 0x0d : 0x0c;
				else
#endif
					ret = kbd->pd & 0x0f;
			}
		}
		ret |= (ppispeakon ? 0x20 : 0);

		if (kbd->type == 4)
			ret |= (tandy1k_eeprom_read() ? 0x10 : 0);
		break;

	case 0x63:
		if ((kbd->type == 2) || (kbd->type == 3))
			ret = kbd->pd;
		break;
    }

    return(ret);
}


static void
kbd_reset(void *priv)
{
    xtkbd_t *kbd = (xtkbd_t *)priv;

    kbd->want_irq = 0;
    kbd->blocked = 0;
    kbd->pa = 0x00;
    kbd->pb = 0x00;

    keyboard_scan = 1;

    key_queue_start = 0,
    key_queue_end = 0;
}


static void *
kbd_init(const device_t *info)
{
    int i, fdd_count = 0;

    xtkbd_t *kbd;

    kbd = (xtkbd_t *)malloc(sizeof(xtkbd_t));
    memset(kbd, 0x00, sizeof(xtkbd_t));

    io_sethandler(0x0060, 4,
		  kbd_read, NULL, NULL, kbd_write, NULL, NULL, kbd);
    keyboard_send = kbd_adddata_ex;
    kbd_reset(kbd);
    kbd->type = info->local;

    if (kbd->type <= 3) {
	for (i = 0; i < FDD_NUM; i++) {
		if (fdd_get_flags(i))
			fdd_count++;
	}

	/* DIP switch readout: bit set = OFF, clear = ON. */
	    /* Switches 7, 8 - floppy drives. */
	    if (!fdd_count)
		kbd->pd = 0x00;
	    else
		kbd->pd = ((fdd_count - 1) << 6) | 0x01;
	/* Switches 5, 6 - video. */
	if (video_is_mda())
		kbd->pd |= 0x30;
	else
		kbd->pd |= 0x20;	/* 0x10 would be 40x25 */
	/* Switches 3, 4 - memory size. */
	if (kbd->type == 3) {
		switch (mem_size) {
			case 256:
				kbd->pd |= 0x00;
				break;
			case 512:
				kbd->pd |= 0x04;
				break;
			case 576:
				kbd->pd |= 0x08;
				break;
			case 640:
			default:
				kbd->pd |= 0x0c;
				break;
		}
	} else if (kbd->type == 2) {
		switch (mem_size) {
			case 64:
				kbd->pd |= 0x00;
				break;
			case 128:
				kbd->pd |= 0x04;
				break;
			case 192:
				kbd->pd |= 0x08;
				break;
			case 256:
			default:
				kbd->pd |= 0x0c;
				break;
		}
	} else {
		switch (mem_size) {
			case 16:
				kbd->pd |= 0x00;
				break;
			case 32:
				kbd->pd |= 0x04;
				break;
			case 48:
				kbd->pd |= 0x08;
				break;
			case 64:
			default:
				kbd->pd |= 0x0c;
				break;
		}
	}
	/* Switch 2 - return bit clear (switch ON) because no 8087 right now. */
	/* Switch 1 - always off. */
		kbd->pd |= 0x01;
    }

    timer_add(kbd_poll, &keyboard_delay, TIMER_ALWAYS_ENABLED, kbd);

    keyboard_set_table(scancode_xt);

    return(kbd);
}


static void
kbd_close(void *priv)
{
    xtkbd_t *kbd = (xtkbd_t *)priv;

    /* Stop the timer. */
    keyboard_delay = 0;

    /* Disable scanning. */
    keyboard_scan = 0;

    keyboard_send = NULL;

    io_removehandler(0x0060, 4,
		     kbd_read, NULL, NULL, kbd_write, NULL, NULL, kbd);

    free(kbd);
}


const device_t keyboard_pc_device = {
    "IBM PC Keyboard (1981)",
    0,
    0,
    kbd_init,
    kbd_close,
    kbd_reset,
    NULL, NULL, NULL
};

const device_t keyboard_pc82_device = {
    "IBM PC Keyboard (1982)",
    0,
    1,
    kbd_init,
    kbd_close,
    kbd_reset,
    NULL, NULL, NULL
};

const device_t keyboard_xt_device = {
    "XT (1982) Keyboard",
    0,
    2,
    kbd_init,
    kbd_close,
    kbd_reset,
    NULL, NULL, NULL
};

const device_t keyboard_xt86_device = {
    "XT (1986) Keyboard",
    0,
    3,
    kbd_init,
    kbd_close,
    kbd_reset,
    NULL, NULL, NULL
};

const device_t keyboard_tandy_device = {
    "Tandy 1000 Keyboard",
    0,
    4,
    kbd_init,
    kbd_close,
    kbd_reset,
    NULL, NULL, NULL
};

#if defined(DEV_BRANCH) && defined(USE_LASERXT)
const device_t keyboard_xt_lxt3_device = {
    "VTech Laser XT3 Keyboard",
    0,
    /*5*/ 3,
    kbd_init,
    kbd_close,
    kbd_reset,
    NULL, NULL, NULL
};
#endif
