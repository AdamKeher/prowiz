/*
 * DM1.c - Delta Music 1 module (upstream r/DM1.c).
 *
 * Delta Music 1 ("shot in the dark" try)
 * 20160309 - Asle
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_DM1(pw_ctx *c)
{
    int32_t t[64], a;
    c->start = (int32_t)c->i;

    memset(t, 0, sizeof t);

    /* file size */
    if ((c->start + 104) > c->in_size)
        return PW_BAD;

    /* get track sizes */
    for (a = 0; a < 4; a++)
    {
        t[a] = ((IN(c->start + 6 + (a * 4)) * 256) +
                IN(c->start + 7 + (a * 4)));
        if (t[a] > 0x7fff)
            return PW_BAD;
    }

    /* get note block size */
    t[4] = ((IN(c->start + 22) * 256) + IN(c->start + 23));

    /* get sounds sizes */
    for (a = 0; a < 20; a++)
    {
        t[a + 40] = ((IN(c->start + 25 + (a * 4)) * 256 * 256) +
                     (IN(c->start + 26 + (a * 4)) * 256) +
                     IN(c->start + 27 + (a * 4)));
        if (t[a + 40] > 0xffff)
            return PW_BAD;
    }

    /* file size */
    c->l = t[0] + t[1] + t[2] + t[3] + t[4] + 104;
    if ((c->start + c->l) > (uint32_t)c->in_size)
    {
        pw_printf(c, "#1,3 (start:%d) (t[0]:%x)(t[1]:%x)(t[2]:%x)(t[3]:%x)(t[4]:%x)\n", c->start,
                  (unsigned)t[0], (unsigned)t[1], (unsigned)t[2], (unsigned)t[3], (unsigned)t[4]);
        return PW_BAD;
    }

    for (a = 40; a < 60; a++)
        c->l += t[a];

    return PW_GOOD;
}

void pw_rip_DM1(pw_ctx *c)
{
    /* c->l contains the whole size */
    c->out_size = c->l;

    c->convert = PW_BAD;
    pw_save_rip(c, "Delta Music 1", PWF_DM1);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}
