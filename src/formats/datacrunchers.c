/*
 * datacrunchers.c - tests for the ArcD, CRND and B9AB data crunchers
 *                   (upstream r/datacrunchers.c). They are ripped by
 *                   pw_rip_special_cruncher_data().
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_ArcDDataCruncher(pw_ctx *c)
{
    c->start = (int32_t)c->i;
    if ((c->start + 12) > c->in_size)
    {
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 8) * 256 * 256 * 256) +
            (IN(c->start + 9) * 256 * 256) +
            (IN(c->start + 10) * 256) +
            IN(c->start + 11));
    /* unpacked size */
    c->k = ((IN(c->start + 5) * 256 * 256) +
            (IN(c->start + 6) * 256) +
            IN(c->start + 7));

    if ((c->k <= 2) || (c->l <= 2))
    {
        return PW_BAD;
    }

    if (c->l > 0xFFFFFF)
    {
        return PW_BAD;
    }

    if (c->k <= c->l)
    {
        return PW_BAD;
    }

    return PW_GOOD;
}

/* reminder : c->l must get the packed size */
int16_t pw_test_CRND(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 4) * 256 * 256 * 256) +
            (IN(c->start + 5) * 256 * 256) +
            (IN(c->start + 6) * 256) +
            IN(c->start + 7));

    if ((c->l + c->i) > (uint32_t)c->in_size)
        return PW_BAD;

    /* the result is ignored; this only sets c->l (and c->k). Note that the
     * unpacked size offset is read from the start of the input. */
    (void)pw_test_special_cruncher_data(c, 4, IN(c->l - 16));

    return PW_GOOD;
}

/* reminder : c->l must get the packed size */
int16_t pw_test_B9AB(pw_ctx *c)
{
    if (c->i < 13)
        return PW_BAD;
    c->start = (int32_t)(c->i - 13);

    /* packed size */
    c->l = (((uint32_t)IN(c->start) * 256 * 256 * 256) +
            (IN(c->start + 1) * 256 * 256) +
            (IN(c->start + 2) * 256) +
            IN(c->start + 3));
    /* unpacked size */
    c->m = (((uint32_t)IN(c->start + 8) * 256 * 256 * 256) +
            (IN(c->start + 9) * 256 * 256) +
            (IN(c->start + 10) * 256) +
            IN(c->start + 11));

    if ((c->l + c->i) > (uint32_t)c->in_size)
        return PW_BAD;
    if (c->l > 0x989689)
        return PW_BAD;
    if (c->m > 0x989689)
        return PW_BAD;
    if ((c->l + c->i) > (uint32_t)c->in_size)
        return PW_BAD;

    return PW_GOOD;
}
