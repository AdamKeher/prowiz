/*
 * QC.c - Quadra Composer module, "FORM....EMIC" (upstream r/QC.c).
 *
 * Only the test and the rip are ported: the scanner of the original never
 * calls Depack_QuadraComposer().
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_QuadraComposer(pw_ctx *c)
{
    /* test #1 */
    if (c->i < 8)
        return PW_BAD;
    c->start = (int32_t)(c->i - 8);

    /* test #2 "FORM" & "EMIC" */
    if ((IN(c->start) != 'F') ||
        (IN(c->start + 1) != 'O') ||
        (IN(c->start + 2) != 'R') ||
        (IN(c->start + 3) != 'M') ||
        (IN(c->start + 12) != 'E') ||
        (IN(c->start + 13) != 'M') ||
        (IN(c->start + 14) != 'I') ||
        (IN(c->start + 15) != 'C'))
        return PW_BAD;

    /* test number of samples */
    c->l = IN(c->start + 63);
    if ((c->l == 0x00) || (c->l > 0x20))
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_QuadraComposer(pw_ctx *c)
{
    uint32_t next;

    c->l = (IN(c->start + 4) * 256 * 256 * 256) +
           (IN(c->start + 5) * 256 * 256) +
           (IN(c->start + 6) * 256) +
           IN(c->start + 7);

    c->out_size = c->l + 8;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Quadra Composer module", PWF_QuadraComposer);

    if (c->save_status == PW_GOOD)
    {
        next = c->i + (c->out_size - 9); /* 8 should do but call it "just to be sure" :) */
        /* Hang guard (not in the original). With a FORM size below 1 (or
         * one that wraps) upstream moves the scan position back, reaches
         * this module again and rips it forever ("0.qc", "1.qc", ...). Keep
         * the position instead: the output matches the original up to the
         * point where it starts repeating, and the scan goes on. */
        if (next >= c->i)
            c->i = next;
    }
}
