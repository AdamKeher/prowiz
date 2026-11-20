/*
 * Crunchmania-Address.c - Crunchmania Address executables
 *                         (upstream r/Crunchmania-Address.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* 20091203 - update with yet another case */
int16_t pw_test_crunchmaniaAddr(pw_ctx *c, int arg)
{
    c->start = (int32_t)c->i;

    if (arg == 1)
    {
        if ((IN(c->start + 32) != 0x6F) ||
            (IN(c->start + 33) != 0x14) ||
            (IN(c->start + 34) != 0x26) ||
            (IN(c->start + 35) != 0x4A) ||
            (IN(c->start + 36) != 0x49) ||
            (IN(c->start + 37) != 0xE9) ||
            (IN(c->start + 42) != 0xE4) ||
            (IN(c->start + 43) != 0x8F) ||
            (IN(c->start + 44) != 0x52) ||
            (IN(c->start + 45) != 0x87) ||
            (IN(c->start + 46) != 0x24) ||
            (IN(c->start + 47) != 0x4C) ||
            (IN(c->start + 48) != 0x28) ||
            (IN(c->start + 49) != 0xDB) ||
            (IN(c->start + 50) != 0x53) ||
            (IN(c->start + 51) != 0x87) ||
            (IN(c->start + 52) != 0x66) ||
            (IN(c->start + 53) != 0xFA))
        { /* another case ...*/
            if ((IN(c->start + 36) != 0x22) ||
                (IN(c->start + 37) != 0x1A) ||
                (IN(c->start + 38) != 0x24) ||
                (IN(c->start + 39) != 0x1A) ||
                (IN(c->start + 40) != 0x47) ||
                (IN(c->start + 41) != 0xEA))
            {
                if ((IN(c->start + 36) != 0x00) ||
                    (IN(c->start + 37) != 0x07) ||
                    (IN(c->start + 38) != 0xf7) ||
                    (IN(c->start + 39) != 0x00) ||
                    (IN(c->start + 40) != 0x20) ||
                    (IN(c->start + 41) != 0x4c))
                {
                    /* should be enough :))) */
                    return PW_BAD;
                }
            }
        }
        /* packed size */
        c->j = ((IN(c->start + 6) * 256) +
                IN(c->start + 7)) + 10;
        if ((c->start + c->j) > (uint32_t)c->in_size)
        {
            return PW_BAD;
        }

        c->l = (((uint32_t)IN(c->start + c->j) * 256 * 256 * 256) +
                (IN(c->start + c->j + 1) * 256 * 256) +
                (IN(c->start + c->j + 2) * 256) +
                IN(c->start + c->j + 3));

        c->l += (40 + c->j);
        if ((c->l % 4) != 0)
            c->l += 2;
    }
    else if (arg == 2)
    {
        if ((IN(c->start + 36) != 0x00) ||
            (IN(c->start + 37) != 0x07) ||
            (IN(c->start + 38) != 0xf7) ||
            (IN(c->start + 39) != 0x00) ||
            (IN(c->start + 40) != 0x20) ||
            (IN(c->start + 41) != 0x4c))
        {
            /* should be enough :))) */
            return PW_BAD;
        }
        c->j = 374;
        if ((c->start + c->j) > (uint32_t)c->in_size)
        {
            return PW_BAD;
        }

        c->l = (((uint32_t)IN(c->start + c->j) * 256 * 256 * 256) +
                (IN(c->start + c->j + 1) * 256 * 256) +
                (IN(c->start + c->j + 2) * 256) +
                IN(c->start + c->j + 3));
        c->l += 378;
        if (((c->l / 4) * 4) != c->l)
            c->l += 2;
    }
    else
        return PW_BAD;

    /* the size is computed once more here, as in the original (this result
     * is the one used) */
    if ((c->start + c->j) > (uint32_t)c->in_size)
    {
        return PW_BAD;
    }

    c->l = (((uint32_t)IN(c->start + c->j) * 256 * 256 * 256) +
            (IN(c->start + c->j + 1) * 256 * 256) +
            (IN(c->start + c->j + 2) * 256) +
            IN(c->start + c->j + 3));

    c->l += (40 + c->j);
    if ((c->l % 4) != 0)
        c->l += 2;

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

void pw_rip_CrunchmaniaAddr(pw_ctx *c)
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
        pw_save_rip_special(c, "Crunchmania Address Exe-file", PWF_CRM1, hdr, 32);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "Crunchmania Address Exe-file", PWF_CRM1);
    }

    if (c->save_status == PW_GOOD)
    {
        /* Guard (not in the original): when OutputSize - 16 wraps (a size
         * below 16, or a huge one) and brings the scan back to or before this
         * position, the original found the same file again forever. Stay
         * here instead. A step that wraps past the end is kept. */
        if ((uint32_t)(c->i + (c->out_size - 16) + 1) > c->i)
            c->i += (c->out_size - 16);
    }
}
