/*
 * TryItCruncher101.c - Try-It Cruncher 1.01 Exe-file
 *                      (upstream r/TryItCruncher101.c).
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

int16_t pw_test_TryIt101(pw_ctx *c)
{
    static const uint8_t code[16] = {
        0x22, 0x2D, 0x00, 0x10, 0x10, 0xD9, 0x53, 0x81,
        0x66, 0xFA, 0x22, 0x4A, 0x20, 0x2D, 0x00, 0x14};
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
    c->l = (((uint32_t)IN(c->start + 358) * 256 * 256 * 256) +
            (IN(c->start + 359) * 256 * 256) +
            (IN(c->start + 360) * 256) +
            IN(c->start + 361));

    c->l += 1470;
    /* not sure about this '+=2' to the size ... */
    /* I have but two exemple filesm so ... exemples please ! */
    c->l = (((c->l / 4) * 4) != c->l) ? c->l + 2 : c->l;

    if (c->i >= 64)
    {
        if (!match(c, c->start - 64, hunk, 16))
            c->amiga_exe_header = PW_BAD;
        else
            c->amiga_exe_header = PW_GOOD;
    }
    else
        c->amiga_exe_header = PW_BAD;

    return PW_GOOD;
    /* c->l is the size of the pack */
}

void pw_rip_TryIt101(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t header[32];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->start -= 32;
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
        pw_save_rip_special(c, "Try-It Cruncher 1.01 Exe-file", PWF_TryIt, header, 32);
    }
    else
    {
        c->start -= 64;
        pw_save_rip(c, "Try-It Cruncher 1.01 Exe-file", PWF_TryIt);
    }

    if (c->save_status == PW_GOOD)
        scan_advance(c, c->out_size - 65); /* 64 should do but call it "just to be sure" :) */
}
