/*
 * sidmon1.c - Sidmon v1 module (upstream r/sidmon1.c).
 *
 * thanks to Markus
 * 20091124 - Asle
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_SIDMON1(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* file size < 4 */
    if ((c->in_size - c->start) < 4)
        return PW_BAD;

    /* get central addy */
    c->m = ((IN(c->start + 2) * 256) + IN(c->start + 3));

    /* test in-size again */
    if (c->start + c->m + 2 > (uint32_t)c->in_size)
        return PW_BAD;

    /* test some bytes */
    if ((IN(c->start + 6) != 0xFF) || (IN(c->start + 7) != 0xD4))
        return PW_BAD;

    /* get tech addys */
    c->n = ((IN(c->start + c->m - 2 + 2) * 256) + /* smps desc */
            IN(c->start + c->m - 1 + 2));
    c->o = ((IN(c->start + c->m - 6 + 2) * 256) + /* patlist addy */
            IN(c->start + c->m - 5 + 2));
    c->j = ((IN(c->start + c->m - 10 + 2) * 256) + /* pat data addy */
            IN(c->start + c->m - 9 + 2));

    /* test in-size again */
    if ((c->start + c->m + 2 + c->n + 4 > (uint32_t)c->in_size) ||
        (c->start + c->m + 2 + c->o + 4 > (uint32_t)c->in_size))
        return PW_BAD;

    /* check both adresses consistency */
    if ((c->n <= c->o) && (c->n != 1))
        return PW_BAD;

    /* no sample - then check only patlist at the end */
    if (c->n == 1)
    {
        c->k = c->start + c->m + 2 + c->o;
        if ((c->k + 4) > (uint32_t)c->in_size) /* one pattern ? odd but ... */
            return PW_GOOD;
        c->k += 4;
        c->l = 1;
        c->n = c->o - c->j;
        while (c->l < c->n)
        {
            c->l = ((IN(c->k) * 256 * 256 * 256) +
                    (IN(c->k + 1) * 256 * 256) +
                    (IN(c->k + 2) * 256) +
                    IN(c->k + 3));
            if (((c->k + 4) > (uint32_t)c->in_size) || (c->l == 0)) /* end of file, so end of patlist .. or 0 */
                return PW_GOOD;
            c->k += 4;
        }
        c->k -= 4;
    }
    else /* there are some samples */
    {
        c->n = c->start + c->m + 2 + c->n;
        c->m = (IN(c->n + 2) * 256) + IN(c->n + 3);
        if (((c->n + c->m) > (uint32_t)c->in_size) || ((c->m % 32) != 0))
            return PW_BAD;
        if (c->m == 0)
        { /* weird hack ?!? try to guess the smp desc size */
            for (;; c->m += 32)
            {
                if (c->m + 4 + 32 > (uint32_t)c->in_size)
                    return PW_BAD;
                c->j = ((IN(c->n + c->m + 4) * 256 * 256 * 256) +
                        (IN(c->n + c->m + 5) * 256 * 256) +
                        (IN(c->n + c->m + 6) * 256) +
                        IN(c->n + c->m + 7));
                c->k = ((IN(c->n + c->m + 8) * 256 * 256 * 256) +
                        (IN(c->n + c->m + 9) * 256 * 256) +
                        (IN(c->n + c->m + 10) * 256) +
                        IN(c->n + c->m + 11));
                c->l = ((IN(c->n + c->m + 12) * 256 * 256 * 256) +
                        (IN(c->n + c->m + 13) * 256 * 256) +
                        (IN(c->n + c->m + 14) * 256) +
                        IN(c->n + c->m + 15));
                if ((c->j > c->k) || (c->k >= c->l))
                    break;
            }
        }
        /* get biggest end addy of samples */
        c->o = 0;
        for (c->j = 0; c->j < (c->m / 32); c->j += 1)
        {
            c->k = ((IN(c->n + 14 + c->j * 32) * 256) + IN(c->n + 15 + c->j * 32));
            if (c->k > c->o)
                c->o = c->k;
        }
        c->n += (c->m + c->o + 4);
    }

    return PW_GOOD;
}

void pw_rip_SIDMON1(pw_ctx *c)
{
    c->out_size = c->n - c->start;

    c->convert = PW_BAD;
    pw_save_rip(c, "Sidmon v1 module", PWF_Sidmon1);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}
