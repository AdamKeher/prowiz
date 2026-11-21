/*
 * DeltaMusic2.c - Delta Music 2.0 (Bent Nielsen)
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * The editor saves its replayer with the music, so a module is an image:
 *
 *   000-877 replayer code          AE2 period table (AA bytes, constant)
 *   878-AE1 channel structures     B8C player globals, BBB start speed
 *   BC6 ".FNL"                     BCA 64 x 16 arpeggios
 *   FCA 4 x (u16 loop, u16 length) FDA the 4 tracks (2 bytes per step)
 *   u32 BL, BL bytes of blocks (64 each)
 *   127 x u16 instrument offsets + u16 IL, IL bytes of instruments (88 each)
 *   u32 WL, WL bytes of waveforms (256 each)
 *   8 x u32 sample lengths + 32 zero bytes
 *   8 x u32 sample offsets, then the sample data
 *
 * The size is the end of the offset table plus the 8 sample lengths (the
 * offsets themselves are garbage in some files). NostalgicPlayer's format
 * description calls the length table "?"; it was identified by checking the
 * Modland files (93/98 exact, the others are damaged copies).
 *
 * In memory the player rewrites its globals and channel structures and
 * fills waveform 0 with noise on every tick; the layout stays the same.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define DM2_FNL      0xBC6
#define DM2_PERIODS  0xAE2
#define DM2_TRACKS   0xFCA
#define DM2_MAXBLOCK 0x100000u  /* blocks / waveforms / one sample: far above real ones */

/* the replayer's period table at AE2, the same in every module */
static const uint16_t dm2_periods[85] =
{
    0x0000, 0x1AC0, 0x1940, 0x17D0, 0x1680, 0x1530, 0x1400, 0x12E0, 0x11D0, 0x10D0, 0x0FE0, 0x0F00,
    0x0E20, 0x0D60, 0x0CA0, 0x0BE8, 0x0B40, 0x0A98, 0x0A00, 0x0970, 0x08E8, 0x0868, 0x07F0, 0x0780,
    0x0710, 0x06B0, 0x0650, 0x05F4, 0x05A0, 0x054C, 0x0500, 0x04B8, 0x0474, 0x0434, 0x03F8, 0x03C0,
    0x0388, 0x0358, 0x0328, 0x02FA, 0x02D0, 0x02A6, 0x0280, 0x025C, 0x023A, 0x021A, 0x01FC, 0x01E0,
    0x01C4, 0x01AC, 0x0194, 0x017D, 0x0168, 0x0153, 0x0140, 0x012E, 0x011D, 0x010D, 0x00FE, 0x00F0,
    0x00E2, 0x00D6, 0x00CA, 0x00BE, 0x00B4, 0x00AA, 0x00A0, 0x0097, 0x008F, 0x0087, 0x007F, 0x0078,
    0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071,
    0x0071,
};

/* bytes 4-F of the replayer (one ripped copy has 0-3 cleared) */
static const uint8_t dm2_code[12] =
{
    0x4A, 0x00, 0x67, 0x00, 0x03, 0xB4, 0x0C, 0x00, 0x00, 0x01, 0x67, 0x00
};

int pw_new_DeltaMusic2(pw_ctx *c)
{
    int64_t  start, off, trk;
    uint32_t k, s, len[4], loop, bl, nblocks, il, wl;
    uint64_t end;

    if (c->i < DM2_FNL || !pw_match(c, c->i, ".FNL", 4))
        return 0;
    start = (int64_t)c->i - DM2_FNL;

    for (k = 0; k < 85; k++)
        if (pw_rd16(c, start + DM2_PERIODS + 2 * k) != dm2_periods[k])
            return 0;
    for (k = 0; k < 12; k++)
        if (pw_rd8(c, start + 4 + k) != dm2_code[k])
            return 0;

    /* tracks: even lengths, loop inside */
    for (k = 0; k < 4; k++)
    {
        loop = pw_rd16(c, start + DM2_TRACKS + 4 * k);
        len[k] = pw_rd16(c, start + DM2_TRACKS + 4 * k + 2);
        if ((len[k] & 1) != 0 || (len[k] != 0 && loop >= len[k]))
            return 0;
    }
    trk = start + DM2_TRACKS + 16;
    off = trk + len[0] + len[1] + len[2] + len[3];

    bl = pw_rd32(c, off);
    if (bl > DM2_MAXBLOCK || (bl % 64) != 0)
        return 0;
    nblocks = bl / 64;
    /* every step names an existing block */
    for (k = 0; k < (uint32_t)(len[0] + len[1] + len[2] + len[3]); k += 2)
        if (pw_rd8(c, trk + k) >= nblocks)
            return 0;
    off += 4 + bl;

    /* instruments: offsets (instrument 0 at 0), the last word is their size */
    il = pw_rd16(c, off + 254);
    if ((il % 88) != 0)
        return 0;
    for (k = 0; k < 127; k++)
    {
        s = pw_rd16(c, off + 2 * k);
        if (s == il)
            break;      /* end of the used instruments */
        if ((s % 88) != 0 || s > il)
            return 0;
    }
    off += 256;
    for (k = 0; k < il / 88; k++)
        if (pw_rd8(c, off + 88 * k + 38) == 0xFF && pw_rd8(c, off + 88 * k + 39) > 7)
            return 0;   /* a sampled instrument names one of the 8 samples */
    off += il;

    wl = pw_rd32(c, off);
    if (wl > DM2_MAXBLOCK || (wl % 256) != 0)
        return 0;
    off += 4 + wl;

    /* 8 sample lengths, 32 zero bytes, 8 sample offsets, the data */
    end = (uint64_t)(off - start) + 64 + 32;
    for (k = 0; k < 8; k++)
    {
        s = pw_rd32(c, off + 4 * k);
        if (s > DM2_MAXBLOCK)
            return 0;
        end += s;
    }
    for (k = 32; k < 64; k++)
        if (pw_rd8(c, off + k) != 0)
            return 0;

    c->start = (int32_t)start;
    c->out_size = (uint32_t)end; /* < 16 MB: every part is capped */
    c->convert = PW_BAD;
    pw_save_rip(c, "Delta Music 2 module", PWF_DeltaMusic2);
    pw_skip_rip(c);
    return 1;
}
