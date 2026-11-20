/*
 * GNUPacker12.c - GNU Packer 1.2 executables (upstream r/GNUPacker12.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_GNUPacker12(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 16) != 0x22) ||
        (IN(c->start + 17) != 0x48) ||
        (IN(c->start + 18) != 0xD3) ||
        (IN(c->start + 19) != 0xED) ||
        (IN(c->start + 20) != 0x00) ||
        (IN(c->start + 21) != 0x08) ||
        (IN(c->start + 22) != 0x30) ||
        (IN(c->start + 23) != 0xDE) ||
        (IN(c->start + 24) != 0xB1) ||
        (IN(c->start + 25) != 0xC9) ||
        (IN(c->start + 26) != 0x66) ||
        (IN(c->start + 27) != 0x00) ||
        (IN(c->start + 28) != 0xFF) ||
        (IN(c->start + 29) != 0xFA) ||
        (IN(c->start + 30) != 0x20) ||
        (IN(c->start + 31) != 0x6D))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 0x240) * 256 * 256 * 256) +
            (IN(c->start + 0x241) * 256 * 256) +
            (IN(c->start + 0x242) * 256) +
            IN(c->start + 0x243));

    c->l += 0x274;

    if (c->i >= 32)
    {
        /* (sic) -19 is tested twice and -18 not at all */
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
            (IN(c->start - 19) != 0x00) ||
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

void pw_rip_GNUPacker12(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t hdr[32];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 32;
        memset(hdr, 0, sizeof hdr);
        hdr[2] = hdr[26] = 0x03;
        hdr[3] = 0xF3;
        hdr[11] = 0x01;
        hdr[27] = 0xE9;

        /* the original copied the bytes of PW_j in reverse (x86) order */
        c->j = c->l - 36;
        c->j /= 4;
        hdr[20] = hdr[28] = (uint8_t)(c->j >> 24);
        hdr[21] = hdr[29] = (uint8_t)(c->j >> 16);
        hdr[22] = hdr[30] = (uint8_t)(c->j >> 8);
        hdr[23] = hdr[31] = (uint8_t)c->j;

        IN_SET(c->start + c->out_size - 4, 0x00);
        IN_SET(c->start + c->out_size - 3, 0x00);
        IN_SET(c->start + c->out_size - 2, 0x03);
        IN_SET(c->start + c->out_size - 1, 0xF2);

        pw_save_rip_special(c, "GNU Packer 1.2 Exe-file", PWF_GNUPacker12, hdr, 32);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "GNU Packer 1.2 Exe-file", PWF_GNUPacker12);
    }

    if (c->save_status == PW_GOOD)
        c->i += 32;
}
