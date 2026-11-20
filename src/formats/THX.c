/*
 * THX.c - AHX ("THX") and Hively Tracker ("HVL") modules (upstream r/THX.c).
 *
 * THX - AHX ripper
 * 20130413 - slightly adapted to HVL ripping
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_THX(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* file size < 113 */
    if ((c->in_size - c->start) < 30)
        return PW_BAD;

    /* get nbr byte to bypass to reach txts */
    c->m = ((IN(c->start + 4) * 256) + IN(c->start + 5));

    /* test in-size again */
    if (c->start + c->m > (uint32_t)c->in_size)
        return PW_BAD;

    /* test patternlist size */
    c->j = (IN(c->start + 6) & 0x0f) * 256 + IN(c->start + 7);
    /* only 999 position are possible - but I've seen 1024 ... */
    if (c->j > 1024)
        return PW_BAD;

    /* test patternlist restart - AHX only */
    if (IN(c->start) == 'T')
    {
        c->k = (IN(c->start + 8) & 0x0f) * 256 + IN(c->start + 9);
        /* only 999 position are possible - but I've seen 1024 ... */
        if (c->k >= c->j)
            return PW_BAD;
    }

    c->k = IN(c->start + 10);
    /* track size */
    if (c->k > 64)
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_THX(pw_ctx *c)
{
    /* c->m is the address of txt */

    uint32_t where = c->start + c->m;
    uint8_t  smp = 0x00, a = 0x00;
    smp = IN(c->start + 12);

    c->out_size = c->m;

    /* title (stops at the latest on the first byte past the input) */
    if (IN(where) == 0x00)
        c->out_size += 1;
    else
        while (IN(where) != 0x00)
            where += 1;

    where += 1;
    if (smp != 0x00)
    {
        while (a < smp)
        {
            if (where > (uint32_t)c->in_size)
                break;
            if (IN(where) == 0x00)
                a += 0x01;
            where += 1;
        }
    }

    c->out_size = where - c->start;

    c->convert = PW_BAD;

    if (IN(c->start) == 'T')
        pw_save_rip(c, "AHX v1/v2 module", PWF_AHX);
    if (IN(c->start) == 'H')
        pw_save_rip(c, "Hively Tracker module", PWF_HVL);

    if (c->save_status == PW_GOOD)
        c->i += 1; /* after THX/HVL tag */
}
