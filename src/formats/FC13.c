/*
 * FC13.c - Future Composer 1.3 module, "SMOD" (upstream r/FC13.c).
 *
 * update 18 mar 2003
 *  - fake test replaced by good test :) -> more FC13 found
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_FC13(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* file size < 113 */
    if ((c->in_size - c->start) < 113)
        return PW_BAD;

    /* get addy of 1st sample */
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

    c->l = ((IN(c->start + 24) * 256 * 256 * 256) +
            (IN(c->start + 25) * 256 * 256) +
            (IN(c->start + 26) * 256) +
            IN(c->start + 27));

    /* test in-size again */
    if ((c->j > (uint32_t)c->in_size) || (c->k > (uint32_t)c->in_size) || (c->l > (uint32_t)c->in_size))
        return PW_BAD;

    /* c->m is the addy of the 1st sample */
    return PW_GOOD;
}

void pw_rip_FC13(pw_ctx *c)
{
    /* c->m is the addy of the 1st sample */

    /* whole sample size */
    c->wss = ((IN(c->start + 36) * 256 * 256 * 256) +
              (IN(c->start + 37) * 256 * 256) +
              (IN(c->start + 38) * 256) +
              IN(c->start + 39));

    c->out_size = c->wss + c->m;

    c->convert = PW_BAD;
    pw_save_rip(c, "Future Composer 1.3 module", PWF_FC13);

    if (c->save_status == PW_GOOD)
        c->i += 4; /* after SMOD tag */
}
