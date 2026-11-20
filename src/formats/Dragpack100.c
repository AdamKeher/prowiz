/*
 * Dragpack100.c - DragPack 1.00 executables (upstream r/Dragpack100.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_Dragpack100(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 16) != 0x20) ||
        (IN(c->start + 17) != 0x18) ||
        (IN(c->start + 18) != 0x22) ||
        (IN(c->start + 19) != 0x18) ||
        (IN(c->start + 20) != 0x2A) ||
        (IN(c->start + 21) != 0x18) ||
        (IN(c->start + 22) != 0x24) ||
        (IN(c->start + 23) != 0x49) ||
        (IN(c->start + 24) != 0xD1) ||
        (IN(c->start + 25) != 0xC0) ||
        (IN(c->start + 26) != 0xD5) ||
        (IN(c->start + 27) != 0xC1))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 0x2ec) * 256 * 256 * 256) +
            (IN(c->start + 0x2ed) * 256 * 256) +
            (IN(c->start + 0x2ee) * 256) +
            IN(c->start + 0x2ef));

    c->l += 0x330;

    /* (sic) tests 32 but looks 40 bytes back */
    if (c->i >= 32)
    {
        if ((IN(c->start - 40) != 0x00) ||
            (IN(c->start - 39) != 0x00) ||
            (IN(c->start - 38) != 0x03) ||
            (IN(c->start - 37) != 0xF3) ||
            (IN(c->start - 36) != 0x00) ||
            (IN(c->start - 35) != 0x00) ||
            (IN(c->start - 34) != 0x00) ||
            (IN(c->start - 33) != 0x00) ||
            (IN(c->start - 32) != 0x00) ||
            (IN(c->start - 31) != 0x00) ||
            (IN(c->start - 30) != 0x00) ||
            (IN(c->start - 29) != 0x03) ||
            (IN(c->start - 28) != 0x00) ||
            (IN(c->start - 27) != 0x00) ||
            (IN(c->start - 26) != 0x00) ||
            (IN(c->start - 25) != 0x00))
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

void pw_rip_Dragpack100(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t hdr[40];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 40;
        memset(hdr, 0, sizeof hdr);
        hdr[2] = 0x03;
        hdr[3] = 0xF3;
        hdr[11] = 0x03;
        hdr[19] = 0x02;
        hdr[23] = 0xA2;
        hdr[34] = 0x03;
        hdr[35] = 0xE9;
        hdr[39] = 0xA2;

        /* the original copied the bytes of PW_j in reverse (x86) order */
        c->j = c->l - 804;
        c->j /= 4;
        hdr[24] = hdr[28] = (uint8_t)(c->j >> 24);
        hdr[25] = hdr[29] = (uint8_t)(c->j >> 16);
        hdr[26] = hdr[30] = (uint8_t)(c->j >> 8);
        hdr[27] = hdr[31] = (uint8_t)c->j;
        pw_save_rip_special(c, "DragPack 1.00 Exe-file", PWF_DragPack100, hdr, 40);
    }
    else
    {
        c->start -= 40;
        pw_save_rip(c, "DragPack 1.00 Exe-file", PWF_DragPack100);
    }

    if (c->save_status == PW_GOOD)
        c->i += 0x42; /* 40 should do but call it "just to be sure" :) */
}
