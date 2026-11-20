/*
 * StoneCracker299.c - StoneCracker 2.99 Exe-file (upstream r/StoneCracker299.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* the input bytes at 'off' are 'sig' (the original compared them one by one) */
static int match(const pw_ctx *c, int64_t off, const uint8_t *sig, int n)
{
    int k;

    for (k = 0; k < n; k++)
        if (IN(off + k) != sig[k])
            return 0;
    return 1;
}

int16_t pw_test_STC299(pw_ctx *c)
{
    static const uint8_t code[16] = {
        0x45, 0xFA, 0x00, 0x5E, 0x3D, 0x5A, 0x00, 0x9A,
        0x18, 0xBC, 0x00, 0x81, 0x3D, 0x5A, 0x00, 0x96};
    static const uint8_t hunk[16] = {
        0x00, 0x00, 0x03, 0xF3, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};

    c->start = (int32_t)c->i;

    if (!match(c, c->start + 16, code, 16))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 352) * 256 * 256 * 256) +
            (IN(c->start + 353) * 256 * 256) +
            (IN(c->start + 354) * 256) +
            IN(c->start + 355));

    c->l += 392;

    if (c->i >= 32)
    {
        if (!match(c, c->start - 32, hunk, 16))
            c->amiga_exe_header = PW_BAD;
        else
            c->amiga_exe_header = PW_GOOD;
    }
    else
        c->amiga_exe_header = PW_BAD;

    return PW_GOOD;
    /* c->l is the size of the pack */
}

void pw_rip_STC299(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t header[32];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 32;
        memset(header, 0, sizeof header);
        header[2] = header[26] = 0x03;
        header[3] = 0xF3;
        header[11] = 0x01;
        header[27] = 0xE9;

        /* the original copied the bytes of PW_j from memory (x86 order) */
        c->j = c->l - 36;
        c->j /= 4;
        header[20] = header[28] = (uint8_t)(c->j >> 24);
        header[21] = header[29] = (uint8_t)(c->j >> 16);
        header[22] = header[30] = (uint8_t)(c->j >> 8);
        header[23] = header[31] = (uint8_t)c->j;
        pw_save_rip_special(c, "StoneCracker 2.99 Exe-file", PWF_STC299, header, 32);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "StoneCracker 2.99 Exe-file", PWF_STC299);
    }

    if (c->save_status == PW_GOOD)
        c->i += 348;
}
