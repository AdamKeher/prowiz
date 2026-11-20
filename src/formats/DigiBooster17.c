/*
 * DigiBooster17.c - DigiBooster 1.7 module (upstream r/DigiBooster17.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_DigiBooster17(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* get nbr of pattern saved */
    c->m = IN(c->start + 46); /* this value is -1 ! */

    /* test if there are pattern in pattern list > number of pattern saved */
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 48 + c->j) > c->m)
            return PW_BAD;
    }

    /* samples > $FFFFFF ) */
    /* c->m is the number of pattern saved -1 */
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
    {
        c->o = ((IN(c->start + 176 + (c->j * 4)) * 256 * 256 * 256) +
                (IN(c->start + 177 + (c->j * 4)) * 256 * 256) +
                (IN(c->start + 178 + (c->j * 4)) * 256) +
                IN(c->start + 179 + (c->j * 4)));
        if (c->o > 0xFFFFFF)
            return PW_BAD;
        /* volumes */
        if (IN(c->start + 548 + c->j) > 0x40)
            return PW_BAD;
        c->wss += c->o;
    }

    /* pattern sizes */
    /* c->m is the number of pattern saved -1 */
    /* c->wss is the whole sample size :) */
    c->k = 1572; /* first pattern addy */
    for (c->j = 0; c->j <= c->m; c->j++)
    {
        c->o = (IN(c->start + c->k) * 256) + IN(c->start + c->k + 1);
        /* size < 64 ? */
        if (c->o < 64)
            return PW_BAD;
        c->k += c->o + 2;
    }

    /* c->m is the number of pattern saved -1 */
    /* c->wss is the whole sample size :) */
    /* c->k is the module size saves the samples data */

    return PW_GOOD;
}

void pw_rip_DigiBooster17(pw_ctx *c)
{
    /* c->m is the number of pattern saved -1 */
    /* c->wss is the whole sample size :) */
    /* c->k is the module size saves the samples data */

    c->out_size = c->wss + c->k;

    c->convert = PW_BAD;
    pw_save_rip(c, "DigiBooster 1.7 module", PWF_DigiBooster);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 2); /* 0 should do but call it "just to be sure" :) */
}
