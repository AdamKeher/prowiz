/*
 * TetraPack1.c - Tetrapack 1.01 and 1.02 Exe-files (upstream r/TetraPack1.c).
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

static const uint8_t hunk[16] = {
    0x00, 0x00, 0x03, 0xF3, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};

int16_t pw_test_Tetrapack102(pw_ctx *c)
{
    static const uint8_t code[11] = {
        0x20, 0x20, 0xE2, 0x88, 0x66, 0x02, 0x61, 0x34, 0x65, 0x7E, 0x72};
    static const uint8_t code2[2] = {0x76, 0x01};

    c->start = (int32_t)c->i;

    if (!match(c, c->start + 20, code, 11) ||
        !match(c, c->start + 32, code2, 2))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 6) * 256 * 256 * 256) +
            (IN(c->start + 7) * 256 * 256) +
            (IN(c->start + 8) * 256) +
            IN(c->start + 9));

    c->l += 268;

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

int16_t pw_test_Tetrapack101(pw_ctx *c)
{
    static const uint8_t code[11] = {
        0x20, 0x20, 0xE2, 0x88, 0x66, 0x02, 0x61, 0x34, 0x65, 0x6C, 0x72};
    static const uint8_t code2[1] = {0x76};

    c->start = (int32_t)c->i;

    if (!match(c, c->start + 20, code, 11) ||
        !match(c, c->start + 32, code2, 1))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 6) * 256 * 256 * 256) +
            (IN(c->start + 7) * 256 * 256) +
            (IN(c->start + 8) * 256) +
            IN(c->start + 9));

    c->l += 260;

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

/* builds the missing 32 byte hunk header (PW_j is a global upstream) */
static void build_header(pw_ctx *c, uint8_t header[32])
{
    memset(header, 0, 32);
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
}

void pw_rip_Tetrapack102(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t header[32];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 32;
        build_header(c, header);
        pw_save_rip_special(c, "Tetrapack 1.02 Exe-file", PWF_TPACK102, header, 32);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "Tetrapack 1.02 Exe-file", PWF_TPACK102);
    }

    if (c->save_status == PW_GOOD)
        scan_advance(c, c->out_size - 36); /* 32 should do but call it "just to be sure" :) */
}

void pw_rip_Tetrapack101(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t header[32];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 32;
        build_header(c, header);
        pw_save_rip_special(c, "Tetrapack 1.01 Exe-file", PWF_TPACK101, header, 32);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "Tetrapack 1.01 Exe-file", PWF_TPACK101);
    }

    if (c->save_status == PW_GOOD)
        c->i += 1;
}
