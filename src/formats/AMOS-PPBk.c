/*
 * AMOS-PPBk.c - AMOS PowerPacker Bank "PPbk" (upstream r/AMOS-PPBk.c).
 * The PP20 subfile is extracted.
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* PW_i += delta. Guard (not in the original): when the size field is tiny
 * the original steps back over bytes it has already scanned, finds the same
 * data again and loops forever. This rip does not change the input, so in
 * that case the scan position is kept instead. */
static void scan_advance(pw_ctx *c, uint32_t delta)
{
    uint32_t next = c->i + delta;

    if ((uint32_t)(next + 1) <= c->i)
        return;
    c->i = next;
}

int16_t pw_test_PPbk(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 12) * 256 * 256 * 256) +
            (IN(c->start + 13) * 256 * 256) +
            (IN(c->start + 14) * 256) +
            IN(c->start + 15));

    if (c->l > (uint32_t)c->in_size)
    {
        return PW_BAD;
    }

    /* PP20 packed size */
    c->k = ((IN(c->start + 20 + c->l) * 256 * 256) +
            (IN(c->start + 21 + c->l) * 256) +
            IN(c->start + 22 + c->l));
    /* PP20 packed size */
    c->m = (((uint32_t)IN(c->start + 8) * 256 * 256 * 256) +
            (IN(c->start + 9) * 256 * 256) +
            (IN(c->start + 10) * 256) +
            IN(c->start + 11));

    if (c->m != c->k)
    {
        return PW_BAD;
    }

    c->l += 8;
    return PW_GOOD;
    /* c->l is the size of the pack (PP20 subfile size !) */
}

void pw_rip_PPbk(pw_ctx *c)
{
    /* c->l is still the whole size */
    c->out_size = c->l;

    c->convert = PW_BAD;

    c->start += 16;
    pw_save_rip(c, "AMOS PowerPacker Bank \"PPbk\" Data-file", PWF_PP20);

    if (c->save_status == PW_GOOD)
        scan_advance(c, c->out_size - 40); /* 36 should do but call it "just to be sure" :) */
}
