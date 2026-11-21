/*
 * SidMon2.c - SidMon 2 ("SidMon II - The MIDI Version", Michael Kleps)
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * One file, no player code:
 *
 *   00 u16 MIDI mode            0E u32 PT  position table (4 x positions)
 *   02 u8  positions - 1        12 u32 NT  note transposes   (= PT)
 *   03 u8  speed                16 u32 IT  instr. transposes (= PT)
 *   04 u16 sample info size     1A u32 I   instruments (32 bytes each)
 *   06 u32 1C (ID length)       1E u32 WL  waveform lists (16 bytes each)
 *   0A u32 4  (song length)     22 u32 A   arpeggios (16 bytes each)
 *   3A "SIDMON II - THE MIDI    26 u32 V   vibratos (16 bytes each)
 *       VERSION"                2A u32 SI  sample infos (64 bytes each)
 *   56 4 bytes                  2E u32 TT  track table (u16 offsets into T)
 *   5A the blocks, in order     32 u32 T   tracks
 *                               36 u32 not a size (ignored by every player)
 *
 * After the blocks: a pad byte if the end is odd, then the sample data, the
 * sum of the sample lengths (u16 words at SI + 4 of each 64-byte entry). This
 * is the layout read by Flod's S2Player and NostalgicPlayer's SidMon20 (a
 * port of the original player); the original Pro-Wizard's sidmon2.c (never
 * called by its scanner) saved only 58 bytes and treated the long at 36 as a
 * block size.
 *
 * In memory the player stores absolute sample addresses at SI + 0 and its
 * "negate" effect inverts sample bytes in place: the layout and the size are
 * unchanged, so such copies are ripped as they are.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define SM2_ID       "SIDMON II - THE MIDI VERSION"
#define SM2_ID_OFF   0x3A
#define SM2_DATA     0x5A
#define SM2_MAXBLOCK 0x100000u  /* far above any real block (tracks: 24 KB) */

int pw_new_SidMon2(pw_ctx *c)
{
    int64_t  start, off;
    uint32_t sz[10];            /* PT NT IT I WL A V SI TT T */
    uint32_t k, n, prev, nsmp, ntracks, nwave, maxpos;
    uint64_t end;

    /* the ID is at 3A in the module; c->i is its 'S' */
    if (c->i < SM2_ID_OFF || !pw_match(c, c->i, SM2_ID, 28))
        return 0;
    start = (int64_t)c->i - SM2_ID_OFF;

    if (pw_rd32(c, start + 0x06) != 0x1C || pw_rd32(c, start + 0x0A) != 4)
        return 0;
    for (k = 0; k < 10; k++)
    {
        sz[k] = pw_rd32(c, start + 0x0E + 4 * k);
        if (sz[k] > SM2_MAXBLOCK)
            return 0;
    }

    /* position table and both transpose tables: 4 voices x positions */
    if (sz[0] != 4 * (pw_rd8(c, start + 2) + 1) || sz[1] != sz[0] || sz[2] != sz[0])
        return 0;
    /* fixed-size records */
    if (sz[3] == 0 || (sz[3] % 32) != 0 || (sz[4] % 16) != 0 ||
        (sz[5] % 16) != 0 || (sz[6] % 16) != 0)
        return 0;
    if (sz[7] != pw_rd16(c, start + 4) || (sz[7] % 64) != 0)
        return 0;
    if (sz[8] < 2 || (sz[8] & 1) != 0 || sz[9] == 0)
        return 0;

    /* track table: offsets into the tracks, ascending from 0 */
    off = start + SM2_DATA;
    for (k = 0; k < 8; k++)
        off += sz[k];
    ntracks = sz[8] / 2;
    prev = 0;
    for (k = 0; k < ntracks; k++)
    {
        n = pw_rd16(c, off + 2 * k);
        if ((k == 0 && n != 0) || n < prev || n >= sz[9])
            return 0;
        prev = n;
    }

    /* every position names an existing track */
    maxpos = 0;
    for (k = 0; k < sz[0]; k++)
    {
        n = pw_rd8(c, start + SM2_DATA + k);
        if (n > maxpos)
            maxpos = n;
    }
    if (maxpos >= ntracks)
        return 0;

    /* every instrument names an existing waveform list */
    nwave = sz[4] / 16;
    off = start + SM2_DATA + sz[0] + sz[1] + sz[2];
    for (k = 0; k < sz[3] / 32; k++)
        if (pw_rd8(c, off + 32 * k) >= nwave)
            return 0;

    /* size: header, blocks, pad to even, sample data */
    end = SM2_DATA;
    for (k = 0; k < 10; k++)
        end += sz[k];
    end += end & 1;
    nsmp = sz[7] / 64;
    off = start + SM2_DATA + sz[0] + sz[1] + sz[2] + sz[3] + sz[4] + sz[5] + sz[6];
    for (k = 0; k < nsmp; k++)
        end += 2 * (uint64_t)pw_rd16(c, off + 64 * k + 4);

    c->start = (int32_t)start;
    c->out_size = (uint32_t)end; /* < 16 MB: 10 capped blocks + 1023 x 128 KB */
    c->convert = PW_BAD;
    pw_save_rip(c, "SidMon 2 module", PWF_Sidmon2);
    pw_skip_rip(c);
    return 1;
}
