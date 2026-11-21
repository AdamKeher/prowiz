/*
 * LSP.c - LightSpeedPlayer (.lsmusic + .lsbank)
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * LightSpeedPlayer (Arnaud Carre aka Leonard/Oxygene, 2021 onwards) plays a
 * MOD that LSPConvert has turned into a per-tick stream of Paula register
 * writes. It comes as two files, both big-endian:
 *
 *   X.lsmusic  "LSP1" (or "LSPm", micro mode), a header, an instrument table
 *              and the streams.
 *   X.lsbank   a 4-byte ID (the one at lsmusic+04 for LSP1) followed by raw
 *              8-bit samples; no other header.
 *
 * LSP1 header by converter version (word at 08 = major<<8 | minor):
 *
 *   off  A: 1.00-1.03   B: 1.04-1.05   C: 1.10       D: 1.11 onwards
 *   0A   BPM            BPM            flags         flags
 *   0C   n              escRewind      BPM           BPM
 *   0E   instruments    escSetBPM      escRewind     escRewind
 *   10                  frames (u32)   escSetBPM     escSetBPM
 *   12                                 frames (u32)  escGetPos
 *   14                  n                            frames (u32)
 *   16                  instruments    n
 *   18                                 instruments   [ADPCM block] n
 *
 *   flags: high byte 00, or FF once a player has initialised the music;
 *          bit 0 getpos, bit 1 setpos, bit 2 ADPCM bank (1.30 onwards).
 *   ADPCM block: u32 in-place offset, u32 lossless mask, one u16
 *          (sample size / 2 - 1) per bank sample, u16 0.
 *
 * Then: n x 12 bytes of instruments (u32 start, u16 length in words, u32
 * repeat start, u16 repeat length; offsets into the bank, 4 or more), u16 T
 * + T words of code table, (C, D) u16 S + S x 8 bytes of sequence table,
 * u32 word stream size W, u32 byte stream loop, u32 word stream loop, the
 * word stream and the byte stream. The byte stream's size is not stored: it
 * ends with the "rewind" escape, so the size comes from walking it exactly as
 * that version's player does (lsp_walk).
 *
 * LSPm (micro, 1.20 onwards): 04 version, 06 BPM (1.24 onwards), n,
 * n x 12 bytes of instruments, 16 u32 offsets of the streams (command,
 * volume, period and instrument streams of each voice), the streams.
 *
 * In memory, the player adds the bank address to every instrument (1.04-1.05
 * also set bit 0 of the start), sets the flag byte at 0A to FF (1.10 onwards,
 * also by the generated "insane" players from 1.11), adds the word stream
 * address to the sequence table, and decodes an ADPCM bank in place. These
 * are undone, and the bank is found through its ID (LSP1) or through the
 * pointers (LSPm, or a relocated music in a dump of memory from address 0).
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define LSP_MAX_INSTR 2600u       /* LSPConvert's LSP_INSTRUMENT_MAX       */
#define LSP_MAX_MICRO 256u        /* instrument numbers are bytes in -micro */
#define LSP_MAX_TABLE 768u        /* 765 codes + 3 reserved zeros           */
#define LSP_MAX_SEQ   128u        /* MOD positions                          */
#define LSP_MAX_SIZE  0x200000u   /* LSPConvert stops at 30 min of 100 Hz ticks */
#define LSP_CHIP_END  0x200000u   /* the bank lives in (at most 2 MB of) chip RAM */
#define LSP_MIN_BASE  0x400u      /* a bank is never loaded below this      */

typedef struct lsp
{
    uint32_t base;            /* input offset of the music              */
    uint32_t avail;           /* input bytes from there on              */
    char     layout;          /* 'A'..'D' (LSP1) or 'm' (LSPm)          */
    uint32_t ver, flags;
    uint32_t esc_rew, esc_bpm, esc_pos;
    uint32_t frames;
    uint32_t adpcm, nadpcm;   /* ADPCM block offset (0: none), samples  */
    uint32_t ninst, inst;     /* instrument count and table offset      */
    uint32_t ntab, nseq, seq; /* code table size, sequence table        */
    uint32_t seq_delta;       /* word stream address a player added     */
    uint32_t seq_bmax;        /* highest byte stream offset in it       */
    uint32_t wstart, wsize, bloop, wloop;
    uint32_t moff[16];        /* micro stream offsets                   */
    uint32_t mdata;           /* micro: start of the stream data        */
    uint64_t size;            /* total size; a lower bound if truncated */
    int      truncated;
    int      restore;         /* found as initialised by its player     */
    uint32_t bank_addr;       /* the bank address the player added, or 0 */
    int      odd_starts;      /* 1.04-1.05: bit 0 of each start was set */
    uint32_t bank_size;
    uint16_t table[LSP_MAX_TABLE];
} lsp;

#define R8(o)  pw_rd8(c, (int64_t)h->base + (int64_t)(o))
#define R16(o) pw_rd16(c, (int64_t)h->base + (int64_t)(o))
#define R32(o) pw_rd32(c, (int64_t)h->base + (int64_t)(o))

static const int8_t lsp_dpcm[16] =
{
    0, 1, 2, 4, 8, 16, 32, 64, -128, -64, -32, -16, -8, -4, -2, -1
};

static uint32_t lsp_bits4(uint32_t v)
{
    return (v & 1) + ((v >> 1) & 1) + ((v >> 2) & 1) + ((v >> 3) & 1);
}

/* The instrument table: are its pointers relocated, and by how much? Sets
 * bank_addr, odd_starts, restore and bank_size; 0 if implausible. */
static int lsp_instruments(pw_ctx *c, lsp *h)
{
    uint32_t k, minp = 0xFFFFFFFFu, minp_b = 0xFFFFFFFFu, odd = 0, base = 0;
    uint64_t maxend = 0;

    for (k = 0; k < h->ninst; k++)
    {
        uint32_t o = h->inst + 12 * k;
        uint32_t s = R32(o), l = R16(o + 4), r = R32(o + 6);

        /* (a repeat length of 0 does occur: the MOD's own loop length) */
        if (l == 0)
            return 0;
        if (s < minp)
            minp = s;
        if (r < minp)
            minp = r;
        if (s & 1)
        {
            odd++;
            if (s - 1 < minp_b)
                minp_b = s - 1;
        }
        if (r < minp_b)
            minp_b = r;
        if ((uint64_t)s + 2u * l > maxend)
            maxend = (uint64_t)s + 2u * l;
    }
    /* offsets start at 4 (after the ID); relocated ones point into chip RAM */
    if (minp < 4 || maxend > LSP_CHIP_END)
        return 0;

    switch (h->layout)
    {
    case 'B':
        /* 1.04-1.05 players set bit 0 of each start, then add the bank */
        if (odd == h->ninst && minp_b - 4 >= LSP_MIN_BASE && ((minp_b - 4) & 1) == 0)
        {
            base = minp_b - 4;
            h->odd_starts = 1;
            h->restore = 1;
            maxend -= 1;
        }
        break;
    case 'C':
    case 'D':
        /* flag FF: initialised; the "insane" players leave the table alone */
        if ((h->flags >> 8) == 0xFF)
        {
            h->restore = 1;
            if (minp - 4 >= LSP_MIN_BASE && ((minp - 4) & 1) == 0)
                base = minp - 4;
        }
        break;
    default:
        /* 1.00-1.03 and micro: no flag; the first sample sits at offset 4 */
        if (minp - 4 >= LSP_MIN_BASE && ((minp - 4) & 1) == 0)
        {
            base = minp - 4;
            h->restore = 1;
        }
        break;
    }
    if (maxend <= (uint64_t)base + 4)
        return 0;
    h->bank_addr = base;
    h->bank_size = (uint32_t)maxend - base;
    return 1;
}

/* LSP1 header up to the streams; 0 if implausible. */
static int lsp_header(pw_ctx *c, lsp *h)
{
    uint32_t p, k, last;

    h->ver = R16(8);
    if ((h->ver >> 8) != 1 || (h->ver & 0xFF) > 0x40)
        return 0;
    if (h->ver < 0x0104)
    {
        h->layout = 'A';
        if (R16(0x0A) == 0 || R16(0x0A) > 255)
            return 0;
        h->ninst = R16(0x0C);
        h->inst = 0x0E;
    }
    else if (h->ver < 0x010A)
    {
        h->layout = 'B';
        if (R16(0x0A) == 0 || R16(0x0A) > 255)
            return 0;
        h->esc_rew = R16(0x0C);
        h->esc_bpm = R16(0x0E);
        h->frames = R32(0x10);
        h->ninst = R16(0x14);
        h->inst = 0x16;
    }
    else
    {
        h->layout = h->ver == 0x010A ? 'C' : 'D';
        h->flags = R16(0x0A);
        if ((h->flags >> 8) != 0x00 && (h->flags >> 8) != 0xFF)
            return 0;
        if (h->flags & 0xF8)
            return 0;
        if (R16(0x0C) == 0 || R16(0x0C) > 255)
            return 0;
        h->esc_rew = R16(0x0E);
        h->esc_bpm = R16(0x10);
        if (h->layout == 'C')
        {
            h->frames = R32(0x12);
            h->ninst = R16(0x16);
            h->inst = 0x18;
        }
        else
        {
            h->esc_pos = R16(0x12);
            if (h->esc_rew == h->esc_pos)
                return 0;
            h->frames = R32(0x14);
            p = 0x18;
            if (h->flags & 4)
            {
                /* ADPCM block (1.30 onwards): up to 31 samples, then 0 */
                if (h->ver < 0x011E)
                    return 0;
                h->adpcm = p;
                for (k = 0; k < 32 && R16(p + 8 + 2 * k) != 0; k++)
                    ;
                if (k == 0 || k == 32)
                    return 0;
                h->nadpcm = k;
                p += 8 + 2 * (k + 1);
            }
            h->ninst = R16(p);
            h->inst = p + 2;
        }
    }
    if (h->layout != 'A')
    {
        if (h->esc_rew == h->esc_bpm)
            return 0;
        if (h->frames == 0 || h->frames > LSP_MAX_SIZE)
            return 0;
    }
    if (h->ninst == 0 || h->ninst > LSP_MAX_INSTR)
        return 0;

    /* the code table; the header must be complete in the input */
    p = h->inst + 12 * h->ninst;
    if ((uint64_t)p + 2 > h->avail)
        return 0;
    h->ntab = R16(p);
    if (h->ntab < 2 || h->ntab > LSP_MAX_TABLE || (h->layout == 'D' && h->ntab < 258))
        return 0;
    if ((uint64_t)p + 2 + 2 * h->ntab + 2 + 12 > h->avail)
        return 0;
    last = 0xFFFFFFFFu;
    for (k = 0; k < h->ntab; k++)
    {
        h->table[k] = (uint16_t)R16(p + 2 + 2 * k);
        if (h->table[k] == (h->layout == 'A' ? 0xFFFFu : h->esc_rew))
            last = k;
    }
    /* index 0 is never used (a 0 byte extends the index); neither are 256
     * and 512 from 1.04 on */
    if (h->table[0] != 0)
        return 0;
    if (h->layout != 'A' && ((h->ntab > 256 && h->table[256] != 0) ||
                             (h->ntab > 512 && h->table[512] != 0)))
        return 0;
    /* the rewind escape is among the last codes (from 1.11: past 255) */
    if (last == 0xFFFFFFFFu)
        return 0;
    if (h->layout != 'A' && last + 4 < h->ntab)
        return 0;
    if (h->layout == 'D' && last < 256)
        return 0;
    p += 2 + 2 * h->ntab;

    /* the sequence table (C, D: -setpos) */
    if (h->layout == 'C' || h->layout == 'D')
    {
        h->nseq = R16(p);
        if (h->nseq > LSP_MAX_SEQ)
            return 0;
        h->seq = p + 2;
        p += 2 + 8 * h->nseq;
        if ((uint64_t)p + 12 > h->avail)
            return 0;
    }
    h->wsize = R32(p);
    h->bloop = R32(p + 4);
    h->wloop = R32(p + 8);
    h->wstart = p + 12;
    if ((h->wsize & 1) || h->wsize > LSP_MAX_SIZE)
        return 0;
    if ((h->wloop & 1) || h->wloop > h->wsize)
        return 0;

    if (h->nseq)
    {
        uint32_t e0w = R32(h->seq), e0b = R32(h->seq + 4), pw = 0, pb = 0;

        /* on disk the first entry is (0, W); a player added the address of
         * the word stream to every offset */
        if ((h->flags >> 8) == 0xFF)
            h->seq_delta = e0w;
        if (e0w != h->seq_delta || e0b < h->seq_delta || e0b - h->seq_delta != h->wsize)
            return 0;
        for (k = 0; k < h->nseq; k++)
        {
            uint32_t w = R32(h->seq + 8 * k), b = R32(h->seq + 8 * k + 4);
            if (w < h->seq_delta || b < h->seq_delta)
                return 0;
            w -= h->seq_delta;
            b -= h->seq_delta;
            if ((w & 1) || w > h->wsize || b < h->wsize || w < pw || b < pb)
                return 0;
            pw = w;
            pb = b;
        }
        h->seq_bmax = pb;
    }
    return 1;
}

/* the ADPCM block against the bank size: sample sizes add up, and the packed
 * data ends exactly at the end of the bank */
static int lsp_adpcm_ok(pw_ctx *c, const lsp *h)
{
    uint32_t k, mask, total = 0, packed = 0;

    if (!h->adpcm)
        return 1;
    mask = R32(h->adpcm + 4);
    if (h->nadpcm < 32 && (mask & ((1u << (32 - h->nadpcm)) - 1)) != 0)
        return 0;
    for (k = 0; k < h->nadpcm; k++)
    {
        uint32_t len = 2 * (R16(h->adpcm + 8 + 2 * k) + 1);
        total += len;
        packed += (mask & (0x80000000u >> k)) ? len : len / 2;
    }
    return (uint64_t)total + 4 == h->bank_size && R32(h->adpcm) == total - packed;
}

/* Walks the byte stream (and the word stream it drives) as the player of
 * that version does, up to the rewind escape: sets the size. Every frame is
 * checked: code index, volumes, word count, instrument offsets. Running out
 * of input on the way makes it a truncated music. */
static int lsp_walk(pw_ctx *c, lsp *h)
{
    const int is_a = h->layout == 'A', is_d = h->layout == 'D';
    const uint32_t bstart = h->wstart + h->wsize, half = h->wsize / 2;
    uint32_t q = bstart, wq = h->wstart, words = 0, fr = 0;

    if (bstart > h->avail)
        goto truncated;

    for (;;)
    {
        uint32_t b, idx = 0, v, nvol, nper, ninst = 0, k;
        int32_t off;
        int ext = 0;

        if (q - bstart > LSP_MAX_SIZE)
            return 0; /* no frame count to stop 1.00-1.03 on garbage */
        if (q >= h->avail)
            goto truncated;
        b = R8(q++);
        if (is_a)
        {
            /* at most two 0 bytes: index 0x100|b, then 0x200|b */
            idx = b;
            if (b == 0)
            {
                if (q >= h->avail)
                    goto truncated;
                b = R8(q++);
                idx = 0x100 | b;
                if (b == 0)
                {
                    if (q >= h->avail)
                        goto truncated;
                    idx = 0x200 | R8(q++);
                }
            }
        }
        else
        {
            /* each 0 byte adds 0x100 to the index */
            while (b == 0)
            {
                idx += 0x100;
                ext = 1;
                if (idx >= h->ntab)
                    return 0;
                if (q >= h->avail)
                    goto truncated;
                b = R8(q++);
            }
            idx += b;
        }
        if (idx >= h->ntab)
            return 0;
        v = h->table[idx];

        /* escapes: 1.00-1.03 fixed values; 1.04-1.10 tested on any non-zero
         * code; from 1.11 only on extended codes (index > 255) */
        if (is_a ? (v == 0xFFFF) : ((is_d ? ext : v != 0) && v == h->esc_rew))
            break;
        if (is_a ? (v == 0xF00F) : ((is_d ? ext : v != 0) && v == h->esc_bpm))
        {
            if (q >= h->avail)
                goto truncated;
            if (R8(q++) == 0)
                return 0; /* BPM */
            continue;
        }
        if (is_d && ext && v == h->esc_pos)
        {
            /* a marker for LSP_MusicGetPos: the MOD position, or any byte
             * (other converters number every row) */
            if (q >= h->avail)
                goto truncated;
            q++;
            continue;
        }

        /* a frame: volumes (bytes), periods then instruments (words) */
        fr++;
        if (!is_a && fr > h->frames)
            return 0;
        nvol = is_a ? lsp_bits4(v >> 8) : lsp_bits4(v >> 4);
        nper = is_a ? lsp_bits4(v >> 4) : lsp_bits4(v);
        for (k = 0; k < nvol; k++)
        {
            if (q >= h->avail)
                goto truncated;
            if (R8(q++) > 64)
                return 0;
        }
        for (k = 0; k < 4; k++)
            if (is_a ? ((v >> k) & 1) : ((v >> (8 + 2 * k)) & 2))
                ninst++;
        if (nper + ninst > half - words)
            return 0;
        words += nper + ninst;
        wq += 2 * nper;
        /* instrument words: running offsets into the instrument table, from
         * -12; a note points at an entry, "no note" at its repeat half */
        off = -12;
        for (k = 4; k-- > 0;)
        {
            uint32_t code = is_a ? ((v >> k) & 1) * 3 : (v >> (8 + 2 * k)) & 3;
            if (!(code & 2))
                continue;
            off += (int16_t)R16(wq);
            wq += 2;
            if (off < 0 || off % 6 != 0 || (uint32_t)off / 6 >= 2 * h->ninst)
                return 0;
            if (code == 3 && off % 12 != 0)
                return 0;
            off += 6;
        }
    }

    if (words != half || (!is_a && fr != h->frames))
        return 0;
    if (h->bloop >= q - bstart || (h->nseq && h->seq_bmax >= h->wsize + (q - bstart)))
        return 0;
    h->size = q;
    return 1;

truncated:
    /* at least one more byte per frame still to come, and the rewind */
    h->truncated = 1;
    h->size = (uint64_t)(q > h->avail ? q : h->avail) + 1;
    if (!is_a && h->frames > fr)
        h->size += h->frames - fr;
    return 1;
}

/* LSPm: header, stream offsets and the streams; sets the size. */
static int lsp_micro(pw_ctx *c, lsp *h)
{
    uint32_t p, k, v, F, ninst3 = 0;
    const uint32_t *t = h->moff;

    h->layout = 'm';
    h->ver = R16(4);
    if ((h->ver >> 8) != 1 || (h->ver & 0xFF) < 0x14 || (h->ver & 0xFF) > 0x40)
        return 0;
    p = 6;
    if (h->ver >= 0x0118)
    {
        if (R16(6) == 0 || R16(6) > 255)
            return 0;
        p = 8;
    }
    h->ninst = R16(p);
    if (h->ninst == 0 || h->ninst > LSP_MAX_MICRO)
        return 0;
    h->inst = p + 2;
    p = h->inst + 12 * h->ninst;
    if ((uint64_t)p + 64 > h->avail)
        return 0;
    for (k = 0; k < 16; k++)
        h->moff[k] = R32(p + 4 * k);
    h->mdata = p + 64;

    /* offsets listed as command, volume, period, instrument streams (voices
     * 0-3 each); the data is stored as period, command, volume, instrument.
     * Every voice has one command byte per frame. */
    F = t[1] - t[0];
    if (t[8] != 0 || t[9] < t[8] || t[10] < t[9] || t[11] < t[10] || t[0] < t[11])
        return 0;
    if (t[1] <= t[0] || t[2] < t[1] || t[3] < t[2] || t[4] < t[3] ||
        t[2] - t[1] != F || t[3] - t[2] != F || t[4] - t[3] != F)
        return 0;
    if (t[5] < t[4] || t[6] < t[5] || t[7] < t[6] || t[12] < t[7] ||
        t[13] < t[12] || t[14] < t[13] || t[15] < t[14])
        return 0;
    if (((t[9] - t[8]) | (t[10] - t[9]) | (t[11] - t[10]) | (t[0] - t[11])) & 1)
        return 0;
    if (t[15] > LSP_MAX_SIZE)
        return 0;
    if (!lsp_instruments(c, h))
        return 0;

    if ((uint64_t)h->mdata + t[4] > h->avail)
    {
        /* not even the command streams: at least up to the last stream */
        h->truncated = 1;
        h->size = (uint64_t)h->mdata + t[15];
        if (h->size <= h->avail)
            h->size = (uint64_t)h->avail + 1;
        return 1;
    }

    for (v = 0; v < 4; v++)
    {
        uint32_t nv = 0, np = 0, ni = 0, saves = 0, cs = h->mdata + t[v];
        for (k = 0; k < F; k++)
        {
            uint32_t b = R8(cs + k), loop = (b >> 3) & 3;
            if (b & 7)
                return 0;
            /* loop control, voice 3 only: 10 marks the loop point (once),
             * 11 the end of the song (last frame) */
            if (loop && (v != 3 || loop == 1 || (loop == 3 && k != F - 1) ||
                         (loop == 2 && ++saves > 1)))
                return 0;
            nv += b >> 7;
            np += (b >> 6) & 1;
            ni += (b >> 5) & 1;
        }
        if (v == 3 && ((R8(cs + F - 1) >> 3) & 3) != 3)
            return 0;
        /* the other streams of this voice hold exactly what it reads */
        if ((v < 3 ? t[5 + v] : t[12]) - t[4 + v] != nv)
            return 0;
        if ((v < 3 ? t[9 + v] : t[0]) - t[8 + v] != 2 * np)
            return 0;
        if (v < 3 && t[13 + v] - t[12 + v] != ni)
            return 0;
        if (v == 3)
            ninst3 = ni;
    }
    h->size = (uint64_t)h->mdata + t[15] + ninst3;
    if (h->size > h->avail)
    {
        h->truncated = 1;
        return 1;
    }
    for (k = t[4]; k < t[12]; k++)
        if (R8(h->mdata + k) > 64)
            return 0;
    for (k = t[12]; k < t[15] + ninst3; k++)
        if (R8(h->mdata + k) >= h->ninst)
            return 0;
    return 1;
}

/* the music as it was on disk: player changes undone; NULL if out of memory */
static uint8_t *lsp_restored(pw_ctx *c, const lsp *h)
{
    uint8_t *d = pw_copy_in(c, h->base, (uint32_t)h->size);
    uint32_t k, o, x;

    if (d == NULL)
        return NULL;
    if (h->layout == 'C' || h->layout == 'D')
        d[0x0A] = 0;
    for (k = 0; k < 2 * h->ninst; k++)
    {
        o = h->inst + 12 * (k / 2) + 6 * (k & 1);
        x = ((uint32_t)d[o] << 24) | ((uint32_t)d[o + 1] << 16) | ((uint32_t)d[o + 2] << 8) | d[o + 3];
        x -= h->bank_addr;
        if (h->odd_starts && (k & 1) == 0)
            x -= 1;
        d[o] = (uint8_t)(x >> 24);
        d[o + 1] = (uint8_t)(x >> 16);
        d[o + 2] = (uint8_t)(x >> 8);
        d[o + 3] = (uint8_t)x;
    }
    for (k = 0; h->seq_delta && k < 2 * h->nseq; k++)
    {
        o = h->seq + 4 * k;
        x = ((uint32_t)d[o] << 24) | ((uint32_t)d[o + 1] << 16) | ((uint32_t)d[o + 2] << 8) | d[o + 3];
        x -= h->seq_delta;
        d[o] = (uint8_t)(x >> 24);
        d[o + 1] = (uint8_t)(x >> 16);
        d[o + 2] = (uint8_t)(x >> 8);
        d[o + 3] = (uint8_t)x;
    }
    return d;
}

/* LSP1: the bank starts with the music's ID. Skip the music itself, other
 * copies of it ("LSP1" before the ID) and the generated "insane" players,
 * which hold the ID in a move.l #ID,d0 (20 3C). */
static int lsp_find_bank_id(pw_ctx *c, const lsp *h, uint32_t *at)
{
    const uint32_t id = R32(4);
    uint32_t x, k, end;

    if (id == 0 || h->bank_size > (uint32_t)c->in_size)
        return 0;
    end = (uint32_t)c->in_size - h->bank_size;
    for (x = 0; x + 4 <= (uint32_t)c->in_size; x++)
    {
        if (pw_rd8(c, x) != (id >> 24) || pw_rd32(c, x) != id)
            continue;
        if (x >= h->base && x < h->base + h->size)
            continue;
        if (x >= 2 && pw_rd8(c, x - 2) == 0x20 && pw_rd8(c, x - 1) == 0x3C)
            continue;
        if (x >= 4 && pw_match(c, x - 4, "LSP1", 4))
        {
            /* An earlier copy of this music already searched the whole
             * input, and claimed the bank it found: reuse that (this keeps
             * an input full of copies from costing copies x size). */
            if (x >= h->base)
                continue;
            for (k = 0; k < c->nclaimed; k++)
                if (c->claimed[k].size == h->bank_size &&
                    pw_rd32(c, c->claimed[k].start) == id)
                {
                    *at = c->claimed[k].start;
                    return 1;
                }
            return 0;
        }
        if (x > end)
            return 0; /* a bank that would run past the input */
        *at = x;
        return 1;
    }
    return 0;
}

/* a relocated music points at its bank: in a dump of memory from address 0,
 * that is where it is. Check what LSPConvert guarantees there: the first
 * two bytes of every one-shot sample (repeat length 0 or 1) are zero. */
static int lsp_find_bank_ptr(pw_ctx *c, const lsp *h, uint32_t *at)
{
    const uint32_t x = h->bank_addr;
    uint32_t k;

    if (x == 0 || (uint64_t)x + h->bank_size > (uint32_t)c->in_size)
        return 0;
    if ((uint64_t)x + h->bank_size > h->base && x < h->base + h->size)
        return 0;
    for (k = 0; k < h->ninst; k++)
    {
        uint32_t o = h->inst + 12 * k;
        if (R16(o + 10) <= 1)
        {
            uint32_t r = R32(o + 6) - h->bank_addr;
            if (pw_rd8(c, (int64_t)x + r) != 0 || pw_rd8(c, (int64_t)x + r + 1) != 0)
                return 0;
        }
    }
    *at = x;
    return 1;
}

/* An initialised music's ADPCM bank was decoded in place by its player.
 * Pack it again as LSPConvert does (nearest DPCM step: every decoded step is
 * exact, so this gives back the original nibbles; the space before the
 * packed data was zero). NULL if the bank is not such decoded data. */
static uint8_t *lsp_repack(pw_ctx *c, const lsp *h, uint32_t at)
{
    uint8_t *d = (uint8_t *)calloc(h->bank_size, 1);
    const uint32_t mask = R32(h->adpcm + 4);
    uint32_t w, r = at + 4, k, j;

    if (d == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        return NULL;
    }
    for (k = 0; k < 4; k++)
        d[k] = (uint8_t)pw_rd8(c, (int64_t)at + k);
    w = 4 + R32(h->adpcm);
    for (k = 0; k < h->nadpcm; k++)
    {
        const uint32_t len = 2 * (R16(h->adpcm + 8 + 2 * k) + 1);
        int s = 0;

        if (mask & (0x80000000u >> k))
        {
            for (j = 0; j < len; j++)
                d[w++] = (uint8_t)pw_rd8(c, (int64_t)r + j);
            r += len;
            continue;
        }
        for (j = 0; j < len; j++)
        {
            const int target = (int8_t)pw_rd8(c, (int64_t)r + j);
            int nib;

            for (nib = 0; nib < 16 && s + lsp_dpcm[nib] != target; nib++)
                ;
            if (nib == 16)
            {
                free(d);
                return NULL;
            }
            s = target;
            if (j & 1)
                d[w++] |= (uint8_t)nib;
            else
                d[w] = (uint8_t)(nib << 4);
        }
        r += len;
    }
    return d;
}

static void lsp_save_bank(pw_ctx *c, const lsp *h)
{
    uint32_t at = 0;
    int found;

    found = h->layout != 'm' && lsp_find_bank_id(c, h, &at);
    if (!found)
        found = lsp_find_bank_ptr(c, h, &at);
    if (!found)
    {
        pw_printf(c, "  Its sample bank (.lsbank) is not in this file.\n");
        return;
    }
    if (h->adpcm && h->restore)
    {
        uint8_t *d = lsp_repack(c, h, at);
        if (c->fatal)
            return;
        if (d != NULL)
        {
            pw_save_companion(c, "lsbank", d, h->bank_size);
            free(d);
            pw_claim(c, at, h->bank_size);
            pw_printf(c, "  Its player had ADPCM-decoded the bank: packed again as on disk !\n");
            return;
        }
        pw_save_companion_in(c, "lsbank", at, h->bank_size);
        pw_printf(c, "  Its bank should be ADPCM-decoded but is not: saved as found !\n");
        return;
    }
    pw_save_companion_in(c, "lsbank", at, h->bank_size);
}

int pw_new_LSP(pw_ctx *c)
{
    lsp h;
    uint32_t k3;
    int ok;

    /* "LSP1" with version 01xx at 08, or "LSPm" with 01xx at 04; this also
     * rejects the copies of "LSP1" inside every LSP player's code */
    if (pw_rd8(c, (int64_t)c->i + 1) != 'S' || pw_rd8(c, (int64_t)c->i + 2) != 'P')
        return 0;
    k3 = pw_rd8(c, (int64_t)c->i + 3);
    if (k3 == '1')
    {
        if (pw_rd8(c, (int64_t)c->i + 8) != 0x01)
            return 0;
    }
    else if (k3 != 'm' || pw_rd8(c, (int64_t)c->i + 4) != 0x01)
        return 0;
    if (pw_claimed(c, c->i))
        return 0;

    memset(&h, 0, sizeof h);
    h.base = c->i;
    h.avail = (uint32_t)c->in_size - c->i;
    if (k3 == 'm')
        ok = lsp_micro(c, &h);
    else
        ok = lsp_header(c, &h) && lsp_instruments(c, &h) && lsp_adpcm_ok(c, &h) &&
             lsp_walk(c, &h);
    if (!ok || h.size > LSP_MAX_SIZE)
        return 0;

    c->start = (int32_t)h.base;
    c->out_size = (uint32_t)h.size;
    c->convert = PW_BAD;
    {
        const char *name = k3 == 'm' ? "LightSpeedPlayer micro (LSPm) music"
                                     : "LightSpeedPlayer (LSP1) music";
        if (h.truncated || !h.restore)
            pw_save_rip(c, name, PWF_LSP);
        else
        {
            uint8_t *d = lsp_restored(c, &h);
            if (d == NULL)
                return 1;
            pw_save_rip_data(c, name, PWF_LSP, d, (size_t)h.size, 1);
            free(d);
        }
    }
    if (c->save_status == PW_GOOD && !c->fatal)
        lsp_save_bank(c, &h);
    pw_skip_rip(c);
    return 1;
}
