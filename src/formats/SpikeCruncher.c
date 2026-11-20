/*
 * SpikeCruncher.c - Spike Cruncher Exe-file (upstream r/SpikeCruncher.c).
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

int16_t pw_test_SpikeCruncher(pw_ctx *c)
{
    static const uint8_t code[16] = {
        0x2C, 0x78, 0x00, 0x04, 0x22, 0x4B, 0x24, 0x4B,
        0x2F, 0x0B, 0x20, 0x3A, 0xFF, 0xD8, 0xD3, 0xC0};
    static const uint8_t hunk[16] = {
        0x00, 0x00, 0x03, 0xF3, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00};

    c->start = (int32_t)c->i;

    if (!match(c, c->start + 38, code, 16))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 10) * 256 * 256 * 256) +
            (IN(c->start + 11) * 256 * 256) +
            (IN(c->start + 12) * 256) +
            IN(c->start + 13));

    c->l += 648;

    if (c->i >= 36)
    {
        if (!match(c, c->start - 36, hunk, 16))
            c->amiga_exe_header = PW_BAD;
        else
            c->amiga_exe_header = PW_GOOD;
    }
    else
        c->amiga_exe_header = PW_BAD;

    return PW_GOOD;
    /* c->l is the size of the pack */
}

void pw_rip_SpikeCruncher(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t header[36];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 36;
        memset(header, 0, sizeof header);

        header[2] = header[30] = 0x03;
        header[3] = 0xF3;
        header[11] = 0x02;
        header[19] = 0x01;
        header[23] = header[35] = 0x96;
        header[31] = 0xE9;

        pw_save_rip_special(c, "Spike Cruncher Exe-file", PWF_SpikeCruncher, header, 36);
    }
    else
    {
        c->start -= 36;
        pw_save_rip(c, "Spike Cruncher Exe-file", PWF_SpikeCruncher);
    }

    if (c->save_status == PW_GOOD)
        scan_advance(c, c->out_size - 40); /* 36 should do but call it "just to be sure" :) */
}
