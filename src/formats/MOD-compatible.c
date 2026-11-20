/*
 * MOD-compatible.c - Protracker/Startrekker/FastTracker style modules with
 *                    any number of channels (upstream r/MOD-compatible.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "formats.h"

/* testMOD(): called with the scan position on the "M.K."-style ID */
int16_t pw_test_MOD(pw_ctx *c, int nbrchannels)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1080);
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        /* size */
        c->j = (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2);
        /* loop size */
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);

        if (pw_test_smps((int32_t)(c->j * 2), (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 45 + 30 * c->k), IN(c->start + 44 + 30 * c->k)) == PW_BAD)
            return PW_BAD;

        c->wss += c->j;
    }

    /* test #4  pattern list size */
    c->l = IN(c->start + 950);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;
    /* c->l holds the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 952 + c->j) > c->k)
            c->k = IN(c->start + 952 + c->j);
        if (IN(c->start + 952 + c->j) > 127)
            return PW_BAD;
    }
    /* c->k holds the highest pattern number */
    /* test last patterns of the pattern list = 0 ? */
    c->j += 2; /* found some obscure ptk :( */
    while (c->j < 128)
    {
        if (IN(c->start + 952 + c->j) > 0x7f)
            return PW_BAD;
        c->j += 1;
    }
    /* c->k is the number of pattern in the file (-1) */
    c->k += 1;

    /* test #5 pattern data ... */
    if (((c->k * 256 * nbrchannels) + 1084 + c->start) > (uint32_t)c->in_size)
        return PW_BAD;
    for (c->j = 0; c->j < (c->k * 256); c->j++)
    {
        /* sample > 1f   or   pitch > 358 ? */
        if (IN(c->start + 1084 + c->j * nbrchannels) > 0x13)
            return PW_BAD;
        c->l = ((IN(c->start + 1084 + c->j * nbrchannels) & 0x0f) * 256) +
               IN(c->start + 1085 + c->j * nbrchannels);
        if ((c->l > 0) && (c->l < 0x1C))
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_MOD(pw_ctx *c, int nbrchannels)
{
    char whatever[64];
    memset(whatever, 0, sizeof whatever);
    /* c->k is still the nbr of pattern */
    /* c->wss is still the whole sample size */

    c->out_size = c->wss + (c->k * 256 * nbrchannels) + 1084;

    c->convert = PW_BAD;
    snprintf(whatever, sizeof whatever, "Pro|Star|whatever tracker %d channels module", nbrchannels);
    pw_save_rip(c, whatever, PWF_Protracker);

    if (c->save_status == PW_GOOD)
        c->i += 1084;
}
