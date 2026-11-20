/*
 * MED-Octamed.c - MED (MMD0) and OctaMED (MMD1) modules (upstream r/MED-Octamed.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* valid for MMD0 & MMD1 */
int16_t pw_test_MMD0(pw_ctx *c)
{
    c->start = (int32_t)c->i;
    if ((c->start + 52) > c->in_size)
        return PW_BAD;

    /* get the 'should be' module size */
    c->k = ((IN(c->start + 4) * 256 * 256 * 256) +
            (IN(c->start + 5) * 256 * 256) +
            (IN(c->start + 6) * 256) +
            IN(c->start + 7));

    /* 52 : size of header */
    if (c->k < 52)
        return PW_BAD;

    /* test flag byte : 1 or 0 */
    c->j = IN(c->start + 20);
    if (c->j > 1)
        return PW_BAD;

    /* get struct MMD* addy */
    c->j = ((IN(c->start + 8) * 256 * 256 * 256) +
            (IN(c->start + 9) * 256 * 256) +
            (IN(c->start + 10) * 256) +
            IN(c->start + 11));
    if ((c->j < 52) || (c->j > c->k))
        return PW_BAD;

    /* test 'reserved' bytes which should be set to 0x00 */
    if ((IN(c->start + 21) != 0x00) ||
        (IN(c->start + 22) != 0x00) ||
        (IN(c->start + 23) != 0x00))
        return PW_BAD;

    /* stop it for now ... few/cheap tests here, I agree .. */
    /* c->k is the module size */

    return PW_GOOD;
}

void pw_rip_MMD0(pw_ctx *c)
{
    /* c->k is the module size */

    c->out_size = c->k;

    c->convert = PW_BAD;
    if (IN(c->i + 3) == '0')
        pw_save_rip(c, "MED (MMD0) module", PWF_MED);
    if (IN(c->i + 3) == '1')
        pw_save_rip(c, "OctaMED (MMD1) module", PWF_MED);

    if (c->save_status == PW_GOOD)
    {
        uint32_t next = c->i + (c->out_size - 2); /* 0 should do but call it "just to be sure" :) */
        /* Hang guard (not in the original): a module size of 2^31 or more
         * is still "saved" (it is negative as int32) and wraps the step, so
         * upstream moves the scan position back, reaches this module again
         * and rips it forever. Keep the position instead; the scan goes on. */
        if (next >= c->i)
            c->i = next;
    }
}
