/*
 * MegaCruncher.c - Mega Cruncher 1.0 / 1.2 executables
 *                  (upstream r/MegaCruncher.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_MegaCruncher10(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 16) != 0x22) ||
        (IN(c->start + 17) != 0x6B) ||
        (IN(c->start + 18) != 0x00) ||
        (IN(c->start + 19) != 0x04) ||
        (IN(c->start + 20) != 0x24) ||
        (IN(c->start + 21) != 0x60) ||
        (IN(c->start + 22) != 0xD5) ||
        (IN(c->start + 23) != 0xC9) ||
        (IN(c->start + 24) != 0x20) ||
        (IN(c->start + 25) != 0x20) ||
        (IN(c->start + 26) != 0x72) ||
        (IN(c->start + 27) != 0x03) ||
        (IN(c->start + 28) != 0x61) ||
        (IN(c->start + 29) != 0x00) ||
        (IN(c->start + 30) != 0x01) ||
        (IN(c->start + 31) != 0x02))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 312) * 256 * 256 * 256) +
            (IN(c->start + 313) * 256 * 256) +
            (IN(c->start + 314) * 256) +
            IN(c->start + 315));

    c->l += 376;

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

int16_t pw_test_MegaCruncher12(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 16) != 0x22) ||
        (IN(c->start + 17) != 0x6B) ||
        (IN(c->start + 18) != 0x00) ||
        (IN(c->start + 19) != 0x04) ||
        (IN(c->start + 20) != 0x24) ||
        (IN(c->start + 21) != 0x60) ||
        (IN(c->start + 22) != 0xD5) ||
        (IN(c->start + 23) != 0xC9) ||
        (IN(c->start + 24) != 0x20) ||
        (IN(c->start + 25) != 0x20) ||
        (IN(c->start + 26) != 0x72) ||
        (IN(c->start + 27) != 0x03) ||
        (IN(c->start + 28) != 0x61) ||
        (IN(c->start + 29) != 0x00) ||
        (IN(c->start + 30) != 0x01) ||
        (IN(c->start + 31) != 0x06))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 316) * 256 * 256 * 256) +
            (IN(c->start + 317) * 256 * 256) +
            (IN(c->start + 318) * 256) +
            IN(c->start + 319));

    c->l += 380;

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

void pw_rip_MegaCruncher(pw_ctx *c)
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
        hdr[27] = 0x01;
        hdr[31] = 0xE9;

        /* the original copied the bytes of PW_j in reverse (x86) order */
        c->j = c->l - 60;
        c->j /= 4;
        hdr[20] = hdr[32] = (uint8_t)(c->j >> 24);
        hdr[21] = hdr[33] = (uint8_t)(c->j >> 16);
        hdr[22] = hdr[34] = (uint8_t)(c->j >> 8);
        hdr[23] = hdr[35] = (uint8_t)c->j;
        pw_save_rip_special(c, "Mega Cruncher 1.0/1.2 Exe-file", PWF_MegaCruncher, hdr, 36);
    }
    else
    {
        c->start -= 36;
        pw_save_rip(c, "Mega Cruncher 1.0/1.2 Exe-file", PWF_MegaCruncher);
    }

    if (c->save_status == PW_GOOD)
    {
        /* Guard (not in the original): when OutputSize - 42 wraps (a size
         * below 42, or a huge one) and brings the scan back to or before this
         * position, the original found the same file again forever. Stay
         * here instead. A step that wraps past the end is kept. */
        if ((uint32_t)(c->i + (c->out_size - 42) + 1) > c->i)
            c->i += (c->out_size - 42); /* 36 should do but call it "just to be sure" :) */
    }
}
