/*
 * MMD23.c - OctaMED MMD2 / MMD3, and MMD0-MMD3 relocated in memory
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * An MMD is a tree of C structures whose pointers are offsets from the
 * start of the module (spec: "MED/OctaMED MMD0/MMD1/MMD2/MMD3 file formats",
 * Teijo Kinnunen, revision 6). The header holds the size of the whole file
 * (modlen at 04), multi-song files included: their further headers ("MCN2",
 * "MCN3", ... chained through MMD0exp.nextmod) have modlen 0.
 *
 * MMD2 and MMD3 files are found as such. The checks follow the structures:
 * every pointer lands inside the module (on an even offset: the spec
 * demands it, though MED 3.00 did not always align its MMD0s) and every
 * table fits. MMD2 and MMD3 differ from MMD0/1 in the song structure (MMD2song:
 * play sequences, sections, track volumes and pans) and use MMD1 blocks.
 *
 * A player relocates the module before playing it: RelocModule (reloc.a,
 * loadmod.a) adds the load address to every non-null pointer, following
 * the graph described below, and the play routine (proplayer.a) writes its
 * position into the header. Such a copy is found when the plain checks
 * fail: the load address is inferred from the header pointers and confirmed
 * by the same checks, then the relocation is undone along the same graph and
 * the player fields are reset to what a file holds.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define MMD_HDR        0x34u        /* header size */
#define MMD_SONG       0x314u       /* MMD0song / MMD2song size */
#define MMD_MIN        0x348u       /* header + song: the smallest module */
#define MMD_LIMIT      0x40000000u  /* far beyond any real module */
#define MMD_MIN_BASE   0x400u       /* the lowest load address (68k vectors below) */
#define MMD_MAX_TRIES  0x10000u     /* load addresses tried per header */
#define MMD_MAX_FULL   64           /* of which fully checked */
#define MMD_WORK       0x400000u    /* structures examined per finding: hostile
                                       data can make the walks quadratic */
#define MMD_MAX_TRACKS 64
#define MMD_EXP_SIZE   0x50u        /* MMD0exp */

/* offsets in MMD0song / MMD2song */
#define SONG_NUMBLOCKS  0x1F8
#define SONG_SONGLEN    0x1FA
#define SONG_PSEQTABLE  0x1FC
#define SONG_SECTABLE   0x200
#define SONG_TRACKVOLS  0x204
#define SONG_NUMTRACKS  0x208
#define SONG_NUMPSEQS   0x20A
#define SONG_TRACKPANS  0x20C
#define SONG_NUMSAMPLES 0x313

/* A module seen through its pointers: in the input at 'start' (with only
 * 'avail' of its 'modlen' bytes present when cut by the end of the input),
 * or in a buffer. 'base' is the address the pointers were relocated to (0
 * for a file). */
typedef struct
{
    const pw_ctx  *c;
    const uint8_t *buf;
    int64_t        start;
    uint32_t       avail;
    uint32_t       modlen;
    uint32_t       base;
    int            align;   /* structures on even offsets (MMD2/MMD3) */
    uint64_t      *ext;     /* when set: the furthest end of the structures seen */
    uint32_t      *work;    /* what is left of the work budget (MMD_WORK) */
} mmd_view;

/* one more structure to examine; 0 once the budget is spent */
static int vwork(const mmd_view *v)
{
    if (*v->work == 0)
        return 0;
    (*v->work)--;
    return 1;
}

static uint32_t v8(const mmd_view *v, uint64_t off)
{
    if (off >= v->avail)
        return 0;
    return v->buf ? v->buf[off] : pw_rd8(v->c, v->start + (int64_t)off);
}
static uint32_t v16(const mmd_view *v, uint64_t off)
{
    return (v8(v, off) << 8) | v8(v, off + 1);
}
static uint32_t v32(const mmd_view *v, uint64_t off)
{
    return (v16(v, off) << 16) | v16(v, off + 2);
}
/* a pointer field: 0 stays 0, anything else becomes an offset in the module
 * (a huge value if it was below the base) */
static uint32_t vptr(const mmd_view *v, uint64_t off)
{
    uint32_t p = v32(v, off);
    return p ? p - v->base : 0;
}
/* [off, off + size) lies after the header and inside the module; MMD2/MMD3
 * structures are on even offsets (byte arrays need not be), MMD0/MMD1 ones
 * may not be: MED 3.00's save routine forgot to align them */
static int vin(const mmd_view *v, uint32_t off, uint64_t size, int even)
{
    if (off < MMD_HDR || (even && v->align && (off & 1)) || (uint64_t)off + size > v->modlen)
        return 0;
    if (v->ext != NULL && (uint64_t)off + size > *v->ext)
        *v->ext = (uint64_t)off + size;
    return 1;
}
/* the structure is present, so its contents can be checked */
static int vhave(const mmd_view *v, uint32_t off, uint64_t size)
{
    return (uint64_t)off + size <= v->avail;
}
/* the module type of a header: '0'-'3' ('T': "MCNT", an MMD0 song) */
static int mmd_type(const mmd_view *v, uint32_t h)
{
    int t = (int)v8(v, h + 3);
    return t == 'T' ? '0' : t;
}

/* ------------------------------------------------------------ checks */

/* The song and blocks of the header at 'h'. */
static int mmd_check_song(const mmd_view *v, uint32_t h)
{
    int      type = mmd_type(v, h);
    uint32_t song = vptr(v, h + 8), blockarr = vptr(v, h + 0x10);
    uint32_t numblocks, k;

    if (type < '0' || type > '3')
        return 0;
    if (!vin(v, song, MMD_SONG, 1) || !vin(v, blockarr, 4, 1))
        return 0;
    if (!vhave(v, song, MMD_SONG))
        return 1; /* cut by the end of the input */
    numblocks = v16(v, song + SONG_NUMBLOCKS);
    if (numblocks == 0 || v8(v, song + SONG_NUMSAMPLES) > 63)
        return 0;
    if (!vin(v, blockarr, 4ull * numblocks, 1))
        return 0;

    if (type >= '2')
    {
        /* MMD2song: play sequences, the section list, track volumes/pans */
        uint32_t songlen = v16(v, song + SONG_SONGLEN);
        uint32_t pseqtbl = vptr(v, song + SONG_PSEQTABLE);
        uint32_t sectbl = vptr(v, song + SONG_SECTABLE);
        uint32_t trkvols = vptr(v, song + SONG_TRACKVOLS);
        uint32_t trkpans = vptr(v, song + SONG_TRACKPANS);
        uint32_t numtracks = v16(v, song + SONG_NUMTRACKS);
        uint32_t numpseqs = v16(v, song + SONG_NUMPSEQS);

        if (numpseqs == 0 || !vin(v, pseqtbl, 4ull * numpseqs, 1))
            return 0;
        if (songlen == 0 || !vin(v, sectbl, 2ull * songlen, 1))
            return 0;
        if (numtracks > MMD_MAX_TRACKS)
            return 0;
        if ((trkvols && !vin(v, trkvols, numtracks, 0)) || (trkpans && !vin(v, trkpans, numtracks, 0)))
            return 0;
        if (vhave(v, pseqtbl, 4ull * numpseqs))
            for (k = 0; k < numpseqs; k++)
            {
                if (!vwork(v))
                    return 0;
                uint32_t ps = vptr(v, pseqtbl + 4 * k);
                if (!vin(v, ps, 42, 1))
                    return 0;
                if (vhave(v, ps, 42) && !vin(v, ps, 42 + 2ull * v16(v, ps + 40), 1))
                    return 0;
            }
        if (vhave(v, sectbl, 2ull * songlen))
            for (k = 0; k < songlen; k++)
                if (!vwork(v) || v16(v, sectbl + 2 * k) >= numpseqs)
                    return 0;
    }
    else if (v16(v, song + SONG_SONGLEN) > 256)
        return 0; /* MMD0song: a 256-byte play sequence */

    if (!vhave(v, blockarr, 4ull * numblocks))
        return 1;
    for (k = 0; k < numblocks; k++)
    {
        uint32_t b = vptr(v, blockarr + 4 * k), tracks, lines;
        if (!vwork(v))
            return 0;
        if (b == 0)
            continue;
        if (type == '0')
        {
            /* MMD0 block: u8 tracks, u8 lines - 1, 3-byte notes */
            if (!vin(v, b, 2, 1))
                return 0;
            if (!vhave(v, b, 2))
                continue;
            tracks = v8(v, b);
            lines = v8(v, b + 1) + 1;
            if (tracks == 0 || tracks > MMD_MAX_TRACKS || !vin(v, b, 2 + 3ull * tracks * lines, 1))
                return 0;
        }
        else
        {
            /* MMD1 block: u16 tracks, u16 lines - 1, BlockInfo *, 4-byte notes */
            uint32_t info;
            if (!vin(v, b, 8, 1))
                return 0;
            if (!vhave(v, b, 8))
                continue;
            tracks = v16(v, b);
            lines = v16(v, b + 2) + 1;
            if (tracks == 0 || tracks > MMD_MAX_TRACKS || !vin(v, b, 8 + 4ull * tracks * lines, 1))
                return 0;
            info = vptr(v, b + 4);
            if (info && !vin(v, info, 36, 1))
                return 0;
        }
    }
    return 1;
}

/* The expansion structure of the header at 'h' (MMD0exp): its pointers. */
static int mmd_check_exp(const mmd_view *v, uint32_t h)
{
    static const uint8_t ptrs[] = {0x04, 0x0C, 0x14, 0x20, 0x28, 0x2C, 0x34, 0x38, 0x3C, 0x40};
    uint32_t e = vptr(v, h + 0x20), k;

    if (e == 0)
        return 1;
    if (!vin(v, e, 4, 1))
        return 0;
    if (!vhave(v, e, MMD_EXP_SIZE) || !vin(v, e, MMD_EXP_SIZE, 1))
        return 1; /* only part of it is known */
    for (k = 0; k < sizeof ptrs; k++)
    {
        uint32_t p = vptr(v, e + ptrs[k]);
        if (p && !vin(v, p, 1, 0))
            return 0;
    }
    /* the tables whose sizes are known */
    if (vptr(v, e + 0x04) && !vin(v, vptr(v, e + 0x04), (uint64_t)v16(v, e + 0x08) * v16(v, e + 0x0A), 0))
        return 0; /* InstrExt */
    if (vptr(v, e + 0x0C) && !vin(v, vptr(v, e + 0x0C), v32(v, e + 0x10), 0))
        return 0; /* annotation */
    if (vptr(v, e + 0x14) && !vin(v, vptr(v, e + 0x14), (uint64_t)v16(v, e + 0x18) * v16(v, e + 0x1A), 0))
        return 0; /* MMDInstrInfo */
    if (vptr(v, e + 0x2C) && !vin(v, vptr(v, e + 0x2C), v32(v, e + 0x30), 0))
        return 0; /* song name */
    return 1;
}

/* The instruments (shared by all the songs of a multi-module). */
static int mmd_check_samples(const mmd_view *v)
{
    uint32_t smplarr = vptr(v, 0x18), song = vptr(v, 8), num, k;

    if (smplarr == 0 || !vhave(v, song, MMD_SONG))
        return 1;
    num = v8(v, song + SONG_NUMSAMPLES);
    if (!vin(v, smplarr, 4ull * num, 1))
        return 0;
    if (!vhave(v, smplarr, 4ull * num))
        return 1;
    for (k = 0; k < num; k++)
    {
        uint32_t sp = vptr(v, smplarr + 4 * k), len;
        int      type;

        if (sp == 0)
            continue;
        if (!vin(v, sp, 6, 1))
            return 0;
        if (!vhave(v, sp, 6))
            continue;
        len = v32(v, sp);
        type = (int)(int16_t)v16(v, sp + 4);
        if (type == -1 || type == -2)
        {
            /* synth / hybrid: 278 bytes, then 'wforms' waveform pointers */
            if (!vin(v, sp, 278, 1))
                return 0;
            if (vhave(v, sp, 22) && (v16(v, sp + 20) > 64 || !vin(v, sp, 278 + 4ull * v16(v, sp + 20), 1)))
                return 0;
        }
        else if (type == -3 || type == -4)
        {
            /* HighLife / VSTi (MED Soundstudio for Windows) */
            if (!vin(v, sp, 6ull + len, 1))
                return 0;
        }
        else
        {
            /* a sample: type 0-7 (8: the obsolete 16-bit 0x18), plus the
             * 16-bit, stereo, delta and packed flags; a stereo sample holds
             * both channels of 'len' bytes */
            if (type < 0 || (type & 0xFF00) || (type & 0x0F) > 8)
                return 0;
            if (!vin(v, sp, 6ull + (uint64_t)len * ((type & 0x20) ? 2 : 1), 1))
                return 0;
        }
    }
    return 1;
}

/* The whole module: header, songs (multi-modules too), instruments. */
static int mmd_check(const mmd_view *v)
{
    uint32_t h = 0, k, extra;

    if (v8(v, 0x14) > 1 || v8(v, 0x15) || v8(v, 0x16) || v8(v, 0x17))
        return 0; /* mmdflags: only MMD_LOADTOFASTMEM; reserved bytes */
    if (vptr(v, 0x18) && !vin(v, vptr(v, 0x18), 4, 1))
        return 0;
    if (!mmd_check_song(v, 0) || !mmd_check_exp(v, 0) || !mmd_check_samples(v))
        return 0;

    /* the further songs of a multi-module, as RelocModule finds them */
    extra = v8(v, 0x33);
    for (k = 0; k < extra; k++)
    {
        uint32_t e = vptr(v, h + 0x20), next;
        if (e == 0 || !vhave(v, e, 4))
            break;
        next = vptr(v, e);
        if (next == 0)
            break;
        if (next <= h || !vin(v, next, MMD_HDR, 1))
            return 0; /* forwards only: no cycles */
        if (!vhave(v, next, MMD_HDR))
            break;
        if (v8(v, next) != 'M' || v8(v, next + 1) != 'C' || v8(v, next + 2) != 'N')
            return 0;
        if (!mmd_check_song(v, next) || !mmd_check_exp(v, next))
            return 0;
        h = next;
    }
    return 1;
}

static const char *mmd_name(int type)
{
    switch (type)
    {
    case '0': return "MED (MMD0) module";
    case '1': return "OctaMED (MMD1) module";
    case '2': return "OctaMED Pro (MMD2) module";
    default:  return "OctaMED Soundstudio (MMD3) module";
    }
}

static int mmd_format_id(int type)
{
    return type == '2' ? PWF_MMD2 : type == '3' ? PWF_MMD3 : PWF_MED;
}

/* ------------------------------------------------------------ extent */

/* A module whose modlen is 0 (MED Soundstudio 2 writes some): its size is
 * the end of its furthest structure. mmd_check() already measured the
 * structures it checks (vin()); these are the ones it does not open. */
static void mmd_extent_extra(const mmd_view *v)
{
    uint32_t h = 0, k, j, n, extra = v8(v, 0x33), smplarr = vptr(v, 0x18);
    uint32_t song = vptr(v, 8);

    for (n = 0; n <= extra; n++)
    {
        uint32_t s = vptr(v, h + 8), blockarr = vptr(v, h + 0x10), e = vptr(v, h + 0x20);
        uint32_t numblocks = v16(v, s + SONG_NUMBLOCKS);

        /* blocks: highlight masks, names, command pages */
        if (mmd_type(v, h) != '0')
            for (k = 0; k < numblocks && vwork(v); k++)
            {
                uint32_t b = vptr(v, blockarr + 4 * k), info, pt;
                uint64_t cells = (uint64_t)v16(v, b) * (v16(v, b + 2) + 1);
                if (b == 0 || (info = vptr(v, b + 4)) == 0)
                    continue;
                if (vptr(v, info))
                    vin(v, vptr(v, info), 4ull * ((v16(v, b + 2) + 32) / 32), 0);
                if (vptr(v, info + 4))
                    vin(v, vptr(v, info + 4), v32(v, info + 8), 0);
                pt = vptr(v, info + 12);
                if (pt && vin(v, pt, 4, 0))
                    for (j = 0; j < v16(v, pt) && vwork(v); j++)
                        if (vptr(v, pt + 4 + 4 * j))
                            vin(v, vptr(v, pt + 4 + 4 * j), 2 * cells, 0);
            }
        /* expansion: notation info, colours, attached text, MIDI dumps,
         * command 3x tables */
        if (e)
        {
            uint32_t d;
            if (vptr(v, e + 0x28))
                vin(v, vptr(v, e + 0x28), 108, 0);
            if (vptr(v, e + 0x20))
                vin(v, vptr(v, e + 0x20), 16, 0);
            if ((d = vptr(v, e + 0x38)) != 0)
                vin(v, d, 12ull + v32(v, d + 8), 0);
            if ((d = vptr(v, e + 0x34)) != 0 && vin(v, d, 8ull + 4ull * v16(v, d), 0))
                for (j = 0; j < v16(v, d) && vwork(v); j++)
                {
                    uint32_t dp = vptr(v, d + 8 + 4 * j);
                    if (dp && vin(v, dp, 10ull + v16(v, dp + 8), 0) && vptr(v, dp + 4))
                        vin(v, vptr(v, dp + 4), v32(v, dp), 0);
                }
            if ((d = vptr(v, e + 0x40)) != 0 && vin(v, d, 12, 0))
            {
                if (vptr(v, d + 4))
                    vin(v, vptr(v, d + 4), v16(v, d + 2), 0);
                if (vptr(v, d + 8))
                    vin(v, vptr(v, d + 8), 2ull * v16(v, d + 2), 0);
            }
        }
        if (e == 0 || n == extra || (h = vptr(v, e)) == 0)
            break;
    }

    /* waveforms of synth and hybrid sounds (relative to the sound) */
    if (smplarr)
        for (k = 0; k < v8(v, song + SONG_NUMSAMPLES); k++)
        {
            uint32_t sp = vptr(v, smplarr + 4 * k);
            int      type = sp ? (int)(int16_t)v16(v, sp + 4) : 0;
            if (type != -1 && type != -2)
                continue;
            for (j = 0; j < v16(v, sp + 20) && j < 64; j++)
            {
                uint32_t wf = v32(v, sp + 278 + 4 * j) + sp;
                if (wf == sp)
                    continue;
                if (type == -2 && j == 0)
                    vin(v, wf, 6ull + v32(v, wf), 0);
                else
                    vin(v, wf, 2 + 2ull * v16(v, wf), 0);
            }
        }
}

/* ------------------------------------------------------------ files */

int pw_new_MMD23(pw_ctx *c)
{
    mmd_view v;
    int64_t  s = (int64_t)c->i;
    int      type = (int)pw_rd8(c, s + 3);
    uint32_t modlen = pw_rd32(c, s + 4);
    uint64_t left = (uint64_t)c->in_size - (uint64_t)s;
    uint64_t ext = 0;
    uint32_t work = MMD_WORK;

    if (type != '2' && type != '3')
        return 0;
    v.c = c;
    v.buf = NULL;
    v.start = s;
    v.base = 0;
    v.align = 1;
    v.ext = NULL;
    v.work = &work;
    if (modlen == 0)
    {
        /* no size stored: measure the structures (all must be present) */
        v.modlen = v.avail = (uint32_t)(left < MMD_LIMIT ? left : MMD_LIMIT);
        v.ext = &ext;
        if (!mmd_check(&v))
            return 0;
        mmd_extent_extra(&v);
        modlen = (uint32_t)((ext + 1) & ~(uint64_t)1);
        if (modlen < MMD_MIN || modlen > v.avail)
            return 0;
    }
    else
    {
        if (modlen < MMD_MIN || modlen > MMD_LIMIT)
            return 0;
        v.modlen = modlen;
        v.avail = (uint32_t)(left < modlen ? left : modlen);
        if (!mmd_check(&v))
            return 0;
    }

    c->start = (int32_t)s;
    c->out_size = modlen;
    c->convert = PW_BAD;
    pw_save_rip(c, mmd_name(type), mmd_format_id(type));
    pw_skip_rip(c);
    return 1;
}

/* ------------------------------------------------------------ relocated */

/* Undoes RelocModule on 'out', a copy of the module seen by 'v'. The walk
 * repeats reloc.a's, reading the pointers as relocated (from 'v') and
 * writing them back as offsets, so a structure reached twice is only fixed
 * once. Where reloc.a loops on a count with its own quirks (a byte
 * decrement for the sample and block tables, 65536 rounds for a zero
 * waveform, page or play sequence count) the same slots are fixed; anything
 * outside the module was not part of it. The number of slots visited is
 * bounded: hostile tables can repeat one another endlessly. */
typedef struct
{
    const mmd_view *v;
    uint8_t        *out;
    uint64_t        work;   /* slots left to visit: hostile tables repeat */
} mmd_undo;

/* one more slot to visit; 0 once the budget is spent */
static int undo_more(mmd_undo *u)
{
    if (u->work == 0)
        return 0;
    u->work--;
    return 1;
}

static void undo_put(mmd_undo *u, uint64_t slot, uint32_t value)
{
    if (slot + 4 > u->v->modlen)
        return;
    u->out[slot] = (uint8_t)(value >> 24);
    u->out[slot + 1] = (uint8_t)(value >> 16);
    u->out[slot + 2] = (uint8_t)(value >> 8);
    u->out[slot + 3] = (uint8_t)value;
}

/* reloc.a "relocentr": a non-null pointer had the base added */
static void undo_ptr(mmd_undo *u, uint64_t slot)
{
    uint32_t p;
    if (slot + 4 > u->v->modlen || !undo_more(u))
        return;
    p = v32(u->v, slot);
    if (p)
        undo_put(u, slot, p - u->v->base);
}

/* is the structure a relocated pointer leads to part of the module? */
static int undo_inside(const mmd_undo *u, uint64_t slot, uint32_t *off)
{
    uint32_t p;
    if (slot + 4 > u->v->modlen)
        return 0;
    p = v32(u->v, slot);
    if (p == 0)
        return 0;
    *off = p - u->v->base;
    return *off < u->v->modlen;
}

static void undo_module(mmd_undo *u, uint32_t h, int first)
{
    const mmd_view *v = u->v;
    int      type = mmd_type(v, h);
    uint32_t song = 0, off, k, count;

    /* header: song, blockarr, smplarr, expdata */
    undo_ptr(u, h + 0x08);
    undo_ptr(u, h + 0x10);
    undo_ptr(u, h + 0x18);
    undo_ptr(u, h + 0x20);
    if (!undo_inside(u, h + 0x08, &song) || (uint64_t)song + MMD_SONG > v->modlen)
        return; /* reloc.a would have run off elsewhere: nothing more here */

    /* instruments (first module only): the table, then the waveform
     * pointers of synth / hybrid sounds, which get the address of their
     * own instrument added, null or not */
    if (first && undo_inside(u, h + 0x18, &off))
    {
        uint32_t smplarr = off;
        count = ((v8(v, song + SONG_NUMSAMPLES) - 1) & 0xFF) + 1; /* subq.b: 0 means 256 */
        for (k = 0; k < count && u->work; k++)
        {
            uint32_t sp, wforms, j;
            undo_ptr(u, smplarr + 4ull * k);
            if (!undo_inside(u, smplarr + 4ull * k, &sp) || (uint64_t)sp + 22 > v->modlen)
                continue;
            if ((int16_t)v16(v, sp + 4) >= 0)
                continue;
            wforms = v16(v, sp + 20);
            if (wforms == 0)
                wforms = 0x10000; /* dbf on -1 */
            for (j = 0; j < wforms && (uint64_t)sp + 278 + 4ull * j + 4 <= v->modlen && undo_more(u); j++)
            {
                uint64_t slot = (uint64_t)sp + 278 + 4ull * j;
                undo_put(u, slot, v32(v, slot) - (v->base + sp));
            }
        }
    }

    /* blocks: the table (with reloc.a's byte-sized loop counter), then the
     * BlockInfo of MMD1+ blocks, walking back from the end of that loop */
    if (undo_inside(u, h + 0x10, &off))
    {
        uint32_t blockarr = off, numblocks = v16(v, song + SONG_NUMBLOCKS);
        count = ((numblocks & 0xFF00) | ((numblocks - 1) & 0xFF)) + 1;
        for (k = 0; k < count && u->work; k++)
            undo_ptr(u, blockarr + 4ull * k);
        if (type != '0' && numblocks <= 0x8000) /* subq.w / bmi */
            for (k = 0; k < numblocks && k < count && u->work; k++)
            {
                uint32_t b, info, pt, pages, j;
                uint64_t slot = blockarr + 4ull * (count - 1 - k);
                if (!undo_inside(u, slot, &b))
                    continue;
                undo_ptr(u, (uint64_t)b + 4);
                if (!undo_inside(u, (uint64_t)b + 4, &info))
                    continue;
                undo_ptr(u, (uint64_t)info);      /* hlmask */
                undo_ptr(u, (uint64_t)info + 4);  /* block name */
                undo_ptr(u, (uint64_t)info + 12); /* command page table */
                if (!undo_inside(u, (uint64_t)info + 12, &pt) || (uint64_t)pt + 2 > v->modlen)
                    continue;
                pages = v16(v, pt);
                if (pages == 0)
                    pages = 0x10000;
                for (j = 0; j < pages && (uint64_t)pt + 4 + 4ull * j + 4 <= v->modlen && u->work; j++)
                    undo_ptr(u, (uint64_t)pt + 4 + 4ull * j);
            }
    }

    /* MMD2song: its tables, then each play sequence */
    if (type >= '2')
    {
        uint32_t pseqtbl, numpseqs = v16(v, song + SONG_NUMPSEQS);
        undo_ptr(u, song + SONG_PSEQTABLE);
        undo_ptr(u, song + SONG_SECTABLE);
        undo_ptr(u, song + SONG_TRACKVOLS);
        undo_ptr(u, song + SONG_TRACKPANS);
        if (undo_inside(u, song + SONG_PSEQTABLE, &pseqtbl))
        {
            if (numpseqs == 0)
                numpseqs = 0x10000;
            for (k = 0; k < numpseqs && (uint64_t)pseqtbl + 4ull * k + 4 <= v->modlen && u->work; k++)
                undo_ptr(u, pseqtbl + 4ull * k);
        }
    }

    /* MMD0exp: its pointers, the MIDI dumps, ARexx and command 3x data */
    if (undo_inside(u, h + 0x20, &off) && (uint64_t)off + 0x44 <= v->modlen)
    {
        static const uint8_t ptrs[] = {0x00, 0x04, 0x0C, 0x14, 0x20, 0x28, 0x2C, 0x34, 0x38, 0x3C, 0x40};
        uint32_t e = off, d, n;
        for (k = 0; k < sizeof ptrs; k++)
            undo_ptr(u, (uint64_t)e + ptrs[k]);
        if (undo_inside(u, (uint64_t)e + 0x34, &d) && (uint64_t)d + 8 <= v->modlen)
        {
            n = v16(v, d); /* MMDDumpData.numdumps, then the dump pointers */
            for (k = 0; k < n && (uint64_t)d + 8 + 4ull * k + 4 <= v->modlen && u->work; k++)
            {
                uint32_t dp;
                undo_ptr(u, (uint64_t)d + 8 + 4ull * k);
                if (undo_inside(u, (uint64_t)d + 8 + 4ull * k, &dp))
                    undo_ptr(u, (uint64_t)dp + 4); /* MMDDump.data */
            }
        }
        if (undo_inside(u, (uint64_t)e + 0x3C, &d))
        {
            uint32_t t;
            undo_ptr(u, (uint64_t)d + 4); /* the first trigger command */
            if (undo_inside(u, (uint64_t)d + 4, &t))
            {
                undo_ptr(u, (uint64_t)t + 8);  /* command */
                undo_ptr(u, (uint64_t)t + 12); /* port */
                /* reloc.a then follows 'next' unrelocated: never inside */
            }
        }
        if (undo_inside(u, (uint64_t)e + 0x40, &d))
        {
            undo_ptr(u, (uint64_t)d + 4); /* controller types */
            undo_ptr(u, (uint64_t)d + 8); /* controller numbers */
        }
    }
}

/* the header fields the play routine writes, as a file has them */
static void mmd_reset_player(uint8_t *m, uint32_t h)
{
    memset(m + h + 0x0C, 0, 4);    /* psecnum, pseq */
    memset(m + h + 0x28, 0, 8);    /* pstate, pblock, pline, pseqnum */
    m[h + 0x30] = 0xFF;            /* actplayline = -1 */
    m[h + 0x31] = 0xFF;
    m[h + 0x32] = 0;               /* counter */
}

static uint8_t *mmd_restore(pw_ctx *c, const mmd_view *v)
{
    mmd_undo u;
    uint8_t *out = pw_copy_in(c, v->start, v->modlen);
    uint32_t h = 0, k, extra = v8(v, 0x33);

    if (out == NULL)
        return NULL;
    u.v = v;
    u.work = (uint64_t)v->modlen / 2 + MMD_WORK;
    u.out = out;
    undo_module(&u, 0, 1);
    mmd_reset_player(out, 0);
    /* RelocModule goes on with each further song while songs are left */
    for (k = 0; k < extra; k++)
    {
        uint32_t e, next;
        if (!undo_inside(&u, h + 0x20, &e) || !undo_inside(&u, e, &next))
            break;
        if (next <= h || (uint64_t)next + MMD_HDR > v->modlen)
            break; /* forwards only: no cycles */
        undo_module(&u, next, 0);
        mmd_reset_player(out, next);
        h = next;
    }
    return out;
}

/* A cheap look at the song for a candidate load address. */
static int mmd_quick(const mmd_view *v, int type)
{
    uint32_t song = vptr(v, 8), blockarr = vptr(v, 0x10), b;

    if (!vin(v, song, MMD_SONG, 1) || !vhave(v, song, MMD_SONG) || !vin(v, blockarr, 4, 1))
        return 0;
    if (v16(v, song + SONG_NUMBLOCKS) == 0 || v8(v, song + SONG_NUMSAMPLES) > 63)
        return 0;
    b = vptr(v, blockarr);
    if (b && !vin(v, b, 2, 1))
        return 0;
    if (type >= '2')
    {
        uint32_t pseqtbl = vptr(v, song + SONG_PSEQTABLE);
        if (v16(v, song + SONG_NUMPSEQS) == 0 || !vin(v, pseqtbl, 4, 1) ||
            !vin(v, vptr(v, pseqtbl), 42, 1))
            return 0;
    }
    return 1;
}

int pw_new_MMD_relocated(pw_ctx *c)
{
    mmd_view v;
    int64_t  s = (int64_t)c->i;
    int      type = (int)pw_rd8(c, s + 3);
    uint32_t modlen = pw_rd32(c, s + 4);
    uint64_t left = (uint64_t)c->in_size - (uint64_t)s;
    uint32_t p[4], k, tries, full = 0, work = MMD_WORK;
    int64_t  lo, hi, base;
    int      found = 0;
    uint8_t *out;

    if (type < '0' || type > '3' || modlen < MMD_MIN || modlen > MMD_LIMIT)
        return 0;
    if (pw_rd8(c, s + 0x14) > 1 || pw_rd8(c, s + 0x15) || pw_rd8(c, s + 0x16) || pw_rd8(c, s + 0x17))
        return 0;

    /* The load address B: every non-null header pointer p was an offset in
     * [34, modlen), so B lies in [max(p) - modlen + 1, min(p) - 34], and the
     * song (0x314 bytes) must fit. The first structure after the header is
     * usually the song (MMD0/1), so the search starts at min(p) - 34 and
     * walks down, i.e. tries the song further and further into the module. */
    p[0] = pw_rd32(c, s + 8);
    p[1] = pw_rd32(c, s + 0x10);
    p[2] = pw_rd32(c, s + 0x18);
    p[3] = pw_rd32(c, s + 0x20);
    if (p[0] == 0 || p[1] == 0 || (type >= '2' && ((p[0] | p[1] | p[2] | p[3]) & 1)))
        return 0;
    lo = (int64_t)p[0] + MMD_SONG - modlen;
    hi = (int64_t)p[0] - MMD_HDR;
    for (k = 1; k < 4; k++)
        if (p[k])
        {
            if ((int64_t)p[k] - modlen + 1 > lo)
                lo = (int64_t)p[k] - modlen + 1;
            if ((int64_t)p[k] - MMD_HDR < hi)
                hi = (int64_t)p[k] - MMD_HDR;
        }
    if (lo < MMD_MIN_BASE)
        lo = MMD_MIN_BASE;
    if (hi & 1)
        hi--;
    if (lo > hi)
        return 0;

    v.c = c;
    v.buf = NULL;
    v.start = s;
    v.modlen = modlen;
    v.avail = (uint32_t)(left < modlen ? left : modlen);
    v.align = type >= '2';
    v.ext = NULL;
    v.work = &work;
    for (base = hi, tries = 0; base >= lo && tries < MMD_MAX_TRIES && full < MMD_MAX_FULL; base -= 2, tries++)
    {
        v.base = (uint32_t)base;
        if (!mmd_quick(&v, type))
            continue;
        full++;
        if (mmd_check(&v))
        {
            found = 1;
            break;
        }
    }
    if (!found)
        return 0;

    c->start = (int32_t)s;
    c->out_size = modlen;
    c->convert = PW_BAD;
    if (v.avail < modlen)
    {
        /* cut by the end of the input: nothing to restore, report it */
        pw_save_rip(c, mmd_name(type), mmd_format_id(type));
        pw_skip_rip(c);
        return 1;
    }

    out = mmd_restore(c, &v);
    if (out == NULL)
        return 0;
    {
        /* the restored module must pass the checks of a file */
        mmd_view r = v;
        uint32_t work2 = MMD_WORK;
        r.buf = out;
        r.base = 0;
        r.work = &work2;
        if (!mmd_check(&r))
        {
            free(out);
            return 0;
        }
    }
    pw_save_rip_data(c, mmd_name(type), mmd_format_id(type), out, modlen, 1);
    free(out);
    pw_skip_rip(c);
    return 1;
}
