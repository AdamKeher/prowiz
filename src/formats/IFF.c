/*
 * IFF.c - EA-IFF "FORM....ILBM" picture (upstream r/IFF.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_IFF(pw_ctx *c)
{
    uint32_t guard = 0;

    c->start = (int32_t)c->i;

    if (c->start + 20 > c->in_size)
        return PW_BAD;

    /* ILBM -> picture */
    if ((IN(c->start + 8) != 'I') ||
        (IN(c->start + 9) != 'L') ||
        (IN(c->start + 10) != 'B') ||
        (IN(c->start + 11) != 'M'))
        return PW_BAD;

    c->m = 16;
    while (1)
    {
        /* Hang guard (not in the original). The walk only depends on c->m
         * and every hunk size is read inside the input, so once it took more
         * steps than there are input positions it is in a cycle (e.g. a hunk
         * size of 0xFFFFFFF8 leads back to itself): upstream loops forever
         * here. Treat it as "not an IFF file" and let the scan go on. */
        if (++guard > (uint32_t)c->in_size + 2)
            return PW_BAD;

        /* size of hunk */
        c->l = ((IN(c->start + c->m) * 256 * 256 * 256) +
                (IN(c->start + c->m + 1) * 256 * 256) +
                (IN(c->start + c->m + 2) * 256) +
                IN(c->start + c->m + 3));
        if (((c->l / 2) * 2) != c->l)
            c->l += 1;

        c->m += 4;
        c->m += c->l;

        if ((c->start + c->m + 8 > (uint32_t)c->in_size) || (c->l == 0))
            return PW_BAD;

        if ((IN(c->start + c->m) == 'B') &&
            (IN(c->start + c->m + 1) == 'O') &&
            (IN(c->start + c->m + 2) == 'D') &&
            (IN(c->start + c->m + 3) == 'Y'))
            break;

        c->m += 4;
    }

    c->k = ((IN(c->start + c->m + 4) * 256 * 256 * 256) +
            (IN(c->start + c->m + 5) * 256 * 256) +
            (IN(c->start + c->m + 6) * 256) +
            IN(c->start + c->m + 7));
    c->l = c->m + 8 + c->k;

    return PW_GOOD;
    /* c->l is the size of the picture */
}

void pw_rip_IFF(pw_ctx *c)
{
    /* c->l is still the whole size */

    c->out_size = c->l;

    c->convert = PW_BAD;

    pw_save_rip(c, "IFF graphic", PWF_IFF);

    if (c->save_status == PW_GOOD)
        c->i += 4;
}
