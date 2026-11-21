/*
 * Symphonie.c - Symphonie / Symphonie Pro (Patrick Meng)
 *              (not in Pro-Wizard 1.70a; see docs/NEW_FORMATS.md).
 *
 * A SymMOD is "SymM", a version (1), then "hunks", each starting with a
 * signed 32-bit type (Symphonie source: MODHEADTYPE_*, LoadModuleParts,
 * SaveActualModule):
 *
 *   value hunks   type + u32 value: -1 channels, -2 track length, -3
 *                 patterns/positions, -4 sample slots (low word), -5 note
 *                 size, -6 speed, -7 "song" (no samples), 10-12 (Pro)
 *   -12           empty sample slot: the type alone
 *   block hunks   type + u32 length + data: -10 positions, -11 sample file,
 *                 -13 notes, -14 instruments, -15 sequences, -16 text,
 *                 -17 / -18 delta-coded 8 / 16-bit sample, -19 to -21 info
 *   0             end of file (the loader honours it; the saver never
 *                 writes it)
 *
 * A block may be RLE-packed ("PACK" FF FF, u32 unpacked length, data); its
 * length counts all of that, and delta coding keeps the length, so packing
 * never changes the size computation.
 *
 * Nothing stores the total size and nothing marks the end, so it follows
 * the saver: header values, the song blocks, then one hunk per sample slot
 * (-4, low word) except the right half of each stereo pair, which has none.
 * Counting those needs the instrument list (-14, 256 bytes per slot), which
 * is decoded when packed. A "song" (-7, saved as .SymSONG) has no sample
 * hunks: its instruments are file names, loaded from disk. It is ripped too,
 * up to its last song block (-15, then optionally -16, -20 + -19).
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define SYM_MAX_HUNKS   1024
#define SYM_MAX_SLOTS   256
#define SYM_INSTR_SIZE  256
#define SYM_LIMIT       0x40000000u /* far beyond any real module */

/* hunk types */
#define SYM_POSITIONS   (-10)
#define SYM_SAMPLE      (-11)
#define SYM_EMPTY       (-12)
#define SYM_NOTES       (-13)
#define SYM_INSTRUMENTS (-14)
#define SYM_SEQUENCES   (-15)
#define SYM_TEXT        (-16)
#define SYM_DELTA8      (-17)
#define SYM_DELTA16     (-18)
#define SYM_INFOTYPE    (-19)
#define SYM_INFOOBJ     (-20)
#define SYM_STRING      (-21)

/* the "PACK" FF FF header of a packed block at 'd' (length 'len'): returns
 * 1 and the unpacked length, or 0 if the block is stored as is */
static int sym_packed(const pw_ctx *c, int64_t d, uint32_t len, uint32_t *unpacked)
{
    if (len >= 10 && pw_match(c, d, "PACK\xff\xff", 6))
    {
        *unpacked = pw_rd32(c, d + 6);
        return 1;
    }
    *unpacked = len;
    return 0;
}

/* Decodes a packed block (OpenMPT DecodeSymChunk; Symphonie PackData) into
 * 'out' (exactly 'size' bytes). Returns 0 on bad data. */
static int sym_unpack(const pw_ctx *c, int64_t d, uint32_t len, uint8_t *out, uint32_t size)
{
    int64_t  p = d + 10, end = d + len;
    uint32_t o = 0, k, n;

    while (p < end)
    {
        int type = (int)(int8_t)pw_rd8(c, p++);
        switch (type)
        {
        case 0: /* n raw bytes */
            n = pw_rd8(c, p++);
            if (n > size - o || p + n > end)
                return 0;
            for (k = 0; k < n; k++)
                out[o++] = pw_rd8(c, p++);
            break;
        case 1: /* a longword n times */
            n = pw_rd8(c, p++);
            if ((uint64_t)n * 4 > size - o || p + 4 > end)
                return 0;
            for (k = 0; k < n * 4; k++)
                out[o++] = pw_rd8(c, p + (k & 3));
            p += 4;
            break;
        case 2: /* a longword twice */
            if (8 > size - o || p + 4 > end)
                return 0;
            for (k = 0; k < 8; k++)
                out[o++] = pw_rd8(c, p + (k & 3));
            p += 4;
            break;
        case 3: /* n zero bytes */
            n = pw_rd8(c, p++);
            if (n > size - o)
                return 0;
            memset(out + o, 0, n);
            o += n;
            break;
        case -1:
            return o == size;
        default:
            return 0;
        }
    }
    return o == size;
}

/* Number of sample slots the saver writes nothing for: right halves of
 * stereo pairs (instrument name set, type not "kill" (-4), +84 == 2).
 * Returns -1 if the instrument list is not valid. */
static int sym_stereo_right(pw_ctx *c, int64_t d, uint32_t len, uint32_t slots)
{
    uint32_t size, r, nrec;
    uint8_t *buf;
    int      count = 0, ok = 1;

    if (sym_packed(c, d, len, &size))
    {
        if (size == 0 || size > SYM_MAX_SLOTS * SYM_INSTR_SIZE || size % SYM_INSTR_SIZE)
            return -1;
        buf = (uint8_t *)malloc(size);
        if (buf == NULL)
        {
            c->fatal = PW_ERR_NOMEM;
            return -1;
        }
        ok = sym_unpack(c, d, len, buf, size);
    }
    else
    {
        if (size == 0 || size > SYM_MAX_SLOTS * SYM_INSTR_SIZE || size % SYM_INSTR_SIZE)
            return -1;
        buf = pw_copy_in(c, d, size);
        if (buf == NULL)
            return -1;
    }
    nrec = size / SYM_INSTR_SIZE;
    if (ok && nrec < slots)
        ok = 0; /* the saver writes one record per slot */
    for (r = 0; ok && r < slots; r++)
    {
        const uint8_t *in = buf + (size_t)r * SYM_INSTR_SIZE;
        if (in[0] != 0 && (int8_t)in[0x80] != -4 && in[0x84] == 2)
            count++;
    }
    free(buf);
    return ok ? count : -1;
}

int pw_new_Symphonie(pw_ctx *c)
{
    int64_t  s = (int64_t)c->i;
    uint64_t avail, pos;
    uint32_t channels, slots = 0, samples = 0, expected = 0, blocks = 0;
    int      n, phase = 0, song = 0, truncated = 0, have_expected = 0, tracklen = 0;
    int      done = 0;
    enum { B_POS = 1, B_NOTES = 2, B_INSTR = 4, B_SEQ = 8, B_TEXT = 16, B_OBJ = 32, B_TYPE = 64 };

    /* "SymM", version 1, then the channel count hunk */
    if (!pw_match(c, s, "SymM", 4) || pw_rd32(c, s + 4) != 1 || pw_rd32(c, s + 8) != 0xFFFFFFFFu)
        return 0;
    channels = pw_rd32(c, s + 12);
    if (channels < 1 || channels > 256)
        return 0;

    avail = (uint64_t)c->in_size - (uint64_t)s;
    pos = 16;
    for (n = 0; n < SYM_MAX_HUNKS && !done; n++)
    {
        int32_t  type;
        uint32_t len, v;
        uint64_t end;

        if (pos + 4 > avail)
        {
            /* the input ends here; anything still missing makes it cut */
            if (!(song ? (blocks & B_SEQ) : (have_expected && samples == expected)))
            {
                truncated = 1;
                pos += 4;
            }
            break;
        }
        type = (int32_t)pw_rd32(c, s + (int64_t)pos);

        /* the song of a .SymSONG ends after its last block */
        if (song && (blocks & B_SEQ))
        {
            if (!((type == SYM_TEXT && !(blocks & (B_TEXT | B_OBJ))) ||
                  (type == SYM_INFOOBJ && !(blocks & B_OBJ)) ||
                  (type == SYM_INFOTYPE && (blocks & B_OBJ) && !(blocks & B_TYPE))))
                break;
        }

        switch (type)
        {
        case -1: case -2: case -3: case -4: case -5: case -6: case -7:
        case 10: case 11: case 12:
            if (phase != 0)
            {
                done = 1; /* values only come first */
                continue;
            }
            if (pos + 8 > avail)
            {
                truncated = 1;
                pos += 8;
                done = 1;
                continue;
            }
            v = pw_rd32(c, s + (int64_t)pos + 4);
            if (type == -1 && (v < 1 || v > 256))
                return 0;
            if (type == -2)
            {
                if (v < 1 || v > 1024)
                    return 0;
                tracklen = 1;
            }
            if (type == -4)
            {
                slots = v & 0xFFFF;
                if (slots < 1 || slots > SYM_MAX_SLOTS)
                    return 0;
            }
            if (type == -5 && (v & 0xFFFF) != 4)
                return 0;
            if (type == -7)
                song = 1;
            pos += 8;
            break;

        case 0: /* end of file mark */
            pos += 4;
            done = 1;
            break;

        case SYM_EMPTY:
            if (song || !(blocks & B_INSTR) || !have_expected || phase == 0)
                return 0;
            phase = 2;
            samples++;
            pos += 4;
            break;

        case SYM_POSITIONS: case SYM_NOTES: case SYM_INSTRUMENTS: case SYM_SEQUENCES:
        case SYM_TEXT: case SYM_INFOTYPE: case SYM_INFOOBJ: case SYM_STRING:
        case SYM_SAMPLE: case SYM_DELTA8: case SYM_DELTA16:
        {
            int sample = (type == SYM_SAMPLE || type == SYM_DELTA8 || type == SYM_DELTA16);
            uint32_t unpacked;

            if (sample)
            {
                if (song || !(blocks & B_INSTR) || !have_expected)
                    return 0;
                phase = 2;
            }
            else
            {
                if (phase == 2)
                {
                    done = 1; /* song blocks come before the samples */
                    continue;
                }
                phase = 1;
            }
            if (pos + 8 > avail)
            {
                truncated = 1;
                pos += 8;
                done = 1;
                continue;
            }
            len = pw_rd32(c, s + (int64_t)pos + 4);
            end = pos + 8 + len;
            if (end > SYM_LIMIT)
                return 0;
            if (end > avail)
            {
                /* cut by the end of the input: the instruments are needed */
                if (type == SYM_INSTRUMENTS || !tracklen)
                    return 0;
                truncated = 1;
                pos = end;
                done = 1;
                continue;
            }
            sym_packed(c, s + (int64_t)pos + 8, len, &unpacked);
            switch (type)
            {
            case SYM_POSITIONS:
                if ((blocks & B_POS) || unpacked == 0 || unpacked % 32)
                    return 0;
                blocks |= B_POS;
                break;
            case SYM_NOTES:
                if ((blocks & B_NOTES) || unpacked == 0 || unpacked % 4)
                    return 0;
                blocks |= B_NOTES;
                break;
            case SYM_SEQUENCES:
                if ((blocks & B_SEQ) || unpacked == 0 || unpacked % 16)
                    return 0;
                blocks |= B_SEQ;
                break;
            case SYM_INSTRUMENTS:
            {
                int right;
                if ((blocks & B_INSTR) || slots == 0 || !tracklen)
                    return 0;
                blocks |= B_INSTR;
                right = sym_stereo_right(c, s + (int64_t)pos + 8, len, slots);
                if (right < 0)
                    return 0;
                expected = slots - (uint32_t)right;
                have_expected = 1;
                break;
            }
            case SYM_TEXT:
                blocks |= B_TEXT;
                break;
            case SYM_INFOOBJ:
                blocks |= B_OBJ;
                break;
            case SYM_INFOTYPE:
                blocks |= B_TYPE;
                break;
            default:
                if (sample)
                    samples++;
                break;
            }
            pos = end;
            break;
        }

        default: /* not a hunk: the module ended before it */
            done = 1;
            continue;
        }

        if (phase == 2 && samples == expected)
            done = 1; /* every sample slot is there */
    }

    /* the parts every module has */
    if (!tracklen || !(blocks & B_INSTR))
        return 0;
    if (!truncated)
    {
        if ((blocks & (B_POS | B_NOTES | B_SEQ)) != (B_POS | B_NOTES | B_SEQ))
            return 0;
        if (!song && samples != expected)
            return 0;
    }

    c->start = (int32_t)s;
    c->out_size = (uint32_t)pos;
    c->convert = PW_BAD;
    pw_save_rip(c, song ? "Symphonie song (samples not included)" : "Symphonie module", PWF_Symphonie);
    pw_skip_rip(c);
    return 1;
}
