/*
 * TNMCruncher.c - TNM Cruncher 1.1 Exe-file (upstream r/TNMCruncher.c).
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

int16_t pw_test_TNMCruncher11(pw_ctx *c)
{
    static const uint8_t code[16] = {
        0x4B, 0xFA, 0x01, 0x9A, 0x41, 0xFA, 0xFF, 0xE6,
        0x20, 0x50, 0xD1, 0xC8, 0xD1, 0xC8, 0x22, 0x50};
    static const uint8_t hunk[16] = {
        0x00, 0x00, 0x03, 0xF3, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00};

    c->start = (int32_t)c->i;

    if (!match(c, c->start + 16, code, 16))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 428) * 256 * 256 * 256) +
            (IN(c->start + 429) * 256 * 256) +
            (IN(c->start + 430) * 256) +
            IN(c->start + 431));

    c->l += 680;

    /* unpacked size */
    c->m = (((uint32_t)IN(c->start + 432) * 256 * 256 * 256) +
            (IN(c->start + 433) * 256 * 256) +
            (IN(c->start + 434) * 256) +
            IN(c->start + 435));

    if (c->i >= 40)
    {
        if (!match(c, c->start - 40, hunk, 16))
            c->amiga_exe_header = PW_BAD;
        else
            c->amiga_exe_header = PW_GOOD;
    }
    else
        c->amiga_exe_header = PW_BAD;

    return PW_GOOD;
    /* c->l is the size of the pack */
}

void pw_rip_TNMCruncher11(pw_ctx *c)
{
    /* c->l is still the whole size */
    /* c->m is the decrunched size  (necessary to rebuild header) */
    uint8_t header[40];
    uint32_t end;
    int changed = 0;

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 40;
        memset(header, 0, sizeof header);
        header[2] = header[34] = 0x03;
        header[3] = 0xF3;
        header[11] = 0x03;
        header[19] = 0x02;
        header[23] = header[39] = 0x9B;
        header[35] = 0xE9;

        /* the original copied the bytes of PW_j from memory (x86 order) */
        c->j = c->l - 680;
        c->j /= 4;
        header[24] = (uint8_t)(c->j >> 24);
        header[25] = (uint8_t)(c->j >> 16);
        header[26] = (uint8_t)(c->j >> 8);
        header[27] = (uint8_t)c->j;
        c->j = c->m / 4;
        header[28] = (uint8_t)(c->j >> 24);
        header[29] = (uint8_t)(c->j >> 16);
        header[30] = (uint8_t)(c->j >> 8);
        header[31] = (uint8_t)c->j;

        /* also the last 12 bytes are 'removed' frequently ... Here they are */
        end = (uint32_t)c->start + c->out_size;
        changed |= patch(c, end - 12, 0x00);
        changed |= patch(c, end - 11, 0x00);
        changed |= patch(c, end - 10, 0x03);
        changed |= patch(c, end - 9, 0xEB);

        changed |= patch(c, end - 8, (uint8_t)(c->j >> 24));
        changed |= patch(c, end - 7, (uint8_t)(c->j >> 16));
        changed |= patch(c, end - 6, (uint8_t)(c->j >> 8));
        changed |= patch(c, end - 5, (uint8_t)c->j);

        changed |= patch(c, end - 4, 0x00);
        changed |= patch(c, end - 3, 0x00);
        changed |= patch(c, end - 2, 0x03);
        changed |= patch(c, end - 1, 0xF2);

        pw_save_rip_special(c, "TNM Cruncher 1.1 Exe-file", PWF_TNMCruncher, header, 40);
    }
    else
    {
        c->start -= 40;
        pw_save_rip(c, "TNM Cruncher 1.1 Exe-file", PWF_TNMCruncher);
    }

    if (c->save_status == PW_GOOD)
        scan_advance(c, c->out_size - 44, changed); /* 40 should do but call it "just to be sure" :) */
}
