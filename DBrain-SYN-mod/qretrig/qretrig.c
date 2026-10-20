/* SPDX-License-Identifier: GPL-2.0-or-later */
/* QRETRIG for the Digitakt mk1 (seq-events): quantized retrig. While [TRIG 14], [TRIG 15] or [TRIG 16]
 * is held (grid recording off), the active audio track also fires on every 4th, 2nd or single step
 * of its own steps, on the sequencer's grid: 1/4, 1/8, 1/16 at the default scale. Those keys no
 * longer play MIDI tracks F-H then. With grid recording on, or with [FUNC], they are stock. Their LEDs
 * are bright blue while they are retrig keys, white while one is held. */
#include "digitakt-mk1/seq.h"
#include "digitakt-mk1/core3.h"

#define GRID_REC   (*(volatile uint8_t *)0x4208d680)    /* OS 1.54: 1 while grid recording is on */
#define KEY_T14    37                                   /* [TRIG 14]-[TRIG 16]: 37-39 */
#define F_PRESS    1
#define F_FUNC     2
#define F_REPEAT   8

static const uint8_t divs[3] = { 4, 2, 1 };
static volatile int32_t rdiv;          /* 0 off */
static volatile int32_t rtrack = -1;
static int32_t held;                    /* the key holding it, 0 none */

int qr_key(void *brain, void *ev)
{
    int32_t key = *(int32_t *)((char *)ev + 12);
    int32_t fl = *(int32_t *)((char *)ev + 16);
    int32_t t;
    (void)brain;
    if (key < KEY_T14 || key > KEY_T14 + 2)
        return 0;
    if (fl & F_PRESS) {
        if (fl & F_REPEAT)
            return held == key;
        if (GRID_REC || (fl & F_FUNC))
            return 0;
        t = fw_active_track;
        if (t < 0 || t >= SQ_AUDIO)
            return 0;
        rtrack = t;
        rdiv = divs[key - KEY_T14];
        held = key;
        return 1;
    }
    if (held == key) {
        held = 0;
        rdiv = 0;
        return 1;
    }
    return 0;
}

/* interrupt level */
void qr_trig(int32_t track, int32_t step, int32_t first, uint32_t *trig)
{
    int32_t d = rdiv;
    (void)first;
    if (d && track == rtrack && step % d == 0)
        *trig |= SQ_TRIG;
}

void qr_cond(int32_t track, int32_t step, int32_t *pass)
{
    int32_t d = rdiv;
    if (d && track == rtrack && step % d == 0)
        *pass = 1;
}

/* the UI task, from the LED setter (qr_led.s): LED 14-16's colour (the setter counts from 1), a palette index */
#define PAL_BLUE   14                                   /* 0x00001f */
#define PAL_WHITE  20                                   /* 0x1f1f1f */
int32_t qr_led_colour(int32_t led, int32_t colour)
{
    if (GRID_REC)
        return colour;
    return held == KEY_T14 + (led - 14) ? PAL_WHITE : PAL_BLUE;
}
