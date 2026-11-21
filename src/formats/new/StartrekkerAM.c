/*
 * StartrekkerAM.c - Startrekker AM synth files (.nt / Audio Sculpture .as),
 *                   and Startrekker 8 channels (FLT8 / EXO8)
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * Startrekker 1.2+ (Exolon/Fairlight) keeps the parameters of its AM synth
 * instruments in a second file, "<module name>.nt"; Audio Sculpture uses
 * ".as" with the same layout. Players look for exactly that name, so the
 * pair is saved as "N.mod" + "N.mod.nt".
 *
 *   000 "ST1.2 ModuleINFO" / "ST1.3 ModuleINFO" / "AudioSculpture10"
 *   010 80 bytes of settings
 *   090 + 78 * i  instrument i + 1 (i < 31): "AM", 4 zero bytes, 15 u16
 *                 (levels, speeds, +1A waveform 0-3, ...), padding
 *   F90 ST1.3 / AS: 16 macros x 110 bytes
 *
 * Saved sizes (Modland files): 1008 for ST1.2 (the last 78 bytes are editor
 * memory), 2090 for ST1.3 and Audio Sculpture.
 *
 * Nothing in the module points at its .nt: the companion is the .nt in the
 * input whose AM instruments are instruments the module leaves empty (see
 * nt_score). A .nt no module claimed is saved on its own.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "new_formats.h"

#define NT_RECORDS   0x90
#define NT_RECSIZE   0x78
#define NT_SIZE_12   0x1008u
#define NT_SIZE_13   0x2090u

/* canonical size of a .nt starting at 'off', 0 if there is none */
static uint32_t nt_kind(const pw_ctx *c, int64_t off)
{
    if (pw_match(c, off, "ST1.2 ModuleINFO", 16))
        return NT_SIZE_12;
    if (pw_match(c, off, "ST1.3 ModuleINFO", 16) || pw_match(c, off, "AudioSculpture10", 16))
        return NT_SIZE_13;
    return 0;
}

/* The settings between the ID and the records: 32 u16 below 100, then 32
 * bytes below 80 (all 49 Modland files: 0018 words, 04 bytes, "0" chars).
 * This keeps the ID string inside the Startrekker program from counting. */
static int nt_settings_ok(const pw_ctx *c, int64_t off)
{
    int64_t k;
    for (k = 0x10; k < 0x50; k += 2)
        if (pw_rd8(c, off + k) != 0)
            return 0;
    for (k = 0x50; k < NT_RECORDS; k++)
        if (pw_rd8(c, off + k) > 0x7F)
            return 0;
    return 1;
}

/* Bit i set: instrument i + 1 is an AM instrument. Returns 0 for a record
 * marked "AM" that is not a valid one (*bad set). Only records that start
 * inside the input count. */
static uint32_t nt_am_mask(const pw_ctx *c, int64_t off, int *bad)
{
    uint32_t mask = 0, i;
    int64_t  r;

    *bad = 0;
    for (i = 0; i < 31; i++)
    {
        r = off + NT_RECORDS + (int64_t)NT_RECSIZE * i;
        if (r + 0x1C > (int64_t)c->in_size)
            break;
        if (pw_rd16(c, r) != 0x414D) /* "AM" */
            continue;
        if (pw_rd32(c, r + 2) != 0 || pw_rd16(c, r + 0x1A) > 3)
        {
            *bad = 1;
            return 0;
        }
        mask |= 1u << i;
    }
    return mask;
}

static int popcount31(uint32_t m)
{
    int n = 0;
    for (; m != 0; m &= m - 1)
        n++;
    return n;
}

/* A .nt fits a module when every one of its AM instruments is (nearly) empty
 * in the module, and at least one is empty: AM instruments have length 0
 * (once 32 words) and usually volume 64. Score: AM instruments that look
 * like AM slots in the module, counting those with a volume twice. */
static int nt_score(uint32_t am, uint32_t small, uint32_t zero, uint32_t zerovol)
{
    if (am == 0 || (am & ~small) != 0 || (am & zero) == 0)
        return -1;
    return 2 * popcount31(am & zerovol) + popcount31(am & zero);
}

/* Find and save the .nt of the module just saved (c->start .. + c->out_size;
 * its 31 instrument headers at + 20 + 30 * i) as companion "<n>.<ext>". */
static void save_nt_companion(pw_ctx *c, const char *ext)
{
    int64_t  mstart = c->start, mend = (int64_t)c->start + c->out_size;
    int64_t  p, best = -1;
    uint64_t bestdist = 0;
    uint32_t small = 0, zero = 0, zerovol = 0, am, size, avail, need, i;
    int      score, bestscore = -1, bad, pass;

    if (c->save_status != PW_GOOD)
        return;

    for (i = 0; i < 31; i++)
    {
        uint32_t len = pw_rd16(c, mstart + 42 + 30 * i);
        if (len <= 32)
            small |= 1u << i;
        if (len == 0)
        {
            zero |= 1u << i;
            if (pw_rd8(c, mstart + 45 + 30 * i) != 0)
                zerovol |= 1u << i;
        }
    }
    if (zero == 0)
        return;

    /* every .nt in the input: the best score, then the nearest. This runs
     * once per FLT4 / EXO4 / FLT8 / EXO8 module, so the first bytes of the
     * IDs are located with memchr() inside the input buffer (in bounds:
     * [0, in_size)); everything else reads through pw_rd*(). */
    for (pass = 0; pass < 2; pass++)
    {
        const int ch = pass == 0 ? 'S' : 'A';
        size_t    from = 0, n = c->in_size > 16 ? (size_t)c->in_size - 15 : 0;

        while (from < n)
        {
            const uint8_t *hit = (const uint8_t *)memchr(c->in + from, ch, n - from);
            uint64_t dist;

            if (hit == NULL)
                break;
            p = hit - c->in;
            from = (size_t)p + 1;
            if (p >= mstart && p < mend)
                continue;
            if (nt_kind(c, p) == 0 || pw_claimed(c, (uint32_t)p) || !nt_settings_ok(c, p))
                continue;
            am = nt_am_mask(c, p, &bad);
            if (bad)
                continue;
            score = nt_score(am, small, zero, zerovol);
            if (score < 0)
                continue;
            dist = (uint64_t)(p < mstart ? mstart - p : p - mstart);
            if (score > bestscore || (score == bestscore && dist < bestdist))
            {
                best = p;
                bestscore = score;
                bestdist = dist;
            }
        }
    }
    if (best < 0)
        return;

    /* its size, cut to the input when every AM record is still in it */
    size = nt_kind(c, best);
    avail = (uint32_t)((int64_t)c->in_size - best);
    if (size > avail)
    {
        am = nt_am_mask(c, best, &bad);
        need = NT_RECORDS;
        for (i = 0; i < 31; i++)
            if (am & (1u << i))
                need = NT_RECORDS + NT_RECSIZE * (i + 1);
        if (need > avail)
            return;
        size = avail;
    }
    pw_save_companion_in(c, ext, (uint32_t)best, size);
}

/* "<module extension>.nt", e.g. "mod.nt" */
static void companion_ext(const pw_ctx *c, int fmt_id, char *buf, size_t n)
{
    snprintf(buf, n, "%s.nt", pw_ext_of(c, fmt_id));
}

void pw_new_StartrekkerAM_companion(pw_ctx *c)
{
    char ext[PW_EXT_LEN + 8];

    companion_ext(c, PWF_Protracker, ext, sizeof ext);
    save_nt_companion(c, ext);
}

/* a .nt on its own: 'S' "ST1.x ModuleINFO" or 'A' "AudioSculpture10" */
int pw_new_StartrekkerNT(pw_ctx *c)
{
    uint32_t size;
    int      bad;
    const char *name;

    size = nt_kind(c, c->i);
    if (size == 0 || pw_claimed(c, c->i))
        return 0;
    if (!nt_settings_ok(c, c->i))
        return 0;
    (void)nt_am_mask(c, c->i, &bad);
    if (bad)
        return 0;

    if (pw_rd8(c, c->i) == 'A')
        name = "Audio Sculpture AM instruments (.as)";
    else if (pw_rd8(c, (int64_t)c->i + 4) == '3')
        name = "Startrekker 1.3 AM instruments (.nt)";
    else
        name = "Startrekker 1.2 AM instruments (.nt)";

    c->start = (int32_t)c->i;
    c->out_size = size;
    c->convert = PW_BAD;
    pw_save_rip(c, name, PWF_StartrekkerNT);
    pw_skip_rip(c);
    return 1;
}

/* Startrekker 8 channels: a 31-instrument module with "FLT8" / "EXO8" at
 * 438, its patterns stored as pairs of 4-channel patterns (channels 1-4,
 * then 5-8); the order list holds the number of the first of each pair. */
int pw_new_Startrekker8(pw_ctx *c)
{
    int64_t  start;
    uint32_t i, len, maxord, npat4, wss;
    uint32_t k;

    if (c->i < 1080 || !(pw_match(c, c->i, "FLT8", 4) || pw_match(c, c->i, "EXO8", 4)))
        return 0;
    start = (int64_t)c->i - 1080;

    wss = 0;
    for (i = 0; i < 31; i++)
    {
        int64_t h = start + 20 + 30 * (int64_t)i;
        len = pw_rd16(c, h + 22);
        if (pw_test_smps((int32_t)(len * 2), (int32_t)(pw_rd16(c, h + 26) * 2),
                         (int32_t)(pw_rd16(c, h + 28) * 2),
                         (uint8_t)pw_rd8(c, h + 25), (uint8_t)pw_rd8(c, h + 24)) == PW_BAD)
            return 0;
        wss += len * 2;
    }
    len = pw_rd8(c, start + 950);
    if (len == 0 || len > 128)
        return 0;
    maxord = 0;
    for (i = 0; i < 128; i++)
    {
        uint32_t o = pw_rd8(c, start + 952 + i);
        if (o > 127)
            return 0;
        if (i < len && (o & 1) != 0)
            return 0;   /* a played order names the first of a pair */
        if (o > maxord)
            maxord = o;
    }
    npat4 = 2 * ((maxord >> 1) + 1);

    /* note data of the played patterns (unused ones can hold anything):
     * instrument < 32, period 0 or a real one */
    for (i = 0; i < len; i++)
    {
        uint32_t first = pw_rd8(c, start + 952 + i);
        for (k = 0; k < 512; k++)
        {
            int64_t  e = start + 1084 + 1024 * (int64_t)first + 4 * (int64_t)k;
            uint32_t b0 = pw_rd8(c, e);
            uint32_t per = ((b0 & 0x0F) << 8) | pw_rd8(c, e + 1);
            if (e + 4 > (int64_t)c->in_size)
                break;  /* a truncated module is still reported */
            if (b0 > 0x13 || (per > 0 && per < 0x1C))
                return 0;
        }
    }

    c->start = (int32_t)start;
    c->out_size = 1084 + npat4 * 1024 + wss;
    c->convert = PW_BAD;
    pw_save_rip(c, "Startrekker 8 channels module", PWF_Startrekker8);
    if (c->save_status == PW_GOOD)
    {
        char ext[PW_EXT_LEN + 8];
        companion_ext(c, PWF_Startrekker8, ext, sizeof ext);
        save_nt_companion(c, ext);
    }
    pw_skip_rip(c);
    return 1;
}
