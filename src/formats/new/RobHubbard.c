/*
 * RobHubbard.c - Rob Hubbard (Amiga, 1989-91)
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * There is no file format: a module is the game's own replay code followed
 * by its song data and samples ("RH.", Modland ".rh"). It starts with a jump
 * table of five bra.w, and its samples are the last thing in it, each one
 * stored as [u32 length][u16 frequency][length bytes]; a nop (4E71) closes
 * the module. The checks and the size follow the Wanted Team EagleRipper
 * (spec only, not open source), which Flod's loader confirms.
 *
 * Not recognised: sound-effect banks (four bra.w), tunes with separate
 * samples (Populous and other Delitracker customs), Rob Hubbard ST ("RHO.",
 * Steve Bak's routine) and Hubbard's 1987-88 tunes made with David
 * Whittaker's routine (David Whittaker handler).
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define RH_MAX_SAMPLE 0x10000u   /* the player rejects longer samples */

static int32_t rh_sx16(uint32_t v)
{
    return (v & 0x8000u) ? (int32_t)v - 0x10000 : (int32_t)v;
}

int pw_new_RobHubbard(pw_ctx *c)
{
    const int64_t p = c->i;
    const int64_t in_size = c->in_size;
    int64_t q, smp, e;
    uint32_t k, n = 0, len;
    int32_t d;
    int found;

    /* 68000 code: only at even offsets */
    if (p & 1)
        return 0;

    /* bra.w at 00, 04, 08, 0C and 10, each jumping forward */
    for (k = 0; k < 5; k++)
    {
        if (pw_rd16(c, p + 4 * k) != 0x6000)
            return 0;
    }
    /* lea x(pc),a0 at 14; rts + lea x(pc),a0 at 1C */
    if (pw_rd16(c, p + 0x14) != 0x41FA || pw_rd32(c, p + 0x1C) != 0x4E7541FAu)
        return 0;
    for (k = 0; k < 5; k++)
    {
        d = rh_sx16(pw_rd16(c, p + 4 * k + 2));
        if (d <= 0 || (d & 1))
            return 0;
    }

    /* the number of samples: moveq #n-1,d1 right before the first
     * move.l (a0)+,d2 within 8 words of 40 */
    found = 0;
    for (q = p + 0x40; q < p + 0x50; q += 2)
    {
        if (pw_rd16(c, q) == 0x2418)
        {
            if (pw_rd8(c, q - 2) != 0x72)
                return 0;
            n = pw_rd8(c, q - 1) + 1;
            found = 1;
            break;
        }
    }
    if (!found)
        return 0;

    /* the samples: target of the first lea x(pc),a0 within 5 words of 36,
     * plus the immediate of an adda.l #imm,a0 right after it */
    found = 0;
    for (q = p + 0x36; q < p + 0x40; q += 2)
    {
        if (pw_rd16(c, q) == 0x41FA)
        {
            found = 1;
            break;
        }
    }
    if (!found)
        return 0;
    smp = q + 2 + rh_sx16(pw_rd16(c, q + 2));
    if (pw_rd16(c, q + 4) == 0xD1FC)
    {
        uint32_t add = pw_rd32(c, q + 6);
        if (add > RH_MAX_SAMPLE)
            return 0;
        smp += add;
    }
    /* behind the header code, and word aligned */
    if (smp < p + 0x20 || (smp & 1))
        return 0;

    /* walk the samples: [u32 length][u16 frequency][data]; outside the
     * input the reads give 0, so a truncated module ends up past its end */
    e = smp;
    for (k = 0; k < n; k++)
    {
        len = pw_rd32(c, e);
        if (len > RH_MAX_SAMPLE)
            return 0;
        e += (int64_t)len + 6;
    }
    /* the closing nop, unless the module runs past the input */
    if (e + 2 <= in_size && pw_rd16(c, e) != 0x4E71)
        return 0;

    c->start = (int32_t)p;
    c->out_size = (uint32_t)(e + 2 - p);
    c->convert = PW_BAD;
    pw_save_rip(c, "Rob Hubbard module", PWF_RobHubbard);
    pw_skip_rip(c);
    return 1;
}
