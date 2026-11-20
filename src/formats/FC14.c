/*
 * FC14.c - Future Composer 1.4 module, "FC14" (upstream r/FC14.c).
 *
 * update 18 mar 2003
 *  - fake test replaced by good test :) -> more FC14 found
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_FC14(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* file size < 193 */
    if ((c->in_size - c->start) < 193)
        return PW_BAD;

    /* get 1st sample addy */
    c->m = ((IN(c->start + 32) * 256 * 256 * 256) +
            (IN(c->start + 33) * 256 * 256) +
            (IN(c->start + 34) * 256) +
            IN(c->start + 35));

    /* test in-size again */
    if (c->start + c->m > (uint32_t)c->in_size)
        return PW_BAD;

    /* test various addresses  */
    c->j = ((IN(c->start + 8) * 256 * 256 * 256) +
            (IN(c->start + 9) * 256 * 256) +
            (IN(c->start + 10) * 256) +
            IN(c->start + 11));

    c->k = ((IN(c->start + 16) * 256 * 256 * 256) +
            (IN(c->start + 17) * 256 * 256) +
            (IN(c->start + 18) * 256) +
            IN(c->start + 19));

    c->l = ((IN(c->start + 36) * 256 * 256 * 256) +
            (IN(c->start + 37) * 256 * 256) +
            (IN(c->start + 38) * 256) +
            IN(c->start + 39));

    /* test in-size again */
    if ((c->j > (uint32_t)c->in_size) || (c->k > (uint32_t)c->in_size) || (c->l > (uint32_t)c->in_size))
        return PW_BAD;

    /* c->l is 1st waveform addy */
    return PW_GOOD;
}

void pw_rip_FC14(pw_ctx *c)
{
    /* c->l is already the 1st waveform addy */

    /* get Waveforms len */
    c->k = 0;
    for (c->j = 100; c->j < 180; c->j++)
    {
        c->k += IN(c->start + c->j);
    }

    c->out_size = c->l + (c->k * 2);

    c->convert = PW_BAD;
    pw_save_rip(c, "Future Composer 1.4 module", PWF_FC14);

    if (c->save_status == PW_GOOD)
        c->i += 4; /* after FC14 tag */
}
