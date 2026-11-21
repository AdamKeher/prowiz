/*
 * PreTracker.c - PreTracker (Pink/Abyss), format versions 18-1E
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * The song is one file of synthesis parameters, read in place by its
 * players. Layout (big-endian, offsets from the module start):
 *
 *   00 "PRT", 03 version
 *   04 POSD, 08 PATT, 0C INST, 10 WAVE: offsets of the four blocks
 *   14 song name (20), 28 author (20)
 *   3C restart, 3D patterns, 3E positions, 3F steps   (versions <= 1C)
 *   40 instruments, 41 waves, 42 wave order (24)      (order: versions > 19)
 *   5A subsongs                                       (versions 1D / 1E)
 *   POSD  versions <= 1C: positions x 4 x {pattern, transpose}
 *         1D / 1E: subsongs x {restart, patterns, steps, positions,
 *         u32 pattern offset from PATT}, then the positions of every
 *         subsong one after the other
 *   PATT  patterns x steps x 3 bytes per subsong
 *   INST  32 names (64 in 1E), instruments x 8 bytes of info, then for each
 *         instrument (info byte 7) x 3 bytes of instrument pattern
 *   WAVE  24 names, a pad byte to an even offset, waves x 42 bytes
 *
 * A name ends after its NUL, or after 23 bytes. There is no end marker: the
 * module ends after the last 42-byte wave record.
 *
 * Raspberry Casket (platon42's replayer) with PRETRACKER_SUPPORT_V1_5 patches
 * a 1E song in memory: POSD += 8, and 3C-3F get the bytes of the first
 * subsong header in the order [0],[1],[3],[2]. Such a song is saved restored.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define PRT_FORMAT      "PreTracker module"
#define PRT_MAX_WAVES   24
#define PRT_WAVE_SIZE   42
#define PRT_NAME_MAX    23
#define PRT_MAX_SUBSONG 31
#define PRT_MAX_OFFSET  0x01000000u /* far above any real song (55 KB) */

/* skips 'count' name records from 'p' (bytes outside the input read as 0) */
static int64_t prt_skip_names(const pw_ctx *c, int64_t p, unsigned count)
{
    unsigned n, k;

    for (n = 0; n < count; n++)
        for (k = 0; k < PRT_NAME_MAX; k++)
            if (pw_rd8(c, p++) == 0)
                break;
    return p;
}

/* every pattern number of 'npos' positions at 'p' is at most 'npat' */
static int prt_positions_ok(const pw_ctx *c, int64_t p, uint32_t npos, uint32_t npat)
{
    uint32_t k;

    for (k = 0; k < npos * 4; k++)
        if (pw_rd8(c, p + 2 * (int64_t)k) > npat)
            return 0;
    return 1;
}

int pw_new_PreTracker(pw_ctx *c)
{
    const int64_t s = c->i;
    const int64_t in_end = c->in_size;
    uint32_t ver, posd, patt, inst, wave, ninst, nwav, nnames, hdr;
    uint32_t posd_orig;
    uint64_t pat_bytes = 0;
    int64_t p, q, end;
    int rc_patched = 0;
    uint32_t k;

    /* signature: "PRT" and a known format version */
    if (pw_rd8(c, s + 1) != 'R' || pw_rd8(c, s + 2) != 'T')
        return 0;
    ver = pw_rd8(c, s + 3);
    if (ver < 0x18 || ver > 0x1E)
        return 0;

    hdr = ver <= 0x19 ? 0x43 : (ver <= 0x1C ? 0x5B : 0x5C);
    if (s + hdr > in_end)
        return 0;

    posd = pw_rd32(c, s + 4);
    patt = pw_rd32(c, s + 8);
    inst = pw_rd32(c, s + 12);
    wave = pw_rd32(c, s + 16);
    ninst = pw_rd8(c, s + 0x40);
    nwav = pw_rd8(c, s + 0x41);

    if (nwav == 0 || nwav > PRT_MAX_WAVES)
        return 0;
    if (posd < hdr || posd >= patt || patt > inst || inst >= wave || wave >= PRT_MAX_OFFSET)
        return 0;
    /* the header and the positions must be in the input */
    if (s + patt > in_end)
        return 0;

    if (ver >= 0x1D)
    {
        /* subsong headers, then the positions of all subsongs */
        uint32_t nsub = pw_rd8(c, s + 0x5A);
        uint32_t npos_total = 0;
        uint32_t i;

        if (nsub == 0 || nsub > PRT_MAX_SUBSONG)
            return 0;
        posd_orig = posd;
        if (ver == 0x1E && posd == 0x5C + 8 &&
            pw_rd8(c, s + 0x3C) == pw_rd8(c, s + 0x5C) && pw_rd8(c, s + 0x3D) == pw_rd8(c, s + 0x5D) &&
            pw_rd8(c, s + 0x3E) == pw_rd8(c, s + 0x5F) && pw_rd8(c, s + 0x3F) == pw_rd8(c, s + 0x5E))
        {
            /* Raspberry Casket's in-memory patch (a real 1E file has 0 at
             * 3C-3F, and its first subsong at least one pattern) */
            rc_patched = 1;
            posd_orig = 0x5C;
        }

        for (i = 0; i < nsub; i++)
        {
            int64_t h = s + posd_orig + 8 * (int64_t)i;
            uint32_t npat = pw_rd8(c, h + 1);
            uint32_t nst = pw_rd8(c, h + 2);
            uint32_t npos = pw_rd8(c, h + 3);

            if (npat == 0 || nst == 0 || npos == 0)
                return 0;
            /* the subsongs' patterns follow one another */
            if (pw_rd32(c, h + 4) != pat_bytes)
                return 0;
            pat_bytes += (uint64_t)npat * nst * 3;
            npos_total += npos;
        }
        if ((uint64_t)posd_orig + 8 * (uint64_t)nsub + 8 * (uint64_t)npos_total != patt)
            return 0;

        p = s + posd_orig + 8 * (int64_t)nsub;
        for (i = 0; i < nsub; i++)
        {
            int64_t h = s + posd_orig + 8 * (int64_t)i;
            uint32_t npos = pw_rd8(c, h + 3);

            if (!prt_positions_ok(c, p, npos, pw_rd8(c, h + 1)))
                return 0;
            p += 8 * (int64_t)npos;
        }
        nnames = ver == 0x1E ? 64 : 32;
    }
    else
    {
        uint32_t npat = pw_rd8(c, s + 0x3D);
        uint32_t npos = pw_rd8(c, s + 0x3E);
        uint32_t nst = pw_rd8(c, s + 0x3F);

        if (npat == 0 || nst == 0 || npos == 0)
            return 0;
        if ((uint64_t)posd + 8 * (uint64_t)npos != patt)
            return 0;
        if (!prt_positions_ok(c, s + posd, npos, npat))
            return 0;
        pat_bytes = (uint64_t)npat * nst * 3;
        nnames = 32;
    }
    if (ninst > nnames)
        return 0;
    if ((uint64_t)patt + pat_bytes != inst)
        return 0;

    /* instruments: names, 8 bytes of info each, then their patterns, up to
     * WAVE (not checkable when the input ends before WAVE: truncated) */
    p = prt_skip_names(c, s + inst, nnames);
    q = p + 8 * (int64_t)ninst;
    for (k = 0; k < ninst; k++)
        q += 3 * (int64_t)pw_rd8(c, p + 8 * (int64_t)k + 7);
    if (s + wave <= in_end && q != s + wave)
        return 0;

    /* waves: names, a pad to an even offset, the 42-byte records */
    p = prt_skip_names(c, s + wave, PRT_MAX_WAVES);
    if ((p - s) & 1)
        p++;
    end = p + PRT_WAVE_SIZE * (int64_t)nwav;
    for (k = 0; k < nwav; k++)
    {
        int64_t r = p + PRT_WAVE_SIZE * (int64_t)k;

        if (r + PRT_WAVE_SIZE > in_end)
            break;
        /* the sample length word is (length byte + 1) * 128 */
        if (pw_rd16(c, r + 0x0A) != (pw_rd8(c, r + 0x19) + 1) * 128)
            return 0;
    }

    c->start = (int32_t)s;
    c->out_size = (uint32_t)(end - s);
    c->convert = PW_BAD;

    if (rc_patched && end <= in_end)
    {
        uint8_t *buf = pw_copy_in(c, s, c->out_size);

        if (buf == NULL)
            return 1;
        buf[4] = 0;
        buf[5] = 0;
        buf[6] = 0;
        buf[7] = 0x5C;
        for (k = 0x3C; k < 0x40; k++)
            buf[k] = 0;
        pw_save_rip_data(c, PRT_FORMAT, PWF_PreTracker, buf, c->out_size, 1);
        free(buf);
    }
    else
        pw_save_rip(c, PRT_FORMAT, PWF_PreTracker);

    pw_skip_rip(c);
    return 1;
}
