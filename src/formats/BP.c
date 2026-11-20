/*
 * BP.c - SoundMonitor v2 / v3 module, "V.2" / "V.3" (upstream r/BP.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_BP(pw_ctx *c)
{
    /* test 1 (upstream tests c->start before setting it: the value an
     * earlier handler left) */
    if ((c->i < 26) || ((c->start + 512) > c->in_size))
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 26);
    for (c->j = 0; c->j < 15; c->j++)
    {
        if (IN(32 + c->j * 32 + c->start) == 0xff)
            continue;
        if (IN(c->j * 32 + 63 + c->start) > 0x40)
            return PW_BAD;
    }

    /* various shits to calculate the size */
    c->wss = 0;
    for (c->k = 0; c->k < 15; c->k++)
    {
        if (IN(c->start + 32 + 32 * c->k) != 0xff)
            c->wss += (((IN(c->start + 56 + 32 * c->k) * 256) + IN(c->start + 57 + 32 * c->k)) * 2);
    }
    c->j = IN(c->start + 29);
    c->l = IN(c->start + 30) * 256 + IN(c->start + 31);
    c->out_size = c->wss + (c->j * 64);
    c->j = 0;
    if ((c->start + 525 + (c->l * 16)) > (uint32_t)c->in_size)
        return PW_BAD;

    for (c->k = 0; c->k < c->l; c->k++)
    {
        if ((uint32_t)(IN(c->start + 512 + c->k * 16) * 256 + IN(c->start + 513 + c->k * 16)) > c->j)
            c->j = (IN(c->start + 512 + c->k * 16) * 256 + IN(c->start + 513 + c->k * 16));
        if ((uint32_t)(IN(c->start + 516 + c->k * 16) * 256 + IN(c->start + 517 + c->k * 16)) > c->j)
            c->j = (IN(c->start + 516 + c->k * 16) * 256 + IN(c->start + 517 + c->k * 16));
        if ((uint32_t)(IN(c->start + 520 + c->k * 16) * 256 + IN(c->start + 521 + c->k * 16)) > c->j)
            c->j = (IN(c->start + 520 + c->k * 16) * 256 + IN(c->start + 521 + c->k * 16));
        if ((uint32_t)(IN(c->start + 524 + c->k * 16) * 256 + IN(c->start + 525 + c->k * 16)) > c->j)
            c->j = (IN(c->start + 524 + c->k * 16) * 256 + IN(c->start + 525 + c->k * 16));
    }

    return PW_GOOD;
}

void pw_rip_BP(pw_ctx *c)
{
    c->out_size += ((c->j * 48) + (c->l * 16) + 512);

    c->convert = PW_BAD;
    pw_save_rip(c, "Sound Monitor v2 / v3 module", PWF_SoundMonitor);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 27); /* 26 should do but call it "just to be sure" :) */
}
