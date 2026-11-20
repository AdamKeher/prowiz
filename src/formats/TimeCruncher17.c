/*
 * TimeCruncher17.c - Time Cruncher 1.7 Exe-file (upstream r/TimeCruncher17.c).
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

/* in_data[off] = v; returns 1 if this changed the input */
static int patch(pw_ctx *c, uint32_t off, uint8_t v)
{
    int changed = (off < (uint32_t)c->in_size) && (IN(off) != v);

    IN_SET(off, v);
    return changed;
}

/* PW_i += delta. Guard (not in the original): when the size field is tiny
 * the original steps back over bytes it has already scanned. If this rip
 * left the input unchanged it finds the same data again and loops forever,
 * so in that case the scan position is kept instead. */
static void scan_advance(pw_ctx *c, uint32_t delta, int changed)
{
    uint32_t next = c->i + delta;

    if (!changed && (uint32_t)(next + 1) <= c->i)
        return;
    c->i = next;
}

int16_t pw_test_TimeCruncher17(pw_ctx *c)
{
    static const uint8_t code[16] = {
        0x72, 0x03, 0x61, 0x00, 0x00, 0xFC, 0x4A, 0x02,
        0x67, 0x5A, 0x0C, 0x42, 0x00, 0x07, 0x66, 0x24};
    static const uint8_t hunk[16] = {
        0x00, 0x00, 0x03, 0xF3, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00};

    c->start = (int32_t)c->i;

    if (!match(c, c->start + 22, code, 16))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 6) * 256 * 256 * 256) +
            (IN(c->start + 7) * 256 * 256) +
            (IN(c->start + 8) * 256) +
            IN(c->start + 9));

    c->l += 376;

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

void pw_rip_TimeCruncher17(pw_ctx *c)
{
    /* c->l is still the whole size */
    static const uint8_t ending[24] = {
        0x00, 0x00, 0x03, 0xEC, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xF2, 0x00, 0x00, 0x03, 0xEB,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0xF2};
    uint8_t header[36];
    uint32_t end;
    int changed = 0;
    int k;

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
        header[27] = 0x01;
        header[31] = 0xE9;

        /* the original copied the bytes of PW_j from memory (x86 order) */
        c->j = c->l - 60;
        c->j /= 4;
        header[20] = header[32] = (uint8_t)(c->j >> 24);
        header[21] = header[33] = (uint8_t)(c->j >> 16);
        header[22] = header[34] = (uint8_t)(c->j >> 8);
        header[23] = header[35] = (uint8_t)c->j;

        /* also the last 24 bytes are 'removed' frequently ... Here they are */
        end = (uint32_t)c->start + c->out_size;
        for (k = 0; k < 24; k++)
            changed |= patch(c, end - 24 + (uint32_t)k, ending[k]);

        pw_save_rip_special(c, "Time Cruncher 1.7 Exe-file", PWF_TimeCruncher, header, 36);
    }
    else
    {
        c->start -= 36;
        pw_save_rip(c, "Time Cruncher 1.7 Exe-file", PWF_TimeCruncher);
    }

    if (c->save_status == PW_GOOD)
        scan_advance(c, c->out_size - 42, changed); /* 36 should do but call it "just to be sure" :) */
}
