/* SPDX-License-Identifier: GPL-2.0-or-later */
/* DRUMBRAIN for the Digitakt mk1 (seq-events + an SRC machine): every audio track whose SRC machine
 * is one of DRUMBRAIN's (DB HOUSE, DB BREAKS: the style) plays a rhythm drawn from
 * DrumBrain's variable-order Markov model in that style, whatever its own trigs say; the machine plays the sample as ONESHOT does, with ONESHOT's
 * page and parameters. Each track keeps its instrument: T1 BD, T2 SD, T3 CP, T4 RS, T5 CH, T6 OH,
 * T7 TOM, T8 CY. The rhythm stays
 * until the player asks for a new one, which plays from the track's next start: [TRK] + the same
 * [TRIG 1-8] twice in a row for that track, [TRK] held + [FUNC] for every DRUMBRAIN track. Switching
 * a track to DRUMBRAIN also gives it a new one. Every other track plays as stock. Strong hits and the accent lane
 * set the velocity. The stored pattern is never changed. */
#include "digitakt-mk1/seq.h"
#include "dbmodel.h"
#include "digitakt-mk1/core3.h"

/* Keys (measured on a unit with keyspy): ev+12 the key, ev+16 flags: 1 pressed, 8 held (repeat),
 * 0x10 released (0 when released after a combination), 2 while [FUNC] is held. */
#define KEY_FUNC   1
#define KEY_TRK    2
#define KEY_TRIG1  24                   /* [TRIG 1]-[TRIG 16]: 24-39 */
#define F_PRESS    1
#define DOUBLE_TICKS 20                 /* two selections of a track within 2/3 s */
#define MSG_TICKS  45
#define DB_MACHINE_ID 41                /* 41 house, 42 breaks: machine:41-42 */

typedef void (*rect_fn)(void *, int32_t, int32_t, int32_t, int32_t, int32_t);
typedef int32_t (*text_fn)(void *, const void *, int32_t, int32_t, int32_t, const char *, ...);

#define LEN       32
#define VLO       90
#define VHI       115
#define VACC      12
#define MAXCTX    32

static uint8_t buf[2][8][LEN];          /* velocity per step, 0 = no hit */
static volatile uint8_t front[8];
static volatile uint8_t ready[8];       /* the back buffer holds a new rhythm */
static volatile uint8_t pending[8];   /* swap to the back buffer at the next start */
static volatile uint32_t gen_mask;      /* tracks whose machine is a DRUMBRAIN one */
static int8_t style_of[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
static uint32_t rng = 0x2545f491u, ticks;
static uint8_t trk_down, func_eaten;
static int32_t last_sel = -1;
static uint32_t last_sel_tick;
static int32_t msg_track;               /* 0 none, 1-8 a track, 9 all */
static uint32_t msg_until;

static void ask_new(uint32_t tracks)
{
    int t;
    for (t = 0; t < 8; t++)
        if ((tracks >> t) & 1)
            pending[t] = 1;
}

static uint32_t rnd(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}

static int child(int n, int sym)
{
    const struct db_node *d = &db_nodes[n];
    uint32_t m = d->mask, below;
    int k = 0;
    if (!((m >> sym) & 1))
        return -1;
    below = m & ((1u << sym) - 1);
    while (below) { k += below & 1; below >>= 1; }
    return d->first + k;
}

/* one lane of LEN steps: 0 none, 1 hit, 2 strong (5, the flam, plays as strong) */
static void gen_lane(int style, int instr, uint8_t *out)
{
    uint8_t text[48];
    int tl = 0, np = 0, phase = 0, i, n, best, s;
    uint32_t x;
    text[tl++] = 0;                     /* '^' */
    while (np < LEN) {
        if (phase == 4) { text[tl++] = 1; phase = 0; continue; }   /* '-' each beat */
        n = db_roots[style][instr];
        best = db_nodes[n].has ? n : -1;
        for (i = tl - 1; i >= 0 && i >= tl - MAXCTX; i--) {
            n = child(n, text[i]);
            if (n < 0)
                break;
            if (db_nodes[n].has)
                best = n;
        }
        x = (rnd() >> 8) % 255u;
        if (best < 0)
            s = 0;
        else if (x < db_nodes[best].th[0])
            s = 0;
        else if (x < db_nodes[best].th[1])
            s = 1;
        else if (x < db_nodes[best].th[2])
            s = 2;
        else
            s = 3;
        text[tl++] = 2 + s;
        out[np++] = s == 3 ? 2 : s;
        phase++;
    }
}

/* UI task, 30 times a second */
void db_tick(void *ctrl)
{
    uint8_t acc[DB_NSTYLES][LEN], lane[LEN];
    uint32_t m = 0, fresh = 0;
    int t, s, st, v, have_acc = 0, mc;
    rng ^= ++ticks * 2654435761u;
    if (msg_track) {
        if ((int32_t)(msg_until - ticks) < 0)
            msg_track = 0;                      /* gone, and one more frame without it */
        *((unsigned char *)ctrl + 0x20) = 1;
    }
    for (t = 0; t < 8; t++) {
        st = -1;
        if (fw_kit) {
            mc = FW_SOUND(fw_kit, t)[FW_SOUND_MACHINE];
            if (mc >= DB_MACHINE_ID && mc < DB_MACHINE_ID + DB_NSTYLES)
                st = mc - DB_MACHINE_ID;
        }
        if (st >= 0)
            m |= 1u << t;
        if (st != style_of[t]) {
            style_of[t] = st;
            if (st >= 0) {
                ready[t] = 0;           /* a new rhythm in this style, from the next start */
                pending[t] = 1;
                fresh |= 1u << t;
            }
        }
    }
    gen_mask = m;
    for (t = 0; t < 8; t++) {
        st = style_of[t];
        if (st < 0 || ready[t])
            continue;
        if (!((have_acc >> st) & 1)) { gen_lane(st, 0, acc[st]); have_acc |= 1 << st; }
        gen_lane(st, 1 + t, lane);
        for (s = 0; s < LEN; s++) {
            v = lane[s] ? (lane[s] == 1 ? VLO : VHI) + (acc[st][s] ? VACC : 0) : 0;
            buf[front[t] ^ 1][t][s] = v > 127 ? 127 : v;
        }
        ready[t] = 1;
    }
    if (fresh)
        sq_rearm(fresh);
}

/* interrupt level: both reads of a step */
void db_trig(int32_t track, int32_t step, int32_t first, uint32_t *trig)
{
    if (track >= 8 || !((gen_mask >> track) & 1))
        return;
    if (first && step == 0) {
        if (pending[track] && ready[track]) {
            front[track] ^= 1;
            ready[track] = 0;
            pending[track] = 0;
        }
    }
    if (buf[front[track]][track][step & (LEN - 1)])
        *trig |= SQ_TRIG;
    else
        *trig &= ~SQ_TRIG;
}

void db_vel(int32_t track, int32_t step, int32_t *vel)
{
    uint8_t v;
    if (track >= 8 || !((gen_mask >> track) & 1))
        return;
    v = buf[front[track]][track][step & (LEN - 1)];
    if (v)
        *vel = v;
}

/* UI task: [TRK] + the same [TRIG 1-8] twice in a row -> a new rhythm for that track;
 * [TRK] held + [FUNC] -> new rhythms for every empty track. Both at each track's next start. */
int db_key(void *brain, void *ev)
{
    int32_t key = *(int32_t *)((char *)ev + 12);
    int32_t fl = *(int32_t *)((char *)ev + 16);
    int32_t pressed = fl & F_PRESS;
    int32_t t;
    (void)brain;
    if (key == KEY_TRK) {
        trk_down = (fl & (F_PRESS | 8)) ? 1 : 0;
        return 0;
    }
    if (key == KEY_FUNC) {
        if (pressed && !(fl & 8) && trk_down) {
            ask_new(0xff);
            msg_track = 9;
            msg_until = ticks + MSG_TICKS;
            func_eaten = 1;
            return 1;                   /* the firmware does not see this [FUNC] */
        }
        if (func_eaten) {
            if (!pressed)
                func_eaten = 0;         /* its release goes too */
            return 1;
        }
        return 0;
    }
    if (pressed && !(fl & 8) && trk_down && key >= KEY_TRIG1 && key < KEY_TRIG1 + 8) {
        t = key - KEY_TRIG1;
        if (t == last_sel && ticks - last_sel_tick <= DOUBLE_TICKS) {
            ask_new(1u << t);
            msg_track = t + 1;
            msg_until = ticks + MSG_TICKS;
            last_sel = -1;
        } else {
            last_sel = t;
            last_sel_tick = ticks;
        }
    }
    return 0;                           /* the track is selected as stock */
}

void db_draw(void *bmp, void *ctrl)
{
    (void)ctrl;
    if (!msg_track)
        return;
    ((rect_fn)fw_fillrect)(bmp, 78, 54, 127, 63, 0);
    if (msg_track == 9)
        ((text_fn)fw_textf)(bmp, (const void *)fw_font5, 81, 57, -1, "NEW: ALL");
    else
        ((text_fn)fw_textf)(bmp, (const void *)fw_font5, 81, 57, -1, "NEW: T%d", (int)msg_track);
    ((rect_fn)fw_framerect)(bmp, 78, 54, 127, 63, 1);
}

/* ---- the SRC machine: plays as ONESHOT, ONESHOT's page and parameters ------------------------- */
static const struct cm_ui db_page = {
    .abi = cm_ui_v3,
    .page_from = 0,                     /* ONESHOT's page, every knob stock */
};

/* 11 x 7 icon, a word a column, the top row in bit 25: a die showing five */
static const uint32_t db_px[11] = {
    0xfe000000, 0x82000000, 0xaa000000, 0x82000000, 0x92000000, 0x82000000,
    0xaa000000, 0x82000000, 0xfe000000, 0x00000000, 0x00000000,
};
static const uint32_t db_mask[11] = {
    0xfe000000, 0xfe000000, 0xfe000000, 0xfe000000, 0xfe000000, 0xfe000000,
    0xfe000000, 0xfe000000, 0xfe000000, 0xfe000000, 0xfe000000,
};
static const struct {
    const void *vt;
    int32_t width, height, one;
    const uint32_t *px, *mask;
    int32_t zero;
} db_icon = { fw_bitmap_vt, 11, 7, 1, db_px, db_mask, 0 };

const struct cm_machine db_house = {
    DB_MACHINE_ID, "DB HOUSE", "DBHS", &db_icon, 0, 0, CM_UI_TAG, &db_page,
};
const struct cm_machine db_breaks = {
    DB_MACHINE_ID + 1, "DB BREAKS", "DBBR", &db_icon, 0, 0, CM_UI_TAG, &db_page,
};

/* interrupt level: a DRUMBRAIN track ignores its stored steps' conditions and probabilities */
void db_cond(int32_t track, int32_t step, int32_t *pass)
{
    (void)step;
    if (track < 8 && ((gen_mask >> track) & 1))
        *pass = 1;
}
