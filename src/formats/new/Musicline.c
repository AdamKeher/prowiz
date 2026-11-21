/*
 * Musicline.c - Musicline Editor (Conny and Christian Cyreus and others)
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * Layout (big-endian, offsets from the module start):
 *
 *   00 "MLEDMODL", 08 u32 length + that many bytes, then "VERS" + u32
 *      length + version. Very old files have "VERS" at 08 directly.
 *   then chunks of {ID, u32 length, data}, unaligned, read one after the
 *   other by the player until an ID it does not know:
 *     TUNE  40 bytes (title, tempo, speed, groove, volume, play mode,
 *           channels), then channels x u32 sizes, then the sequences. The
 *           player reads exactly that much; files up to version 1.06 store
 *           a length as if all 8 sizes were there (16 bytes too much for 4
 *           channels, 32 for none), so the length is not used for the walk.
 *     PART  u16 number, packed rows               (length bytes)
 *     ARPG  u16 number, 6-byte rows               (length bytes)
 *     INST  the instrument, at most 206 bytes     (length bytes)
 *     SMPL  6 bytes (u32 unpacked length, delta command, pad), then the
 *           50-byte sample header and the (maybe delta-packed) data: the
 *           length counts the last two only, so the chunk takes 8 + 6 +
 *           length bytes
 *     INFO  title, author, date, duration, 5 lines of text (length bytes)
 *
 * The module ends at the first ID that is not one of those (or one whose
 * fields are impossible), as the player's loader stops there. The first
 * chunk must be a TUNE: the player refuses a module without one.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define ML_FORMAT       "Musicline Editor module"
#define ML_ID(a, b, c_, d) ((uint32_t)(a) << 24 | (uint32_t)(b) << 16 | (uint32_t)(c_) << 8 | (uint32_t)(d))
#define ML_VERS         ML_ID('V', 'E', 'R', 'S')
#define ML_TUNE         ML_ID('T', 'U', 'N', 'E')
#define ML_PART         ML_ID('P', 'A', 'R', 'T')
#define ML_ARPG         ML_ID('A', 'R', 'P', 'G')
#define ML_INST         ML_ID('I', 'N', 'S', 'T')
#define ML_SMPL         ML_ID('S', 'M', 'P', 'L')
#define ML_INFO         ML_ID('I', 'N', 'F', 'O')

#define ML_MAX_HEADER   0x100u      /* MODL and VERS data: 4-40 and 6 in practice */
#define ML_MAX_CHUNKS   4096        /* 256 tunes + 1024 parts + 256 arpeggios, ...  */
#define ML_TUNE_HEAD    40
#define ML_MAX_CHANNELS 8
#define ML_MAX_SEQUENCE 512         /* 256 u16 steps per channel              */
#define ML_MAX_PART     (2 + 128 * 13 + 2) /* number, 128 packed rows, end    */
#define ML_MAX_ARPG     (2 + 6 * 128)
#define ML_INST_SIZE    206
#define ML_SMPL_HEAD    50
#define ML_MAX_SAMPLE   0x01000000u
#define ML_MAX_INFO     0x10000u

/* bytes the player takes for the chunk at 'pos' (after its 8-byte header),
 * or -1 if its fields are impossible */
static int64_t ml_chunk_size(const pw_ctx *c, int64_t pos, uint32_t id)
{
    const int64_t body = pos + 8;
    const uint32_t len = pw_rd32(c, pos + 4);

    switch (id)
    {
    case ML_TUNE:
    {
        uint32_t ch = pw_rd8(c, body + 39);
        uint32_t k;
        int64_t size = ML_TUNE_HEAD;

        if (pw_rd8(c, body + 38) > 1 || ch > ML_MAX_CHANNELS)
            return -1;
        for (k = 0; k < ch; k++)
        {
            uint32_t seq = pw_rd32(c, body + ML_TUNE_HEAD + 4 * (int64_t)k);

            if (seq > ML_MAX_SEQUENCE || (seq & 1))
                return -1;
            size += 4 + (int64_t)seq;
        }
        /* the length is exact, or (old files) counts 8 size longwords */
        if (len != size && len != size + 4 * (int64_t)(ML_MAX_CHANNELS - ch))
            return -1;
        return size;
    }
    case ML_PART:
        if (pw_rd16(c, body) >= 1024 || len < 3 || len > ML_MAX_PART)
            return -1;
        return len;
    case ML_ARPG:
        if (pw_rd16(c, body) >= 256 || len < 2 || len > ML_MAX_ARPG || (len - 2) % 6 != 0)
            return -1;
        return len;
    case ML_INST:
        if (len == 0 || len > ML_INST_SIZE)
            return -1;
        return len;
    case ML_SMPL:
        /* unpacked length, then the data: as is, or delta-packed (smaller) */
        if (len < ML_SMPL_HEAD || pw_rd32(c, body) > ML_MAX_SAMPLE ||
            len - ML_SMPL_HEAD > pw_rd32(c, body))
            return -1;
        return 6 + (int64_t)len;
    case ML_INFO:
        if (len > ML_MAX_INFO)
            return -1;
        return len;
    default:
        return -1;
    }
}

int pw_new_Musicline(pw_ctx *c)
{
    const int64_t s = c->i;
    const int64_t in_end = c->in_size;
    int64_t pos, size;
    int n;

    if (!pw_match(c, s, "MLEDMODL", 8))
        return 0;

    /* MODL data (absent in very old files), then VERS */
    pos = s + 8;
    if (pw_rd32(c, pos) != ML_VERS)
    {
        uint32_t len = pw_rd32(c, pos);

        if (len > ML_MAX_HEADER)
            return 0;
        pos += 4 + len;
        if (pw_rd32(c, pos) != ML_VERS)
            return 0;
    }
    if (pw_rd32(c, pos + 4) > ML_MAX_HEADER)
        return 0;
    pos += 8 + pw_rd32(c, pos + 4);

    /* the first chunk is a TUNE */
    if (pos + 8 + ML_TUNE_HEAD > in_end || pw_rd32(c, pos) != ML_TUNE ||
        ml_chunk_size(c, pos, ML_TUNE) < 0)
        return 0;

    for (n = 0; n < ML_MAX_CHUNKS; n++)
    {
        if (pos + 8 > in_end)
            break; /* the file ends here (or the input does) */
        size = ml_chunk_size(c, pos, pw_rd32(c, pos));
        if (size < 0)
            break; /* not a chunk: the module ends */
        pos += 8 + size;
        if (pos > in_end)
            break; /* runs past the input: truncated */
    }

    c->start = (int32_t)s;
    c->out_size = (uint32_t)(pos - s);
    c->convert = PW_BAD;
    pw_save_rip(c, ML_FORMAT, PWF_Musicline);
    pw_skip_rip(c);
    return 1;
}
