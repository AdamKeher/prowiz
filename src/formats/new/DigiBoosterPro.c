/*
 * DigiBooster Pro (DBM0) - DigiBooster Pro 2.x (Tomasz & Waldemar Piasta)
 * and DigiBooster 3.x (Grzegorz Kraszewski)
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * The file is "DBM0", a version byte (2 or 3), a BCD revision, two reserved
 * bytes, then IFF-like chunks: a 4-byte ID and a big-endian length, with no
 * padding. Nothing stores the total size and nothing marks the end, so the
 * module ends where the chunks stop: at the first ID that is not a DBM0
 * chunk. The checks follow the official loader (libdigibooster3, read_*()):
 * INFO comes before the chunks it describes, its counts are in range, and
 * each content chunk holds exactly what INFO announces.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define DBM_MAX_CHUNKS 64

enum
{
    DBM_NAME, DBM_INFO, DBM_SONG, DBM_INST, DBM_VENV, DBM_PENV,
    DBM_PATT, DBM_PNAM, DBM_SMPL, DBM_DSPE, DBM_MPEG, DBM_NCHUNKS
};

static const char dbm_ids[DBM_NCHUNKS][5] =
{
    "NAME", "INFO", "SONG", "INST", "VENV", "PENV",
    "PATT", "PNAM", "SMPL", "DSPE", "MPEG"
};

typedef struct
{
    uint32_t instruments, samples, songs, patterns, tracks;
} dbm_info;

static int dbm_chunk_kind(const pw_ctx *c, int64_t off)
{
    int k;
    for (k = 0; k < DBM_NCHUNKS; k++)
        if (pw_match(c, off, dbm_ids[k], 4))
            return k;
    return -1;
}

/* Checks the contents of a chunk lying entirely in the input; 'd' is the
 * offset of its data, 'len' its length. Returns 0 if it is not valid. */
static int dbm_check_chunk(const pw_ctx *c, int kind, int64_t d, uint32_t len, dbm_info *in)
{
    uint64_t used = 0;
    uint32_t k;

    switch (kind)
    {
    case DBM_INFO:
        if (len < 10)
            return 0;
        in->instruments = pw_rd16(c, d);
        in->samples = pw_rd16(c, d + 2);
        in->songs = pw_rd16(c, d + 4);
        in->patterns = pw_rd16(c, d + 6);
        in->tracks = pw_rd16(c, d + 8);
        return in->instruments >= 1 && in->instruments <= 255 &&
               in->samples >= 1 && in->samples <= 255 &&
               in->songs >= 1 && in->songs <= 255 &&
               in->patterns >= 1 &&
               in->tracks >= 2 && in->tracks <= 254 && (in->tracks & 1) == 0;

    case DBM_SONG: /* per song: name[44], u16 orders, orders * u16 */
        for (k = 0; k < in->songs; k++)
        {
            if (used + 46 > len)
                return 0;
            used += 46 + 2 * (uint64_t)pw_rd16(c, d + (int64_t)used + 44);
        }
        return used <= len;

    case DBM_INST: /* 50 bytes per instrument */
        return (uint64_t)in->instruments * 50 <= len;

    case DBM_VENV:
    case DBM_PENV: /* u16 count, count * 136 */
        if (len < 2 || pw_rd16(c, d) > 255)
            return 0;
        return 2 + 136 * (uint64_t)pw_rd16(c, d) <= len;

    case DBM_PATT: /* per pattern: u16 rows, u32 packed size, data */
        for (k = 0; k < in->patterns; k++)
        {
            uint32_t rows, packed;
            if (used + 6 > len)
                return 0;
            rows = pw_rd16(c, d + (int64_t)used);
            packed = pw_rd32(c, d + (int64_t)used + 2);
            if (rows == 0 || packed == 0)
                return 0;
            used += 6 + (uint64_t)packed;
        }
        return used <= len;

    case DBM_SMPL: /* per sample: u32 flags, u32 frames, frames * bytes */
        for (k = 0; k < in->samples; k++)
        {
            uint32_t flags, frames, bps;
            if (used + 8 > len)
                return 0;
            flags = pw_rd32(c, d + (int64_t)used);
            frames = pw_rd32(c, d + (int64_t)used + 4);
            if (frames >= 0x40000000)
                return 0;
            bps = flags & 7;
            if (frames > 0 && bps != 1 && bps != 2 && bps != 4)
                return 0;
            used += 8 + (uint64_t)frames * (frames ? bps : 0);
        }
        return used <= len;

    case DBM_DSPE: /* u16 mask length (= tracks), the mask, 8 bytes */
        return len >= 2 && pw_rd16(c, d) == in->tracks &&
               2 + (uint64_t)in->tracks + 8 <= len;

    default: /* NAME, PNAM, MPEG */
        return 1;
    }
}

int pw_new_DigiBoosterPro(pw_ctx *c)
{
    int64_t  s = (int64_t)c->i;
    uint64_t avail, pos = 8;
    uint32_t seen = 0;
    int      n, truncated = 0, version, rev;
    dbm_info info = {0, 0, 0, 0, 0};

    if (!pw_match(c, s, "DBM0", 4))
        return 0;
    version = (int)pw_rd8(c, s + 4);
    rev = (int)pw_rd8(c, s + 5);
    if ((version != 2 && version != 3) || (rev >> 4) > 9 || (rev & 15) > 9)
        return 0;
    /* the first chunk is the song name (or, at least, the INFO chunk) */
    n = dbm_chunk_kind(c, s + 8);
    if (n != DBM_NAME && n != DBM_INFO)
        return 0;

    avail = (uint64_t)c->in_size - (uint64_t)s;
    for (n = 0; n < DBM_MAX_CHUNKS; n++)
    {
        int      kind;
        uint32_t len;
        uint64_t end;

        if (pos + 8 > avail)
        {
            /* a chunk ID cut by the end of the input: the module goes on */
            if (pos + 4 <= avail && dbm_chunk_kind(c, s + (int64_t)pos) >= 0)
            {
                truncated = 1;
                pos += 8;
            }
            break;
        }
        kind = dbm_chunk_kind(c, s + (int64_t)pos);
        if (kind < 0)
            break;
        if (seen & (1u << kind))
            return 0; /* each chunk once */
        if (kind != DBM_NAME && kind != DBM_INFO && kind != DBM_PNAM && kind != DBM_MPEG &&
            !(seen & (1u << DBM_INFO)))
            return 0; /* the official loader wants INFO first */
        len = pw_rd32(c, s + (int64_t)pos + 4);
        end = pos + 8 + len;
        if (end > 0x40000000u)
            return 0; /* far beyond any real module */
        seen |= 1u << kind;
        if (end > avail)
        {
            /* runs past the input: validate what can be, report it cut */
            if (kind == DBM_INFO)
                return 0; /* too little to judge */
            truncated = 1;
            pos = end;
            break;
        }
        if (!dbm_check_chunk(c, kind, s + (int64_t)pos + 8, len, &info))
            return 0;
        pos = end;
    }

    if (!(seen & (1u << DBM_INFO)))
        return 0;
    if (!truncated)
    {
        const uint32_t need = (1u << DBM_SONG) | (1u << DBM_INST) | (1u << DBM_PATT) | (1u << DBM_SMPL);
        if ((seen & need) != need)
            return 0;
    }

    c->start = (int32_t)s;
    c->out_size = (uint32_t)pos;
    c->convert = PW_BAD;
    pw_save_rip(c, version == 3 ? "DigiBooster 3 module" : "DigiBooster Pro module",
                PWF_DigiBoosterPro);
    pw_skip_rip(c);
    return 1;
}
