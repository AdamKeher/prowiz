/*
 * DoubleAction.c - Double Action 1.0 executables (upstream r/DoubleAction.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_DoubleAction10(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 32) != 0x12) ||
        (IN(c->start + 33) != 0xBC) ||
        (IN(c->start + 34) != 0x00) ||
        (IN(c->start + 35) != 0xFD))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start - 4) * 256 * 256 * 256) +
            (IN(c->start - 3) * 256 * 256) +
            (IN(c->start - 2) * 256) +
            IN(c->start - 1));
    c->l *= 4;
    c->l += 36;

    /* the original also tested PW_l < 0, never true for the unsigned PW_l */
    if (c->l > 2000000)
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    if (c->i >= 32)
    {
        if ((IN(c->start - 32) != 0x00) ||
            (IN(c->start - 31) != 0x00) ||
            (IN(c->start - 30) != 0x03) ||
            (IN(c->start - 29) != 0xF3) ||
            (IN(c->start - 28) != 0x00) ||
            (IN(c->start - 27) != 0x00) ||
            (IN(c->start - 26) != 0x00) ||
            (IN(c->start - 25) != 0x00) ||
            (IN(c->start - 24) != 0x00) ||
            (IN(c->start - 23) != 0x00) ||
            (IN(c->start - 22) != 0x00) ||
            (IN(c->start - 21) != 0x01) ||
            (IN(c->start - 20) != 0x00) ||
            (IN(c->start - 19) != 0x00) ||
            (IN(c->start - 18) != 0x00) ||
            (IN(c->start - 17) != 0x00))
        {
            c->amiga_exe_header = PW_BAD;
        }
        else
            c->amiga_exe_header = PW_GOOD;
    }
    else
        c->amiga_exe_header = PW_BAD;

    return PW_GOOD;
    /* c->l is the size of the pack */
}

void pw_rip_DoubleAction10(pw_ctx *c)
{
    /* c->l is still the whole size */

    c->out_size = c->l;

    c->convert = PW_BAD;
    if (c->amiga_exe_header == PW_BAD)
    {
        /* the header can't be rebuilt; Save_Status keeps its old value */
        pw_printf(c, "Double Action 1.0 Exe-file without header detected at %d. Can't rebuild it, sorry\n", c->start);
        pw_report_unrippable(c, PWF_Double_Action, "Double Action 1.0 Exe-file", (uint32_t)c->start);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "Double Action 1.0 Exe-file", PWF_Double_Action);
    }

    if (c->save_status == PW_GOOD)
        c->i += 0xA0;
}
