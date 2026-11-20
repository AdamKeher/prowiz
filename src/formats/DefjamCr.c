/*
 * DefjamCr.c - Defjam Cruncher 3.2 / 3.2 pro / 3.2T executables
 *              (upstream r/DefjamCr.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_Defjam32(pw_ctx *c)
{
    c->start = (int32_t)(c->i - 2);

    if ((IN(c->start + 12) != 0x28) ||
        (IN(c->start + 13) != 0x7A) ||
        (IN(c->start + 14) != 0x01) ||
        (IN(c->start + 15) != 0x52) ||
        (IN(c->start + 16) != 0x20) ||
        (IN(c->start + 17) != 0x4C) ||
        (IN(c->start + 18) != 0xD1) ||
        (IN(c->start + 19) != 0xFC) ||
        (IN(c->start + 24) != 0xB3) ||
        (IN(c->start + 25) != 0xCC) ||
        (IN(c->start + 26) != 0x6E) ||
        (IN(c->start + 27) != 0x08) ||
        (IN(c->start + 28) != 0x20) ||
        (IN(c->start + 29) != 0x49) ||
        (IN(c->start + 30) != 0xD1) ||
        (IN(c->start + 31) != 0xFA) ||
        (IN(c->start + 32) != 0xFF) ||
        (IN(c->start + 33) != 0xF4) ||
        (IN(c->start + 34) != 0x60) ||
        (IN(c->start + 35) != 0x06) ||
        (IN(c->start + 36) != 0x18) ||
        (IN(c->start + 37) != 0xD9) ||
        (IN(c->start + 38) != 0xB9) ||
        (IN(c->start + 39) != 0xC8) ||
        (IN(c->start + 40) != 0x6D) ||
        (IN(c->start + 41) != 0xFA) ||
        (IN(c->start + 42) != 0x43) ||
        (IN(c->start + 43) != 0xF9) ||
        (IN(c->start + 44) != 0x00))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 20) * 256 * 256 * 256) +
            (IN(c->start + 21) * 256 * 256) +
            (IN(c->start + 22) * 256) +
            IN(c->start + 23));

    c->l += 692;

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

int16_t pw_test_Defjam32pro(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 16) != 0x3D) ||
        (IN(c->start + 17) != 0x40) ||
        (IN(c->start + 18) != 0x00) ||
        (IN(c->start + 19) != 0x9A) ||
        (IN(c->start + 20) != 0x3D) ||
        (IN(c->start + 21) != 0x40) ||
        (IN(c->start + 22) != 0x00) ||
        (IN(c->start + 23) != 0x9C))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 180) * 256 * 256 * 256) +
            (IN(c->start + 181) * 256 * 256) +
            (IN(c->start + 182) * 256) +
            IN(c->start + 183));

    c->l += 796;

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

int16_t pw_test_Defjam32t(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 12) != 0x28) ||
        (IN(c->start + 13) != 0x7A) ||
        (IN(c->start + 14) != 0x01) ||
        (IN(c->start + 15) != 0x54) ||
        (IN(c->start + 16) != 0x20) ||
        (IN(c->start + 17) != 0x4C) ||
        (IN(c->start + 18) != 0xD1) ||
        (IN(c->start + 19) != 0xFC))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 20) * 256 * 256 * 256) +
            (IN(c->start + 21) * 256 * 256) +
            (IN(c->start + 22) * 256) +
            IN(c->start + 23));

    c->l += 692;

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

void pw_rip_Defjam32(pw_ctx *c)
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

        pw_save_rip_special(c, "Defjam Cruncher 3.2 / pro / t Exe-file", PWF_Defjam_32, hdr, 32);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "Defjam Cruncher 3.2 / pro / t Exe-file", PWF_Defjam_32);
    }

    if (c->save_status == PW_GOOD)
        c->i += 36;
}
