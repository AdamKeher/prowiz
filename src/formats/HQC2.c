/*
 * HQC2.c - HQC Cruncher 2.0 executables (upstream r/HQC2.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_HQCCruncher2(pw_ctx *c)
{
    c->start = (int32_t)(c->i - 64);

    if ((IN(c->start + 1776) != 0x02) ||
        (IN(c->start + 1777) != 0x4D) ||
        (IN(c->start + 1778) != 0x45) ||
        (IN(c->start + 1779) != 0x58) ||
        (IN(c->start + 1780) != 0x00) ||
        (IN(c->start + 1781) != 0x00) ||
        (IN(c->start + 1782) != 0x03) ||
        (IN(c->start + 1783) != 0xEC) ||
        (IN(c->start + 1784) != 0x00) ||
        (IN(c->start + 1785) != 0x00) ||
        (IN(c->start + 1786) != 0x00) ||
        (IN(c->start + 1787) != 0x00) ||
        (IN(c->start + 1788) != 0x00) ||
        (IN(c->start + 1789) != 0x00) ||
        (IN(c->start + 1790) != 0x03) ||
        (IN(c->start + 1791) != 0xF2))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 1808) * 256 * 256 * 256) +
            (IN(c->start + 1809) * 256 * 256) +
            (IN(c->start + 1810) * 256) +
            IN(c->start + 1811));

    c->l *= 4;
    c->l += 1816;

    /* the hunk header test is disabled in the original */
    c->amiga_exe_header = PW_GOOD;

    return PW_GOOD;
    /* c->l is the size of the pack */
}

void pw_rip_HQCCruncher2(pw_ctx *c)
{
    /* c->l is still the whole size */
    /* The original filled a 36 byte block but saved 50 bytes of it; the 14
     * bytes past the block are zero here. */
    uint8_t hdr[50];

    c->out_size = c->l;

    c->convert = PW_BAD;

    /*---------------------------------------------------*/
    /* not used yet coz I still dont know hoz to rebuild */
    /* 3 chunks EXE cases ...                            */
    /* so, the code below IS useless AND wrong !!!!!     */
    /* (the test always sets the header as present)      */
    /*---------------------------------------------------*/
    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 50;
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

        /* also the last 24 bytes are 'removed' frequently ... Here they are */
        IN_SET(c->start + c->out_size - 24, 0x00);
        IN_SET(c->start + c->out_size - 23, 0x00);
        IN_SET(c->start + c->out_size - 22, 0x03);
        IN_SET(c->start + c->out_size - 21, 0xEC);

        IN_SET(c->start + c->out_size - 20, 0x00);
        IN_SET(c->start + c->out_size - 19, 0x00);
        IN_SET(c->start + c->out_size - 18, 0x00);
        IN_SET(c->start + c->out_size - 17, 0x00);

        IN_SET(c->start + c->out_size - 16, 0x00);
        IN_SET(c->start + c->out_size - 15, 0x00);
        IN_SET(c->start + c->out_size - 14, 0x03);
        IN_SET(c->start + c->out_size - 13, 0xF2);

        IN_SET(c->start + c->out_size - 12, 0x00);
        IN_SET(c->start + c->out_size - 11, 0x00);
        IN_SET(c->start + c->out_size - 10, 0x03);
        IN_SET(c->start + c->out_size - 9, 0xEB);

        IN_SET(c->start + c->out_size - 8, 0x00);
        IN_SET(c->start + c->out_size - 7, 0x00);
        IN_SET(c->start + c->out_size - 6, 0x00);
        IN_SET(c->start + c->out_size - 5, 0x01);

        IN_SET(c->start + c->out_size - 4, 0x00);
        IN_SET(c->start + c->out_size - 3, 0x00);
        IN_SET(c->start + c->out_size - 2, 0x03);
        IN_SET(c->start + c->out_size - 1, 0xF2);

        pw_save_rip_special(c, "HQC Cruncher 2.0 Exe-file", PWF_HQC, hdr, 50);
    }
    else
    {
        pw_save_rip(c, "HQC Cruncher 2.0 Exe-file", PWF_HQC);
    }

    if (c->save_status == PW_GOOD)
    {
        /* Guard (not in the original): when OutputSize - 65 wraps (a size
         * below 65, or a huge one) and brings the scan back to or before this
         * position, the original found the same file again forever. Stay
         * here instead. A step that wraps past the end is kept. */
        if ((uint32_t)(c->i + (c->out_size - 65) + 1) > c->i)
            c->i += (c->out_size - 65); /* 64 should do but call it "just to be sure" :) */
    }
}
