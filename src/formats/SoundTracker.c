/*
 * SoundTracker.c - SoundTracker 15 samples module (upstream r/SoundTracker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/*
 * 20100902: removed *2 for loop size and loop start. was too restrictive in some case.
 */
int16_t pw_test_SoundTracker(pw_ctx *c)
{
    /* test 1 */
    /* start of stk before start of file ? */
    if ((c->i < 45) || ((c->i + 555) > (uint32_t)c->in_size))
        return PW_BAD;

    /* test 2 */
    /* samples tests */
    c->start = (int32_t)(c->i - 45);
    c->wss = 0;
    for (c->k = 0; c->k < 15; c->k++)
    {
        /* size */
        c->j = (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)));
        /* loop size */
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)));
        /* all sample sizes */
        c->wss += c->j;

        /* size,loopstart,replen > 64k ? */
        if ((c->j > 0xFFFF) || (c->m > 0xFFFF) || (c->n > 0xFFFF))
            return PW_BAD;
        /* replen > size ? */
        if (c->n > (c->j + 2))
            return PW_BAD;
        /* loop start > size ? */
        if ((c->m >= c->j) && (c->j > 0))
            return PW_BAD;
        /* loop size =0 & loop start != 0 ?*/
        if ((c->m != 0) && (c->n == 0))
            return PW_BAD;
        /* size & loopstart !=0 & size=loopstart ? */
        if ((c->j != 0) && (c->j == c->m))
            return PW_BAD;
        /* size =0 & loop start !=0 */
        if ((c->j == 0) && (c->m != 0))
            return PW_BAD;
    }
    /* all sample sizes < 8 ? */
    if (c->wss < 8)
        return PW_BAD;

    /* test #3  finetunes & volumes */
    for (c->k = 0; c->k < 15; c->k++)
    {
        if ((IN(c->start + 44 + c->k * 30) > 0x0f) || (IN(c->start + 45 + c->k * 30) > 0x40))
            return PW_BAD;
    }

    /* test #4  pattern list size */
    c->l = IN(c->start + 470);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;
    /* c->l holds the size of the pattern list */
    /* but let's ignore it as some STK have some pattern after the official size */
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 472 + c->j) > c->k)
            c->k = IN(c->start + 472 + c->j);
        if (IN(c->start + 472 + c->j) > 127)
            return PW_BAD;
    }
    /* c->k holds the highest pattern number */
    /* c->k is the number of pattern in the file (-1) */
    c->k += 1;

    /* test #5 pattern data ... */
    if (((c->k * 1024) + 600 + c->start) > (uint32_t)c->in_size)
        return PW_BAD;
    for (c->j = 0; c->j < (c->k * 256); c->j++)
    {
        /* sample > 1f ? */
        if ((IN(c->start + 600 + c->j * 4) & 0xf0) > 0x10)
            return PW_BAD;
        if ((IN(c->start + 600 + c->j * 4) & 0x0f) > 0x03)
            return PW_BAD;
        c->l = ((IN(c->start + 600 + c->j * 4) & 0x0f) * 256) + IN(c->start + 601 + c->j * 4);
        if ((c->l > 0) && (c->l < 0x71))
            return PW_BAD;
        /* 20120930 - additional test - there's a note but no sample ref */
        if (((IN(c->start + 600 + c->j * 4) > 0) || (IN(c->start + 601 + c->j * 4) > 0)) &&
            ((IN(c->start + 602 + c->j * 4) & 0xf0) == 0))
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_SoundTracker(pw_ctx *c)
{
    /* c->k is still the nbr of pattern */
    /* c->wss is still the whole sample size */

    c->out_size = c->wss + (c->k * 1024) + 600;

    c->convert = PW_BAD;
    pw_save_rip(c, "SoundTracker module", PWF_SoundTracker);

    if (c->save_status == PW_GOOD)
        c->i += 46; /* after 1st volume */
}
