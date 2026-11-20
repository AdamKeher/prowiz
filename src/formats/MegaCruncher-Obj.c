/*
 * MegaCruncher-Obj.c - Mega Cruncher Obj executables
 *                      (upstream r/MegaCruncher-Obj.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_MegaCruncherObj(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 12) != 0x4C) ||
        (IN(c->start + 13) != 0xDD) ||
        (IN(c->start + 14) != 0x00) ||
        (IN(c->start + 15) != 0x03) ||
        (IN(c->start + 16) != 0x4E) ||
        (IN(c->start + 17) != 0xAE) ||
        (IN(c->start + 18) != 0xFF) ||
        (IN(c->start + 19) != 0x3A) ||
        (IN(c->start + 20) != 0x4A) ||
        (IN(c->start + 21) != 0x80) ||
        (IN(c->start + 22) != 0x67) ||
        (IN(c->start + 23) != 0x30) ||
        (IN(c->start + 24) != 0x41) ||
        (IN(c->start + 25) != 0xFA) ||
        (IN(c->start + 26) != 0x00) ||
        (IN(c->start + 27) != 0x0E))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 466) * 256 * 256 * 256) +
            (IN(c->start + 467) * 256 * 256) +
            (IN(c->start + 468) * 256) +
            IN(c->start + 469));
    c->l += 532;

    if ((c->l > (uint32_t)c->in_size) || (c->l > 2000000))
    {
        return PW_BAD;
    }

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

void pw_rip_MegaCruncherObj(pw_ctx *c)
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
        c->j = c->l - 204;
        c->j /= 4;
        hdr[20] = hdr[32] = (uint8_t)(c->j >> 24);
        hdr[21] = hdr[33] = (uint8_t)(c->j >> 16);
        hdr[22] = hdr[34] = (uint8_t)(c->j >> 8);
        hdr[23] = hdr[35] = (uint8_t)c->j;

        /* also the last 16 bytes are 'removed' frequently ... Here they are */
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

        pw_save_rip_special(c, "Mega Cruncher Obj", PWF_MegaCruncherObj, hdr, 36);
    }
    else
    {
        c->start -= 36;
        pw_save_rip(c, "Mega Cruncher Obj", PWF_MegaCruncherObj);
    }

    if (c->save_status == PW_GOOD)
        c->i += 40; /* beside header */
}
