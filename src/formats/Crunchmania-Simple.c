/*
 * Crunchmania-Simple.c - Crunchmania Simple executables
 *                        (upstream r/Crunchmania-Simple.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_crunchmaniaSimple(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 16) != 0xFF) ||
        (IN(c->start + 17) != 0xEC) ||
        (IN(c->start + 18) != 0xD9) ||
        (IN(c->start + 19) != 0xCC) ||
        (IN(c->start + 20) != 0xD9) ||
        (IN(c->start + 21) != 0xCC) ||
        (IN(c->start + 22) != 0x2A) ||
        (IN(c->start + 23) != 0x49) ||
        (IN(c->start + 24) != 0xD3) ||
        (IN(c->start + 25) != 0xC1) ||
        (IN(c->start + 26) != 0xD5) ||
        (IN(c->start + 27) != 0xC2) ||
        (IN(c->start + 28) != 0x30) ||
        (IN(c->start + 29) != 0x22) ||
        (IN(c->start + 30) != 0x2C) ||
        (IN(c->start + 31) != 0x22) ||
        (IN(c->start + 32) != 0x7E) ||
        (IN(c->start + 33) != 0x10))
    {
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 368) * 256 * 256 * 256) +
            (IN(c->start + 369) * 256 * 256) +
            (IN(c->start + 370) * 256) +
            IN(c->start + 371));

    c->l += 422;

    if (c->i >= 36)
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
            (IN(c->start - 21) != 0x02) ||
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

void pw_rip_CrunchmaniaSimple(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t hdr[36];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        /* packed size */
        c->n = (((uint32_t)IN(c->start + 364) * 256 * 256 * 256) +
                (IN(c->start + 365) * 256 * 256) +
                (IN(c->start + 366) * 256) +
                IN(c->start + 367));

        c->out_size -= 36;
        memset(hdr, 0, sizeof hdr);
        hdr[2] = hdr[30] = 0x03;
        hdr[3] = 0xF3;
        hdr[19] = 0x01;
        hdr[31] = 0xE9;

        hdr[24] = 0x40;
        hdr[25] = IN(c->start + 361);
        hdr[26] = IN(c->start + 362);
        hdr[27] = IN(c->start + 363);

        /* the original copied the bytes of PW_j / PW_n in reverse (x86) order */
        c->j = c->l - 48;
        c->j /= 4;
        hdr[32] = (uint8_t)(c->j >> 24);
        hdr[33] = (uint8_t)(c->j >> 16);
        hdr[34] = (uint8_t)(c->j >> 8);
        hdr[35] = (uint8_t)c->j;

        if ((c->n % 4) != 0)
            c->n += 2;
        c->n += 372;
        c->n /= 4;
        hdr[20] = (uint8_t)(c->n >> 24);
        hdr[21] = (uint8_t)(c->n >> 16);
        hdr[22] = (uint8_t)(c->n >> 8);
        hdr[23] = (uint8_t)c->n;

        pw_save_rip_special(c, "Crunchmania Simple Exe-file", PWF_CrunchmaniaSimple, hdr, 36);
    }
    else
    {
        c->start -= 36;
        pw_save_rip(c, "Crunchmania Simple Exe-file", PWF_CrunchmaniaSimple);
    }

    if (c->save_status == PW_GOOD)
        c->i += 0x66; /* after detection sequence */
}
