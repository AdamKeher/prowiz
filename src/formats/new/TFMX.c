/*
 * TFMX.c - TFMX (Chris Huelsbeck / Peter Thierolf): mdat (+ smpl), and the
 *          single-file containers TFHD and TFMXPAK
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * The music data ("mdat") starts with a 200-byte header:
 *
 *   000 "TFMX-SONG " (also "TFMX-SONG" + 0, "TFMX_SONG "), "tfmxsong",
 *       or the old "TFMX " (TFMX 1.x)
 *   010 6 x 40 characters of text
 *   100/140/180 32 x u16 song start step / end step / tempo
 *   1D0/1D4/1D8 u32 offsets of the trackstep, the pattern table and the
 *       macro table; all 0 ("unpacked"): 800, 400 (128 entries), 600 (128)
 *
 * Sizes (Wanted Team's EaglePlayers compute them the same way):
 *   unpacked: the editor stores all 128 macros, the last one last, so the
 *     module ends after the first 07000000 (macro "stop") at or after the
 *     start of macro 127.
 *   packed: the macro table is the last thing in the file, right after the
 *     pattern table; it ends at the first entry that is not an ascending
 *     offset below the pattern table.
 *
 * The samples ("smpl") are headerless 8-bit data the macros address by
 * offset (02 SetBegin / 03 SetLen, 22 / 23 for the SID synth). Nothing in the
 * mdat gives the smpl's place or size, and the player keeps its address in
 * its own variables. So a smpl is only saved when it follows the mdat
 * directly, and it ends at the highest sample end any macro uses: exact for
 * 277 of the 368 Modland pairs; for the others the smpl file holds more
 * (sample sets shared by several songs) or less (damaged copies) than the
 * macros reach. It must start with 4 zero bytes (365/368; the player clears
 * them anyway), fit in the input and not run into another TFMX header.
 *
 * Variant names: "7V" when the trackstep uses the 7-voice command EFFE 0003,
 * "1.5" for the old header or an unpacked "(Empty)" text (as UADE decides),
 * "Pro" otherwise. The layout is the same for all three.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "new_formats.h"

#define TFMX_MAXOFF   0x100000u   /* table entries / mdat size: real ones < 64 KB */
#define TFMX_MAXSTOP  0x4000u     /* 4-byte steps searched for the last "stop"     */
#define TFMX_MACSTEPS 1024u       /* steps read per macro for the sample bound     */

typedef struct
{
    int      packed;
    int      old_tag;     /* "TFMX " */
    uint32_t ts, pt, mt;  /* trackstep, pattern table, macro table */
    uint32_t npat, nmac;
    uint32_t size;        /* mdat size, can run past the input */
    int      complete;    /* the size is known (not cut by the end of input) */
    uint32_t smpl;        /* highest sample end used by a macro */
    int      v7, v15;
} tfmx_info;

/* 1 for a TFMX header tag at 'off', 2 for the old "TFMX " one, 0 for none */
static int tfmx_tag(const pw_ctx *c, int64_t off)
{
    if (pw_match(c, off, "TFMX-SONG", 9) &&
        (pw_rd8(c, off + 9) == ' ' || pw_rd8(c, off + 9) == 0))
        return 1;
    if (pw_match(c, off, "TFMX_SONG ", 10) || pw_match(c, off, "tfmxsong", 8))
        return 1;
    if (pw_match(c, off, "TFMX ", 5))
        return 2;
    return 0;
}

/* highest offset + length into the smpl any macro reaches */
static uint32_t tfmx_smpl_bound(const pw_ctx *c, int64_t start, const tfmx_info *t)
{
    uint32_t m, k, nmac = t->packed ? t->nmac : 128, best = 0;
    int64_t  end = start + t->size;

    if (end > (int64_t)c->in_size)
        end = c->in_size;
    for (m = 0; m < nmac; m++)
    {
        uint32_t o = pw_rd32(c, start + t->mt + 4 * (int64_t)m);
        int64_t  p = start + o;
        int      have = 0;
        uint32_t begin = 0;

        if (o == 0 || o >= t->size)
            continue;
        for (k = 0; k < TFMX_MACSTEPS && p + 4 <= end; k++, p += 4)
        {
            uint32_t w = pw_rd32(c, p), cmd = w >> 24, arg = w & 0xFFFFFF, e;

            if (cmd == 0x07)
                break;
            if (cmd == 0x02 || cmd == 0x22)
            {
                begin = arg;
                have = 1;
                continue;
            }
            if (!have || (cmd != 0x03 && cmd != 0x23))
                continue;
            e = begin + (cmd == 0x03 ? 2 * (arg & 0xFFFF) : (arg & 0xFFFF));
            if (e > best)
                best = e;
        }
    }
    return best;
}

/* Validate an mdat at 'start' and work out its size. 0 if it is not one. */
static int tfmx_parse(const pw_ctx *c, int64_t start, tfmx_info *t)
{
    uint32_t k, v, prev;
    int      tag = tfmx_tag(c, start);

    memset(t, 0, sizeof *t);
    if (tag == 0)
        return 0;
    t->old_tag = (tag == 2);

    /* the first song: start <= end <= 1FF */
    if (pw_rd16(c, start + 0x100) > pw_rd16(c, start + 0x140) || pw_rd16(c, start + 0x140) > 0x1FF)
        return 0;

    t->ts = pw_rd32(c, start + 0x1D0);
    t->pt = pw_rd32(c, start + 0x1D4);
    t->mt = pw_rd32(c, start + 0x1D8);

    if ((t->ts | t->pt | t->mt) == 0)
    {
        uint32_t m127;
        int64_t  p, avail;

        t->packed = 0;
        t->ts = 0x800;
        t->pt = 0x400;
        t->mt = 0x600;
        t->npat = 128;
        /* 200-3FF unused (138/138 Modland files) */
        for (k = 0x200; k < 0x400; k += 4)
            if (pw_rd32(c, start + k) != 0)
                return 0;
        prev = 0x800;
        for (k = 0; k < 128; k++)
        {
            v = pw_rd32(c, start + 0x400 + 4 * k);
            if (v < prev || (v & 1) != 0 || v >= TFMX_MAXOFF)
                return 0;
            prev = v;
        }
        for (k = 0; k < 128; k++)
        {
            v = pw_rd32(c, start + 0x600 + 4 * k);
            if (v < 0x800 || (v & 1) != 0 || v >= TFMX_MAXOFF)
                return 0;
        }
        m127 = pw_rd32(c, start + 0x7FC);
        avail = (int64_t)c->in_size - start;
        t->complete = 0;
        for (k = 0, p = m127; k < TFMX_MAXSTOP && p + 4 <= avail; k++, p += 4)
            if (pw_rd32(c, start + p) == 0x07000000)
            {
                t->size = (uint32_t)p + 4;
                t->complete = 1;
                break;
            }
        if (!t->complete)
        {
            if (p + 4 <= avail)
                return 0;   /* no stop where there should be one */
            /* cut by the end of the input: at least an empty macro 127 */
            t->size = (uint32_t)(avail + 4 > (int64_t)m127 + 8 ? avail + 4 : (int64_t)m127 + 8);
        }
    }
    else
    {
        uint32_t lastpat = 0;

        if (t->ts == 0 || t->pt == 0 || t->mt == 0)
            return 0;   /* e.g. little-endian offsets of PC conversions */
        t->packed = 1;
        if (t->ts < 0x200 || t->ts >= t->pt || t->pt > t->mt || t->mt >= TFMX_MAXOFF ||
            ((t->mt - t->pt) & 3) != 0 || (t->mt - t->pt) / 4 > 128)
            return 0;
        t->npat = (t->mt - t->pt) / 4;
        prev = t->ts + 1;
        for (k = 0; k < t->npat; k++)
        {
            v = pw_rd32(c, start + t->pt + 4 * k);
            if (v < prev || v >= t->pt || (v & 1) != 0)
                return 0;
            prev = lastpat = v;
        }
        /* macro table: ascending offsets below the pattern table, up to the
         * first entry that is not one (or the end of the input) */
        prev = lastpat;
        for (k = 0; k < 128 && start + t->mt + 4 * (int64_t)k + 4 <= (int64_t)c->in_size; k++)
        {
            v = pw_rd32(c, start + t->mt + 4 * (int64_t)k);
            if (v == 0 || v < prev || v >= t->pt || (v & 1) != 0)
                break;
            prev = v;
        }
        t->nmac = k;
        t->size = t->mt + 4 * k;
        t->complete = 1;
        if (k == 0 && start + t->mt + 4 > (int64_t)c->in_size)
        {
            t->size = t->mt + 4;  /* the table is past the end of the input */
            t->complete = 0;
        }
    }

    /* 7V: the trackstep uses EFFE 0003 (lines of 16 bytes up to the first
     * pattern) */
    {
        uint32_t tsend = t->npat ? pw_rd32(c, start + t->pt) : t->size;
        uint32_t lines = tsend > t->ts ? (tsend - t->ts) / 16 : 0;
        if (lines > 0x1000)
            lines = 0x1000;
        for (k = 0; k < lines; k++)
            if (pw_rd32(c, start + t->ts + 16 * (int64_t)k) == 0xEFFE0003)
            {
                t->v7 = 1;
                break;
            }
    }
    t->v15 = !t->v7 && (t->old_tag || (!t->packed && pw_match(c, start + 0x10, "(Empty)         ", 16)));
    t->smpl = tfmx_smpl_bound(c, start, t);
    return 1;
}

static const char *tfmx_variant(const tfmx_info *t)
{
    return t->v7 ? "7V" : t->v15 ? "1.5" : "Pro";
}

/* a smpl right after the mdat: its size, or 0 */
static uint32_t tfmx_find_smpl(const pw_ctx *c, int64_t at, uint32_t size)
{
    int64_t p;

    if (size < 4 || at + size > (int64_t)c->in_size || pw_rd32(c, at) != 0)
        return 0;
    /* not the next module (a stray header copy inside sample data, as in
     * Modland's "smpl.z-out level 4", does not count) */
    for (p = at; p + 4 <= at + size; p++)
    {
        uint32_t  b = pw_rd8(c, p);
        tfmx_info t;
        if ((b == 'T' || b == 't') && tfmx_tag(c, p) != 0 && tfmx_parse(c, p, &t))
            return 0;
    }
    return size;
}

/* TFHD: "TFHD", u32 header size, u8 version hint, u8 0, u32 mdat size,
 * u32 smpl size; TFMXPAK: "TFMXPAK <mdat size> <smpl size> >>>". The mdat
 * follows the header, the smpl the mdat. */
static int tfmx_single(pw_ctx *c)
{
    int64_t   start = c->i;
    uint32_t  hdr, msize, ssize;
    tfmx_info t;
    char      name[64];
    const char *kind;

    if (pw_match(c, start, "TFHD", 4))
    {
        hdr = pw_rd32(c, start + 4);
        if (hdr < 0x12 || hdr > 0x400 || (pw_rd8(c, start + 8) & 0x7F) > 3 || pw_rd8(c, start + 9) != 0)
            return 0;
        msize = pw_rd32(c, start + 0x0A);
        ssize = pw_rd32(c, start + 0x0E);
        kind = "TFHD";
    }
    else if (pw_match(c, start, "TFMXPAK ", 8))
    {
        uint32_t n[2] = {0, 0}, k, pos = 8, digits;

        for (k = 0; k < 2; k++)
        {
            while (pos < 32 && pw_rd8(c, start + pos) == ' ')
                pos++;
            for (digits = 0; pos < 32 && digits < 9; pos++, digits++)
            {
                uint32_t ch = pw_rd8(c, start + pos);
                if (ch < '0' || ch > '9')
                    break;
                n[k] = n[k] * 10 + (ch - '0');
            }
            if (digits == 0)
                return 0;
        }
        while (pos < 32 && pw_rd8(c, start + pos) == ' ')
            pos++;
        if (pos > 29 || !pw_match(c, start + pos, ">>>", 3))
            return 0;
        hdr = pos + 3;
        msize = n[0];
        ssize = n[1];
        kind = "TFMXPAK";
    }
    else
        return 0;

    if (msize < 0x200 || msize > TFMX_MAXOFF || ssize == 0 || ssize > 0x2000000u)
        return 0;
    if (!tfmx_parse(c, start + hdr, &t) || (t.complete && t.size > msize))
        return 0;

    snprintf(name, sizeof name, "TFMX %s module (%s single file)", tfmx_variant(&t), kind);
    c->start = (int32_t)start;
    c->out_size = hdr + msize + ssize;
    c->convert = PW_BAD;
    pw_save_rip(c, name, PWF_TFMXSingle);
    pw_skip_rip(c);
    return 1;
}

int pw_new_TFMX(pw_ctx *c)
{
    tfmx_info t;
    char      name[64];
    uint32_t  smpl = 0;
    uint32_t  b1 = pw_rd8(c, (int64_t)c->i + 1);

    if (b1 != 'F' && b1 != 'f')
        return 0;
    if (tfmx_single(c))
        return 1;
    if (!tfmx_parse(c, c->i, &t))
        return 0;

    snprintf(name, sizeof name, "TFMX %s module", tfmx_variant(&t));
    c->start = (int32_t)c->i;
    c->out_size = t.size;
    c->convert = PW_BAD;
    pw_save_rip(c, name, PWF_TFMX);
    if (c->save_status == PW_GOOD && t.complete)
    {
        smpl = tfmx_find_smpl(c, (int64_t)c->start + t.size, t.smpl);
        if (smpl != 0)
            pw_save_companion_in(c, "smpl", (uint32_t)c->start + t.size, smpl);
    }
    pw_skip_rip(c);
    if (smpl != 0 && c->save_status == PW_GOOD)
    {
        /* go on after the samples too */
        uint32_t next = (uint32_t)c->start + t.size + smpl - 1;
        if (next > c->i && next < (uint32_t)c->in_size)
            c->i = next;
    }
    return 1;
}
