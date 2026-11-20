/*
 * FT2.c - Fasttracker 2 XM module (upstream r/FT2.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_XM(pw_ctx *c)
{
    /* test #1 */
    c->start = (int32_t)c->i;
    if ((c->start + 336) > c->in_size)
        return PW_BAD;

    if ((IN(c->start + 37) != 0x1A) || (IN(c->start + 58) != 0x04))
        return PW_BAD;

    /* get header siz */
    c->l = ((IN(c->start + 63) * 256 * 256 * 256) +
            (IN(c->start + 62) * 256 * 256) +
            (IN(c->start + 61) * 256) +
            IN(c->start + 60));
    /* get number of patterns */
    c->j = (IN(c->start + 71) * 256) + IN(c->start + 70);
    /* get number of instruments */
    c->k = (IN(c->start + 73) * 256) + IN(c->start + 72);

    if ((c->j > 256) || (c->k > 128))
        return PW_BAD;

    c->l += 60; /* points now on the first pattern data */

    /* c->l is the header size and points on the first pattern */
    /* c->j is the number of pattern */
    /* c->k is the number of instruments */
    return PW_GOOD;
}

void pw_rip_XM(pw_ctx *c)
{
    /* c->l is the header size and points on the first pattern */
    /* c->j is the number of pattern */
    /* c->k is the number of instruments */

    /* get whole pattern data siz */
    for (c->o = 0; c->o < c->j; c->o++)
    {
        /* getting siz of one pattern */
        c->m = (IN(c->start + c->l + 8) * 256) + IN(c->start + c->l + 7);
        /* adding it to current pointer + 9 being the pat header siz */
        c->l += (c->m + 9);
    }

    /* c->l is now on the first inst .. c->j is free :)*/
    /* get whole insts data siz */
    for (c->o = 0; c->o < c->k; c->o++)
    {
        int32_t siz = 0;
        /* getting siz of one inst header */
        c->m = (IN(c->start + c->l + 1) * 256) + IN(c->start + c->l);
        /* getting nbr of samples in this inst */
        c->j = (IN(c->start + c->l + 28) * 256) + IN(c->start + c->l + 27);
        /* getting sizes of samples */
        if (c->j > 16)
            break;
        c->l += c->m; /* so that it points on first sample header */
        for (c->n = 0; c->n < c->j; c->n++)
        {
            /* siz of this samples */
            siz += ((IN(c->start + c->l + 3) * 256 * 256 * 256) +
                    (IN(c->start + c->l + 2) * 256 * 256) +
                    (IN(c->start + c->l + 1) * 256) +
                    IN(c->start + c->l));
            /* move pointer onto the next sample header if one exists*/
            c->l += 40;
        }
        /* add sample datas of this instrument now */
        c->l += siz;
    }

    c->out_size = c->l;

    c->convert = PW_BAD;
    pw_save_rip(c, "Fastracker (XM) 2.0 module", PWF_XM);

    if (c->save_status == PW_GOOD)
        c->i += 2;
}
