/*
 * MastCrunch30-Add.c - Master Cruncher 3.0 address executables
 *                      (upstream r/MastCrunch30-Add.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_MasterCruncher30addr(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 16)  != 0xD3) ||
        (IN(c->start + 17)  != 0xC9) ||
        (IN(c->start + 18)  != 0x58) ||
        (IN(c->start + 19)  != 0x89) ||
        (IN(c->start + 20)  != 0x2B) ||
        (IN(c->start + 21)  != 0x49) ||
        (IN(c->start + 124) != 0xE3) ||
        (IN(c->start + 125) != 0x10) ||
        (IN(c->start + 126) != 0xE3) ||
        (IN(c->start + 127) != 0x51) ||
        (IN(c->start + 128) != 0x51) ||
        (IN(c->start + 129) != 0xCA) ||
        (IN(c->start + 130) != 0xFF) ||
        (IN(c->start + 131) != 0xF4) ||
        (IN(c->start + 132) != 0x4A) ||
        (IN(c->start + 133) != 0x43) ||
        (IN(c->start + 134) != 0x67) ||
        (IN(c->start + 135) != 0x06))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 428) * 256 * 256 * 256) +
            (IN(c->start + 429) * 256 * 256) +
            (IN(c->start + 430) * 256) +
            IN(c->start + 431));

    c->l *= 4;
    c->l += 472;

    if (c->i >= 36)
    {
        if ((IN(c->start - 36) != 0x00) ||
            (IN(c->start - 35) != 0x00) ||
            (IN(c->start - 34) != 0x03) ||
            (IN(c->start - 33) != 0xF3) ||
            (IN(c->start - 32) != 0x00) ||
            (IN(c->start - 31) != 0x00) ||
            (IN(c->start - 30) != 0x00) ||
            (IN(c->start - 29) != 0x00) ||
            (IN(c->start - 28) != 0x00) ||
            (IN(c->start - 27) != 0x00) ||
            (IN(c->start - 26) != 0x00) ||
            (IN(c->start - 25) != 0x02) ||
            (IN(c->start - 24) != 0x00) ||
            (IN(c->start - 23) != 0x00) ||
            (IN(c->start - 22) != 0x00) ||
            (IN(c->start - 21) != 0x00))
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

void pw_rip_MasterCruncher30addr(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t hdr[36];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 36;
        memset(hdr, 0, sizeof hdr);
        hdr[2] = hdr[30] = 0x03;
        hdr[3] = 0xF3;
        hdr[11] = 0x02;
        hdr[19] = 0x01;
        hdr[23] = hdr[35] = 0x6A;
        hdr[31] = 0xE9;

        hdr[24] = IN(c->start + 428);
        hdr[25] = IN(c->start + 429);
        hdr[26] = IN(c->start + 430);
        hdr[27] = IN(c->start + 431);
        pw_save_rip_special(c, "Master Cruncher 3.0 address Exe-file", PWF_MasterCruncher, hdr, 36);
    }
    else
    {
        c->start -= 36;
        pw_save_rip(c, "Master Cruncher 3.0 address Exe-file", PWF_MasterCruncher);
    }

    if (c->save_status == PW_GOOD)
    {
        /* Guard (not in the original): when OutputSize - 38 wraps (a size
         * below 38, or a huge one) and brings the scan back to or before this
         * position, the original found the same file again forever. Stay
         * here instead. A step that wraps past the end is kept. */
        if ((uint32_t)(c->i + (c->out_size - 38) + 1) > c->i)
            c->i += (c->out_size - 38); /* 36 should do but call it "just to be sure" :) */
    }
}
