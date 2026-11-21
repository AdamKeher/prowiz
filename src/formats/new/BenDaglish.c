/*
 * BenDaglish.c - Ben Daglish (Colin Dooley's sound player, 1988-92)
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * There is no file format: a module is the game's own position-independent
 * replay code followed by its data and samples ("BD.", Modland ".bd"):
 *
 *   00  bra.w init     04  bra.w play     08  dc.w ?     0A  bra.w sfx
 *   init: move.w d0,-(sp); bsr.w ...; move.w #x,d16(a6); ...; lea x(pc),a0
 *
 * The size comes from the instrument tables. Each table is a list of
 * longword offsets (from the table) to 14-byte instrument records:
 *   00 sample offset  04 loop offset  08 length (words)
 *   0A loop length (words)  0C volume (0-64)
 * and the sample and loop offsets of every record count from the first
 * table. The module ends where the last sample (or loop) ends.
 *
 *  - Table 1 (the music) comes from the init code: the second lea x(pc),a0
 *    after "add.w d0,d0" x 3 + lea. It must fit in the input; when it does
 *    not, the module is reported as truncated.
 *  - Table 2 (sound effects, used by the third entry point) comes from the
 *    first "add.w d0,d0" x 2 + lea x(pc),a0 there. Its samples count only
 *    as far as they fit in the input: some rips (Super Cars) lack the
 *    effects at the end.
 *
 * The checks follow the Wanted Team player and EagleRipper (spec only) and
 * EaglePlayer 2.04's GPL ripper; NostalgicPlayer (MIT) reads the samples
 * the same way.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define BD_MAX_INSTR  256          /* a table holds at most this many records */
#define BD_MAX_OFFSET 0x1000000u   /* sample / loop offsets beyond this are junk */

static int32_t bd_sx16(uint32_t v)
{
    return (v & 0x8000u) ? (int32_t)v - 0x10000 : (int32_t)v;
}

/*
 * The instrument table at 't' (sample offsets count from 'base'). Returns 0
 * if it is not a valid table. Else returns 1 with the largest sample end of
 * all its records in *end, and the largest one that fits in the input (or -1
 * if none does) in *end_in.
 */
static int bd_table(const pw_ctx *c, int64_t t, int64_t base, int64_t *end, int64_t *end_in)
{
    uint32_t mn = pw_rd32(c, t);   /* table length: the smallest offset */
    uint32_t o, so, lo, ln, ll, count = 0;
    int64_t q, r, e;

    *end = -1;
    *end_in = -1;
    if (mn < 4 || mn > 0x10000u)
        return 0;
    for (q = t; q < t + (int64_t)mn; q += 4)
    {
        if (++count > BD_MAX_INSTR)
            return 0;
        o = pw_rd32(c, q);
        if (o < 4 || (o & 1) || o > 0x10000u)
            return 0;
        if (o < mn)
            mn = o;
        r = t + o;
        if (r < 0 || r + 14 > (int64_t)c->in_size)
            return 0;
        so = pw_rd32(c, r);
        lo = pw_rd32(c, r + 4);
        ln = pw_rd16(c, r + 8);
        ll = pw_rd16(c, r + 10);
        if (pw_rd16(c, r + 12) > 64 || so == 0 || so > BD_MAX_OFFSET || lo > BD_MAX_OFFSET)
            return 0;
        e = base + so + 2 * (int64_t)ln;
        if (lo != 0 && base + lo + 2 * (int64_t)ll > e)
            e = base + lo + 2 * (int64_t)ll;
        if (e > *end)
            *end = e;
        if (e <= (int64_t)c->in_size && e > *end_in)
            *end_in = e;
    }
    /* the table ends where the first record starts: a whole number of longs */
    return count > 0 && (mn & 3) == 0;
}

int pw_new_BenDaglish(pw_ctx *c)
{
    const int64_t p = c->i;
    int64_t init, q, t1, t2 = -1, end, end_in, e2, e2_in;
    int32_t d1, d2, d3, d;
    uint32_t k;
    int found;

    /* 68000 code: only at even offsets */
    if (p & 1)
        return 0;

    /* bra.w at 00, 04 and 0A, each jumping forward */
    if (pw_rd16(c, p) != 0x6000 || pw_rd16(c, p + 4) != 0x6000 || pw_rd16(c, p + 10) != 0x6000)
        return 0;
    d1 = bd_sx16(pw_rd16(c, p + 2));
    d2 = bd_sx16(pw_rd16(c, p + 6));
    d3 = bd_sx16(pw_rd16(c, p + 12));
    if (d1 <= 0 || (d1 & 1) || d2 <= 0 || (d2 & 1) || d3 <= 0 || (d3 & 1))
        return 0;

    /* init: move.w d0,-(sp); bsr.w; move.w #x,d16(a6) at +6; lea x(pc),a0 at +C */
    init = p + 2 + d1;
    if (pw_rd32(c, init) != 0x3F006100u || pw_rd16(c, init + 6) != 0x3D7C ||
        pw_rd16(c, init + 12) != 0x41FA)
        return 0;

    /* table 1: add.w d0,d0 x 3 + lea x(pc),a0 (the subsongs), then the
     * next lea x(pc),a0 */
    found = 0;
    q = init;
    for (k = 0; k < 0x100; k++)
    {
        if (pw_rd32(c, q) == 0xD040D040u && pw_rd16(c, q + 4) == 0xD040 &&
            pw_rd16(c, q + 6) == 0x41FA)
        {
            found = 1;
            break;
        }
        q += 2;
    }
    if (!found)
        return 0;
    q += 8;   /* the subsong lea's displacement, where the next search starts */
    found = 0;
    for (k = 0; k < 0x80; k++)
    {
        if (pw_rd16(c, q) == 0x41FA)
        {
            found = 1;
            break;
        }
        q += 2;
    }
    if (!found)
        return 0;
    t1 = q + 2 + bd_sx16(pw_rd16(c, q + 2));
    if (t1 <= p || (t1 & 1))
        return 0;
    if (!bd_table(c, t1, t1, &end, &end_in))
        return 0;

    /* table 2: in the sound effect routine (third entry), the first
     * add.w d0,d0 x 2, if a lea x(pc),a0 follows it */
    q = p + 12 + d3;
    for (k = 0; k < 0x100; k++)
    {
        if (pw_rd32(c, q) == 0xD040D040u)
        {
            q += 4;
            if (pw_rd16(c, q) == 0x41FA)
            {
                d = bd_sx16(pw_rd16(c, q + 2));
                if ((d & 1) == 0 && pw_rd16(c, q + 2 + d) == 0)
                    t2 = q + 2 + d;
                break;
            }
            continue;
        }
        q += 2;
    }
    /* its samples count as far as they fit in the input */
    if (t2 > p && end <= (int64_t)c->in_size && bd_table(c, t2, t1, &e2, &e2_in) &&
        e2_in > end)
        end = e2_in;

    if (end <= t1)
        return 0;
    c->start = (int32_t)p;
    c->out_size = (uint32_t)(end - p);
    c->convert = PW_BAD;
    pw_save_rip(c, "Ben Daglish module", PWF_BenDaglish);
    pw_skip_rip(c);
    return 1;
}
