/*
 * StoneCracker270.c - StoneCracker 2.70 Exe-file (upstream r/StoneCracker270.c).
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

/* PW_i += delta. Guard (not in the original): when the size field is tiny
 * the original steps back over bytes it has already scanned, finds the same
 * data again and loops forever. This rip does not change the input, so in
 * that case the scan position is kept instead. */
static void scan_advance(pw_ctx *c, uint32_t delta)
{
    uint32_t next = c->i + delta;

    if ((uint32_t)(next + 1) <= c->i)
        return;
    c->i = next;
}

int16_t pw_test_StoneCracker270(pw_ctx *c)
{
    static const uint8_t code[18] = {
        0x1E, 0x3A, 0x01, 0x57, 0x1A, 0x3A, 0x01, 0x60, 0x1C,
        0x3A, 0x01, 0x5D, 0x41, 0xFA, 0x01, 0x5A, 0xD1, 0xFA};
    static const uint8_t hunk[16] = {
        0x00, 0x00, 0x03, 0xF3, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};

    c->start = (int32_t)c->i;

    if (!match(c, c->start + 16, code, 18))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 370) * 256 * 256 * 256) +
            (IN(c->start + 371) * 256 * 256) +
            (IN(c->start + 372) * 256) +
            IN(c->start + 373));

    c->l += 412;

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

void pw_rip_StoneCracker270(pw_ctx *c)
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
        pw_save_rip_special(c, "StoneCracker 2.70 Exe-file", PWF_StoneCracker270, header, 32);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "StoneCracker 2.70 Exe-file", PWF_StoneCracker270);
    }

    if (c->save_status == PW_GOOD)
        scan_advance(c, c->out_size - 36); /* 32 should do but call it "just to be sure" :) */
}
