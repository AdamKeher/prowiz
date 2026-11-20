/*
 * BSI-FutureComposer.c - BSI Future Composer module, "FUCO" (upstream
 * r/BSI-FutureComposer.c).
 *
 * Note: the first test compares the scan position with itself + 18424, so it
 * always fails and the original never reports this format. Kept as is.
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_BSIFutureComposer(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* file size < 18424 */
    if (pw_test_1_start(c, (uint32_t)(c->start + 18424)))
        return PW_BAD;

    if ((IN(c->start + 17412) != 'D') ||
        (IN(c->start + 17413) != 'I') ||
        (IN(c->start + 17414) != 'G') ||
        (IN(c->start + 17415) != 'I'))
        return PW_BAD;

    if ((IN(c->start + 18424) != 'D') ||
        (IN(c->start + 18425) != 'I') ||
        (IN(c->start + 18426) != 'G') ||
        (IN(c->start + 18427) != 'P'))
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_BSIFutureComposer(pw_ctx *c)
{
    /* get whole sample size */
    c->wss = 0;
    for (c->j = 0; c->j < 63; c->j++)
    {
        c->o = ((IN(c->start + 17420 + (c->j * 16)) * 256 * 256 * 256) +
                (IN(c->start + 17421 + (c->j * 16)) * 256 * 256) +
                (IN(c->start + 17422 + (c->j * 16)) * 256) +
                IN(c->start + 17423 + (c->j * 16)));
        c->wss += c->o;
    }

    c->out_size = c->wss + 18428;

    c->convert = PW_BAD;
    pw_save_rip(c, "BSI Future Composer module", PWF_BSIFC);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 2); /* 0 should do but call it "just to be sure" :) */
}
