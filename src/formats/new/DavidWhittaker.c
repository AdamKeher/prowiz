/*
 * DavidWhittaker.c - David Whittaker (1987-94; also Rob Hubbard's 1987-88
 *                    Amiga tunes, J.C. Brooke and Paul Robotham)
 *                  (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * There is no file format: a module ("DW.", Modland ".dw") is the game's own
 * replay code followed by its data and samples, with many player variants.
 *
 * Start: one of three code prologues (they cover 119 of the 121 Modland
 * files; the Wanted Team player accepts the same three):
 *   A  48E7 F1FE 6100 xxxx  movem.l d0-d3/d7/a0-a6,-(sp); bsr.w init, where
 *                           init has a lea x(pc),a3 (47FA) in its first
 *                           10 words
 *   B  08F9 0001 00BF E001  bset #1,$bfe001, then at 08 or 0A a
 *                           lea x(pc),a3 whose target is at or before 00
 *   C  47FA dddd            lea x(pc),a3 pointing at most 8 bytes back,
 *                           i.e. at the module start (moveq #0,d0 or a
 *                           few such words may come first)
 * Some variants keep their variables before the code: lea x(pc),a3 points
 * up to a few KB before the first instruction (Bubble Bobble, Grimblood,
 * Krusty's Super Fun House). That area is not part of the module (zeros on
 * disk, live variables in memory): the rip starts at the prologue, like the
 * existing rips. A module that starts with such a lea (no A/B prologue,
 * e.g. Grimblood) is taken when the lea is followed by bsr.w or tst.b and
 * does not point at an A or B prologue (it would be the init routine of a
 * module found there).
 *
 * Confirmation, as in NostalgicPlayer (MIT): the sample set-up code within
 * the first 16 KB of the module, before any sample data,
 *   41FA dddd [D1FC imm32 | 2748 xxxx D0FC imm16 xxxx xxxx D0FC imm16]
 *   4BFA dddd 72nn         lea samples(pc),a0 [+ adds]; lea info(pc),a5;
 *                          moveq #n-1,d1
 * followed by 2418 3618 (move.l (a0)+,d2; move.w (a0)+,d3: the music
 * samples) or 3428 0002 C4FC 0032 (a second set, used by sound effects).
 * The first music set-up whose samples check out is the module's.
 *
 * Samples: n records [u32 length][u16 frequency][length bytes] in a row.
 *
 * Size rule:
 *  1. the end of the music samples;
 *  2. or the end of a second sample set (sound effects, or more music
 *     samples) that starts at or after that end and lies entirely in the
 *     input, if it ends later; Modland rips that include the effects stop
 *     exactly there;
 *  3. when the end comes from the music samples (1), plus 64 bytes if the
 *     64 bytes there are zero (85 of the 121 Modland rips end that way;
 *     ExoticRipper's notes say the players often need them).
 * Some rips keep more data than this: a ~3.8 KB block of waveforms that
 * sits between the music samples and the effects, without the effects
 * (Fire & Brimstone, Gazza 2), or unknown data (Krusty, Snow Strike).
 *
 * Not recognised: the 1987 player (Feud, Leviathan and Xenon sound effects,
 * Rob Hubbard's QBall) and modules with sound effects only (Anarchy, Blood
 * Money, Spellbound). A module whose own set-up is not found would take the
 * set-up of a module that follows within 16 KB; no such case is known.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define DW_SCAN       0x4000     /* the set-ups are in the first 16 KB of code */
#define DW_MAX_SETUPS 16
#define DW_MAX_SAMPLE 0x20000u   /* length / 2 must fit in a word */
#define DW_MAX_ADD    0x100000u  /* adda.l to a second sample set */
#define DW_PAD        64         /* zero bytes after the music samples */

typedef struct
{
    int64_t  at;     /* the lea samples(pc),a0 */
    int64_t  smp;    /* the first sample record */
    uint32_t n;      /* number of samples */
    int      music;  /* 2418 3618 (music) or 3428 0002 C4FC 0032 */
} dw_setup;

static int32_t dw_sx16(uint32_t v)
{
    return (v & 0x8000u) ? (int32_t)v - 0x10000 : (int32_t)v;
}

/* the prologue at 'p' (even); sets *start */
static int dw_prologue(const pw_ctx *c, int64_t p, int64_t *start)
{
    uint32_t first = pw_rd32(c, p);
    int64_t t;
    int k;

    if (first == 0x48E7F1FEu)
    {
        /* A: bsr.w init; init has lea x(pc),a3 within 10 words */
        if (pw_rd16(c, p + 4) != 0x6100)
            return 0;
        t = p + 6 + dw_sx16(pw_rd16(c, p + 6));
        for (k = 0; k < 10; k++)
        {
            if (pw_rd16(c, t + 2 * k) == 0x47FA)
            {
                *start = p;
                return 1;
            }
        }
        return 0;
    }
    if (first == 0x08F90001u)
    {
        /* B: bset #1,$bfe001; lea x(pc),a3 at 08 or 0A, pointing back */
        if (pw_rd32(c, p + 4) != 0x00BFE001u)
            return 0;
        for (k = 8; k <= 10; k += 2)
        {
            if (pw_rd16(c, p + k) == 0x47FA &&
                p + k + 2 + dw_sx16(pw_rd16(c, p + k + 2)) <= p)
            {
                *start = p;
                return 1;
            }
        }
        return 0;
    }
    if ((first >> 16) == 0x47FA)
    {
        uint32_t next = pw_rd16(c, p + 4);

        t = p + 2 + dw_sx16(first & 0xFFFF);
        if (t & 1)
            return 0;
        if (t <= p && t >= p - 8)
        {
            /* C: lea start(pc),a3 */
            if (t < 0)
                return 0;
            *start = t;
            return 1;
        }
        if (t < p - 8 && (next == 0x6100 || next == 0x4A2B))
        {
            /* variables before the code; not the init of a module at 't' */
            first = pw_rd32(c, t);
            if (first == 0x48E7F1FEu || first == 0x08F90001u)
                return 0;
            *start = p;
            return 1;
        }
    }
    return 0;
}

/* the sample set-ups in the code: from 'start', at most DW_SCAN bytes and
 * never past the first sample data seen (code comes before data) */
static uint32_t dw_setups(const pw_ctx *c, int64_t start, dw_setup *out)
{
    int64_t lim = start + DW_SCAN;
    int64_t q, s, k;
    uint32_t n = 0, add;
    int music;

    if (lim > (int64_t)c->in_size)
        lim = c->in_size;
    for (q = start; q + 2 <= lim && n < DW_MAX_SETUPS; q += 2)
    {
        if (pw_rd16(c, q) != 0x41FA)
            continue;
        s = q + 2 + dw_sx16(pw_rd16(c, q + 2));
        k = q + 4;
        if (pw_rd16(c, k) == 0xD1FC)
        {
            /* adda.l #imm,a0 */
            add = pw_rd32(c, k + 2);
            if (add > DW_MAX_ADD)
                continue;
            s += add;
            k += 6;
        }
        else if (pw_rd16(c, k) == 0x2748 && pw_rd16(c, k + 4) == 0xD0FC &&
                 pw_rd16(c, k + 12) == 0xD0FC)
        {
            /* move.l a0,x(a3); adda.w #imm,a0 (twice) */
            s += (int64_t)pw_rd16(c, k + 6) + pw_rd16(c, k + 14);
            k += 16;
        }
        if (pw_rd16(c, k) != 0x4BFA || pw_rd8(c, k + 4) != 0x72)
            continue;
        if (pw_rd16(c, k + 6) == 0x2418 && pw_rd16(c, k + 8) == 0x3618)
            music = 1;
        else if (pw_rd32(c, k + 6) == 0x34280002u && pw_rd32(c, k + 10) == 0xC4FC0032u)
            music = 0;
        else
            continue;
        out[n].at = q;
        out[n].smp = s;
        out[n].n = pw_rd8(c, k + 5) + 1;
        out[n].music = music;
        n++;
        if (s > q && s < lim)
            lim = s;
    }
    return n;
}

/*
 * The sample records of a set-up. Returns the end of the last sample, or -1
 * if a record is not a sample. A set running past the input sets *trunc and
 * returns an end past the input.
 */
static int64_t dw_walk(const pw_ctx *c, const dw_setup *su, int *trunc)
{
    int64_t e = su->smp;
    uint32_t k, len, freq;

    *trunc = 0;
    for (k = 0; k < su->n; k++)
    {
        if (e + 6 > (int64_t)c->in_size)
        {
            *trunc = 1;
            return e + 6;
        }
        len = pw_rd32(c, e);
        freq = pw_rd16(c, e + 4);
        if (len > DW_MAX_SAMPLE || freq < 1000 || freq > 60000)
            return -1;
        e += (int64_t)len + 6;
    }
    if (e > (int64_t)c->in_size)
        *trunc = 1;
    return e;
}

int pw_new_DavidWhittaker(pw_ctx *c)
{
    const int64_t p = c->i;
    dw_setup su[DW_MAX_SETUPS];
    int64_t start, end = -1, e;
    uint32_t n, k, m = 0;
    int trunc = 0, t2, from_music = 1;

    /* 68000 code: only at even offsets */
    if (p & 1)
        return 0;
    if (!dw_prologue(c, p, &start))
        return 0;

    n = dw_setups(c, start, su);

    /* the music samples: the first music set-up whose records check out */
    for (k = 0; k < n; k++)
    {
        if (!su[k].music || su[k].smp <= su[k].at || (su[k].smp & 1) || su[k].smp < start + 16)
            continue;
        end = dw_walk(c, &su[k], &trunc);
        if (end >= 0)
        {
            m = k;
            break;
        }
    }
    if (end < 0)
        return 0;

    if (!trunc)
    {
        /* a second set behind the music samples, entirely in the input */
        int64_t music_end = end;
        for (k = 0; k < n; k++)
        {
            if (k == m || su[k].smp < music_end)
                continue;
            e = dw_walk(c, &su[k], &t2);
            if (e > end && !t2)
            {
                end = e;
                from_music = 0;
            }
        }
        /* the 64 zero bytes after the music samples */
        if (from_music && end + DW_PAD <= (int64_t)c->in_size)
        {
            for (k = 0; k < DW_PAD; k++)
            {
                if (pw_rd8(c, end + k) != 0)
                    break;
            }
            if (k == DW_PAD)
                end += DW_PAD;
        }
    }

    c->start = (int32_t)start;
    c->out_size = (uint32_t)(end - start);
    c->convert = PW_BAD;
    pw_save_rip(c, "David Whittaker module", PWF_DavidWhittaker);
    pw_skip_rip(c);
    return 1;
}
