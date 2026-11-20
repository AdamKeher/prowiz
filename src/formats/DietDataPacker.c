/*
 * DietDataPacker.c - Diet (PC) data packer (upstream misc/misc.c:
 *                    testDietDataPacker, Rip_DietDataPacker).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_DietDataPacker(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    c->l = (IN(c->start + 9) * 256 * 256) +
           (IN(c->start + 11) * 256) +
           IN(c->start + 10) + 5 + 12;

    if (c->l > (uint32_t)c->in_size)
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    return PW_GOOD;
    /* c->l is the size of the pack */
}

void pw_rip_DietDataPacker(pw_ctx *c)
{
    /* c->l is still the whole size */
    c->out_size = c->l;

    c->convert = PW_BAD;

    /* the "EOS" variant is saved with the "dlz" ID; the input itself is
     * patched, as the original did */
    if ((IN(c->start + 6) == 0x45) &&
        (IN(c->start + 7) == 0x4F) &&
        (IN(c->start + 8) == 0x53))
    {
        IN_SET(c->start + 6, 0x64);
        IN_SET(c->start + 7, 0x6C);
        IN_SET(c->start + 8, 0x7A);
    }

    pw_save_rip(c, "Diet Packer Data-file", PWF_Diet);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}
