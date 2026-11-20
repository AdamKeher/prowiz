/*
 * HighPresCr.c - High Pressure Cruncher executables (upstream r/HighPresCr.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_HighPressureCruncher(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 23) != 0x84) ||
        (IN(c->start + 24) != 0x28) ||
        (IN(c->start + 25) != 0x00) ||
        (IN(c->start + 26) != 0xD5) ||
        (IN(c->start + 27) != 0xC4) ||
        (IN(c->start + 28) != 0x61) ||
        (IN(c->start + 29) != 0x00) ||
        (IN(c->start + 30) != 0x00) ||
        (IN(c->start + 31) != 0x7C) ||
        (IN(c->start + 32) != 0xD3) ||
        (IN(c->start + 33) != 0xC0) ||
        (IN(c->start + 34) != 0x26) ||
        (IN(c->start + 35) != 0x00) ||
        (IN(c->start + 36) != 0x08) ||
        (IN(c->start + 37) != 0x79) ||
        (IN(c->start + 38) != 0x00) ||
        (IN(c->start + 39) != 0x01) ||
        (IN(c->start + 40) != 0x00) ||
        (IN(c->start + 41) != 0xBF) ||
        (IN(c->start + 42) != 0xE0) ||
        (IN(c->start + 43) != 0x01) ||
        (IN(c->start + 44) != 0x18))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 176) * 256 * 256 * 256) +
            (IN(c->start + 177) * 256 * 256) +
            (IN(c->start + 178) * 256) +
            IN(c->start + 179));

    c->l += 548;

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

void pw_rip_HighPressureCruncher(pw_ctx *c)
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
        hdr[13] = hdr[27] = 0x01;
        hdr[31] = 0xE9;

        /* the original copied the bytes of PW_j in reverse (x86) order */
        c->j = c->l - 60;
        c->j /= 4;
        hdr[20] = hdr[32] = (uint8_t)(c->j >> 24);
        hdr[21] = hdr[33] = (uint8_t)(c->j >> 16);
        hdr[22] = hdr[34] = (uint8_t)(c->j >> 8);
        hdr[23] = hdr[35] = (uint8_t)c->j;

        pw_save_rip_special(c, "High Pressure Cruncher Exe-file", PWF_HighPresCruncher, hdr, 36);
    }
    else
    {
        c->start -= 36;
        pw_save_rip(c, "High Pressure Cruncher Exe-file", PWF_HighPresCruncher);
    }

    if (c->save_status == PW_GOOD)
        c->i += 1;
}
