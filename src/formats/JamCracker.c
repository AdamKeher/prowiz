/*
 * JamCracker.c - JamCracker / Pro module, "BeEp" (upstream r/JamCracker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_JamCracker(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* number of samples */
    c->j = IN(c->start + 5);
    if ((c->j == 0) || (c->j > 0x1f))
        return PW_BAD;

    /* sample sizes */
    /* c->j is the number of sample */
    c->wss = 0;
    for (c->k = 0; c->k < c->j; c->k++)
    {
        c->l = ((IN(c->start + 38 + c->k * 40) * 256 * 256 * 256) +
                (IN(c->start + 39 + c->k * 40) * 256 * 256) +
                (IN(c->start + 40 + c->k * 40) * 256) +
                IN(c->start + 41 + c->k * 40));
        if (c->l == 0)
            return PW_BAD;
        c->wss += c->l;
    }

    /* number of pattern saved */
    /* c->j is the number of sample */
    /* c->wss is the whole sample size :) */
    c->l = IN(c->start + (c->j * 40) + 7);
    /* test if more than FF patterns */
    if (IN(c->start + (c->j * 40) + 6) != 0)
        return PW_BAD;

    /* size of pattern list */
    /* c->j is the number of sample */
    /* c->wss is the whole sample size :) */
    /* c->l is the number of pattern saved */
    c->m = IN(c->start + (c->j * 40) + 9 + (c->l * 6));
    c->n = IN(c->start + (c->j * 40) + 8 + (c->l * 6));
    /* test if more than FF patterns */
    if (c->n != 0)
        return PW_BAD;

    /* c->j is the number of sample */
    /* c->wss is the whole sample size :) */
    /* c->l is the number of pattern saved */
    /* c->m is the size of the pattern list */
    return PW_GOOD;
}

void pw_rip_JamCracker(pw_ctx *c)
{
    /* c->j is the number of sample */
    /* c->wss is the whole sample size :) */
    /* c->l is the number of pattern saved */
    /* c->m is the size of the pattern list */

    /* first, get rid of header size (til end of pattern list) */
    c->n = 6 + (c->j * 40) + 2;
    c->out_size = c->n + (c->l * 6) + 2 + (c->m * 2);

    /* c->n points now at the beginning of pattern descriptions */
    /* now, let's calculate pattern data size */
    /* first address : */
    c->o = ((IN(c->start + c->n + 2) * 256 * 256 * 256) +
            (IN(c->start + c->n + 3) * 256 * 256) +
            (IN(c->start + c->n + 4) * 256) +
            IN(c->start + c->n + 5));
    c->n += (c->l * 6);
    c->k = ((IN(c->start + c->n - 4) * 256 * 256 * 256) +
            (IN(c->start + c->n - 3) * 256 * 256) +
            (IN(c->start + c->n - 2) * 256) +
            IN(c->start + c->n - 1));
    c->k -= c->o;
    /* c->k shoulb be the track data size by now ... save the last pattern ! */
    /* let's get its number of lines */
    c->o = IN(c->start + c->n - 5);
    c->o *= 4; /* 4 voices */
    c->o *= 8; /* 8 bytes per note */

    c->out_size += c->wss + c->k + c->o;

    c->convert = PW_BAD;
    pw_save_rip(c, "JamCracker / Pro module", PWF_JamCracker);

    if (c->save_status == PW_GOOD)
    {
        uint32_t next = c->i + (c->out_size - 2); /* 0 should do but call it "just to be sure" :) */
        /* Hang guard (not in the original): the size is computed from
         * pattern addresses and can wrap to below 2 (or to 2^31 or more), so
         * upstream moves the scan position back, reaches this module again
         * and rips it forever. Keep the position instead; the scan goes on. */
        if (next >= c->i)
            c->i = next;
    }
}
