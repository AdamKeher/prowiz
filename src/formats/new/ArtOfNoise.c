/*
 * ArtOfNoise.c - Art of Noise (Bastian Spiegel, Twice/Lego), AON4 and AON8
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * Layout (big-endian; offsets from the module start and lengths in hex):
 *
 *   00 "AON4" (4 channels) or "AON8" (8 channels)
 *   04 42 bytes of text, usually "artofnoise by bastian spiegel (twice/lego)";
 *      "Chorus by ..." and other spellings exist, so it is not checked
 *   2E chunks of {ID, u32 length, data}, no padding between them:
 *      NAME AUTH DATE RMRK (optional text), INFO (4 bytes: 34, song length,
 *      restart, 0), ARPG (40), PLST ((song length + 1) & ~1 pattern
 *      numbers), PATT (patterns x 400 / 800: 64 rows x channels x 4 bytes),
 *      INST (instruments x 20, at most 61), INAM (optional, names x 20),
 *      WLEN (100: 64 u32 wave lengths), WAVE (the waves: sum of WLEN)
 *
 * The module ends after WAVE. The editor pads the file with '?' to a
 * multiple of 4 bytes; those bytes are part of the rip when present.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define AON_FIRST_CHUNK 0x2E
#define AON_MAX_LEN     0x01000000u /* any chunk: far above real ones */
#define AON_MAX_TEXT    0x10000u
#define AON_MAX_INST    61

enum { AON_NAME, AON_AUTH, AON_DATE, AON_RMRK, AON_INFO, AON_ARPG, AON_PLST,
       AON_PATT, AON_INST, AON_INAM, AON_WLEN, AON_WAVE, AON_NCHUNKS };

static int aon_chunk(uint32_t id)
{
    static const char ids[AON_NCHUNKS][5] = {
        "NAME", "AUTH", "DATE", "RMRK", "INFO", "ARPG",
        "PLST", "PATT", "INST", "INAM", "WLEN", "WAVE"
    };
    int k;

    for (k = 0; k < AON_NCHUNKS; k++)
        if (id == ((uint32_t)(uint8_t)ids[k][0] << 24 | (uint32_t)(uint8_t)ids[k][1] << 16 |
                   (uint32_t)(uint8_t)ids[k][2] << 8 | (uint32_t)(uint8_t)ids[k][3]))
            return k;
    return -1;
}

#define SEEN(k) (seen & (1u << (k)))

int pw_new_ArtOfNoise(pw_ctx *c)
{
    const int64_t s = c->i;
    const int64_t in_end = c->in_size;
    uint32_t v, pattern_size, seen = 0, npos = 0, npat = 0;
    uint64_t wave_len = 0;
    int wave_len_known = 0;
    int64_t pos, plst = 0, end = -1;
    int n;

    /* signature: "AON4" / "AON8" and a known first chunk */
    if (pw_rd8(c, s + 1) != 'O' || pw_rd8(c, s + 2) != 'N')
        return 0;
    v = pw_rd8(c, s + 3);
    if (v != '4' && v != '8')
        return 0;
    if (s + AON_FIRST_CHUNK + 8 > in_end || aon_chunk(pw_rd32(c, s + AON_FIRST_CHUNK)) < 0)
        return 0;
    pattern_size = v == '4' ? 0x400 : 0x800;

    pos = s + AON_FIRST_CHUNK;
    for (n = 0; n < AON_NCHUNKS; n++)
    {
        int64_t body = pos + 8;
        uint32_t len;
        int k;

        if (body > in_end)
        {
            /* the input ends between two chunks: at least a header more */
            end = body;
            break;
        }
        k = aon_chunk(pw_rd32(c, pos));
        len = pw_rd32(c, pos + 4);
        if (k < 0 || SEEN(k) || len > AON_MAX_LEN)
            return 0;
        seen |= 1u << k;

        switch (k)
        {
        case AON_NAME:
        case AON_AUTH:
        case AON_DATE:
        case AON_RMRK:
            if (len > AON_MAX_TEXT)
                return 0;
            break;
        case AON_INFO:
            npos = pw_rd8(c, body + 1);
            if (len != 4 || pw_rd8(c, body) != 0x34 || npos == 0)
                return 0;
            break;
        case AON_ARPG:
            if (len != 0x40)
                return 0;
            break;
        case AON_PLST:
            if (!SEEN(AON_INFO) || len != ((npos + 1) & ~1u))
                return 0;
            plst = body;
            break;
        case AON_PATT:
        {
            uint32_t p;

            if (!SEEN(AON_PLST) || len == 0 || len % pattern_size != 0)
                return 0;
            npat = len / pattern_size;
            for (p = 0; p < npos; p++)
                if (plst + p < in_end && pw_rd8(c, plst + p) >= npat)
                    return 0;
            break;
        }
        case AON_INST:
        {
            uint32_t i;

            if (len == 0 || len % 0x20 != 0 || len / 0x20 > AON_MAX_INST)
                return 0;
            /* instrument type: 0 sample, 1 synth */
            for (i = 0; i < len / 0x20; i++)
                if (pw_rd8(c, body + 0x20 * (int64_t)i) > 1)
                    return 0;
            break;
        }
        case AON_INAM:
            if (len % 0x20 != 0 || len / 0x20 > AON_MAX_INST)
                return 0;
            break;
        case AON_WLEN:
        {
            int w;

            if (len != 0x100)
                return 0;
            for (w = 0; w < 64; w++)
                wave_len += pw_rd32(c, body + 4 * w);
            wave_len_known = body + 0x100 <= in_end;
            break;
        }
        case AON_WAVE:
            if (!SEEN(AON_WLEN) || !SEEN(AON_INST) || !SEEN(AON_PATT) || !SEEN(AON_ARPG))
                return 0;
            if (wave_len_known && len != wave_len)
                return 0;
            break;
        default:
            break;
        }

        pos = body + len;
        if (k == AON_WAVE)
        {
            end = pos;
            break;
        }
        if (pos > in_end)
        {
            /* a chunk before WAVE runs past the end: the size is at least
             * this much */
            end = pos;
            break;
        }
    }

    /* a module needs its song (INFO, PLST, PATT) to be recognised at all */
    if (end < 0 || !SEEN(AON_INFO) || !SEEN(AON_PLST) || !SEEN(AON_PATT))
        return 0;
    if (SEEN(AON_WAVE) && end <= in_end)
    {
        /* the editor's '?' pad to a multiple of 4 */
        int pad = (int)((4 - ((end - s) & 3)) & 3);
        int k;

        for (k = 0; k < pad; k++)
            if (end + k >= in_end || pw_rd8(c, end + k) != '?')
                break;
        if (k == pad)
            end += pad;
    }

    c->start = (int32_t)s;
    c->out_size = (uint32_t)(end - s);
    c->convert = PW_BAD;
    pw_save_rip(c, v == '4' ? "Art of Noise (AON4) module" : "Art of Noise (AON8) module",
                PWF_ArtOfNoise);
    pw_skip_rip(c);
    return 1;
}
