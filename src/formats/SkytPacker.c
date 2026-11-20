/*
 * SkytPacker.c - SKYT Packer (upstream r/SkytPacker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 *
 * Even in these poor lines, I managed to insert a bug :)
 *
 * update: 19/04/00
 *  - bug correction (really testing volume !)
 */
#include "formats.h"

int16_t pw_test_SKYT(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 256)
        return PW_BAD;

    /* test 2 */
    c->wss = 0;
    c->start = (int32_t)(c->i - 256);
    for (c->j = 0; c->j < 31; c->j++)
    {
        if (IN(c->start + 3 + 8 * c->j) > 0x40)
            return PW_BAD;
        c->wss += (((IN(c->start + 8 * c->j) * 256) + IN(c->start + 1 + 8 * c->j)) * 2);
    }

    return PW_GOOD;
}

void pw_rip_SKYT(pw_ctx *c)
{
    /* c->wss is still the whole sample size */
    c->l = 0;
    c->j = IN(c->start + 260) + 1;
    for (c->k = 0; c->k < (c->j * 4); c->k++)
    {
        c->m = (IN(c->start + 262 + c->k * 2));
        if (c->m > c->l)
            c->l = c->m;
    }
    c->out_size = (c->l * 256) + 262 + c->wss + (c->j * 8);

    c->convert = PW_GOOD;
    pw_save_rip(c, "SKYT Packed module", PWF_SKYT_packer);

    if (c->save_status == PW_GOOD)
        c->i += 257;
}

/*
 *   Skyt_Packer.c   1997 (c) Asle / ReDoX
 *
 * Converts back to ptk SKYT packed MODs
 ********************************************************
 * 13 april 1999 : Update
 *   - no more open() of input file ... so no more fread() !.
 *     It speeds-up the process quite a bit :).
 * 28 Nov 1999 : Update
 *   - Speed & Size optimizings
 * 19 Apr 2000 : Update
 *   - address of samples data bug correction
 *     (thx to Thomas Neumann)
 * 29 Nov 2003 : Update
 *   - another bug removed :(.
 * 2 Aug 2010 :
 *   - cleaned up a bit
 *   - track 0 was not correctly handled
 * 3 Aug 2010 :
 *   - rewrote patternlist generation
 */

/* Header[] of the original is a 1084 byte heap block, but the pattern list
 * (up to 255 entries) is written and read back at 952+i, past its end. */
#define SKYT_HDR (1084 + 256)

void pw_depack_SKYT(pw_ctx *c)
{
    uint8_t  header[SKYT_HDR];
    uint8_t  pattern[1024];
    uint8_t  poss[37][2];
    int32_t  i = 0, j = 0, k = 0, l = 0, m = 0;
    int32_t  total_sample_size = 0;
    uint32_t read_trk_pat[128][4], read_pat[128];
    int32_t  where = c->start;
    int32_t  highest_track = 0;
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    memset(read_trk_pat, 0, sizeof read_trk_pat);
    memset(read_pat, 0, sizeof read_pat);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(header, 0, sizeof header);
    memset(pattern, 0, sizeof pattern);

    /* read and write sample descriptions */
    for (i = 0; i < 31; i++)
    {
        total_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);
        /* write Size,Fine,Vol & Loop start */
        header[42 + i * 30] = IN(where);
        header[43 + i * 30] = IN(where + 1);
        header[44 + i * 30] = IN(where + 2);
        header[45 + i * 30] = IN(where + 3);
        header[46 + i * 30] = IN(where + 4);
        header[47 + i * 30] = IN(where + 5);
        header[48 + i * 30] = IN(where + 6);
        header[49 + i * 30] = IN(where + 7);
        /* loop size */
        if ((IN(where + 6) == 0x00) && (IN(where + 7) == 0x00))
            header[49 + i * 30] = 0x01;
        where += 8;
    }

    /* go to track addresses list */
    where = c->start + 260;

    /* pattern table lenght */
    header[950] = (uint8_t)(IN(where) + 1);
    where += 1;

    /* write NoiseTracker byte */
    header[951] = 0x7F;

    /* pattern list */
    /* those are tracks, being saved. Pattern list must be regenerated */
    /* each track is 256 bytes (4*64 notes) */
    /* ReadPat[] and ReadTrkPat[] have 128 entries but the list can hold
     * 255: entries past the end are dropped and read as 0 */
    for (i = 0; i < header[950]; i++)
    {
        uint32_t t0, t1, t2, t3;

        if (i < 128)
        {
            read_pat[i] = ((uint32_t)IN(where + i * 8 + 1) << 24) +
                          ((uint32_t)IN(where + 3 + i * 8) << 16) +
                          ((uint32_t)IN(where + 5 + i * 8) << 8) +
                          IN(where + 7 + i * 8);
            read_trk_pat[i][0] = (uint32_t)(IN(where + i * 8 + 1) - 1);
            read_trk_pat[i][1] = (uint32_t)(IN(where + i * 8 + 3) - 1);
            read_trk_pat[i][2] = (uint32_t)(IN(where + i * 8 + 5) - 1);
            read_trk_pat[i][3] = (uint32_t)(IN(where + i * 8 + 7) - 1);
            t0 = read_trk_pat[i][0];
            t1 = read_trk_pat[i][1];
            t2 = read_trk_pat[i][2];
            t3 = read_trk_pat[i][3];
        }
        else
            t0 = t1 = t2 = t3 = 0;
        if ((t0 != 0xffffffff) && (t0 > (uint32_t)highest_track))
            highest_track = (int32_t)t0;
        if ((t1 != 0xffffffff) && (t1 > (uint32_t)highest_track))
            highest_track = (int32_t)t1;
        if ((t2 != 0xffffffff) && (t2 > (uint32_t)highest_track))
            highest_track = (int32_t)t2;
        if ((t3 != 0xffffffff) && (t3 > (uint32_t)highest_track))
            highest_track = (int32_t)t3;
    }

    /* sorting ?*/
    k = 0; /* next min */
    l = 0;

    /* put the first pattern number */
    /* could be stored several times */
    for (j = 0; j < header[950]; j++)
    {
        m = 0x7fffffff; /* min */
        /* search for min */
        for (i = 0; i < header[950]; i++)
        {
            uint32_t rp = (i < 128) ? read_pat[i] : 0;
            if ((rp < (uint32_t)m) && (rp > (uint32_t)k))
                m = (int32_t)rp;
        }
        /* if k == m then an already existing ref was found */
        if (k == m)
            continue;
        /* m is the next minimum */
        k = m;
        for (i = 0; i < header[950]; i++)
            if (((i < 128) ? read_pat[i] : 0) == (uint32_t)k)
                header[952 + i] = (uint8_t)l;
        l++;
    }

    /* write ptk's ID */
    header[1080] = 'M';
    header[1081] = header[1083] = '.';
    header[1082] = 'K';
    pw_fwrite(out, header, 1084);

    /* get track address */
    where = c->start + 261 + (header[950] * 8) + 1;

    /* l is the number of stored patterns */
    /* rebuild pattern data now */
    for (i = 0; i < l - 1; i++)
    {
        memset(pattern, 0, sizeof pattern);
        /* which pattern is it now ? */
        for (j = 0; j < header[950]; j++)
        {
            if (header[952 + j] == i)
                break; /* found */
        }
        for (k = 0; k < 4; k++) /* loop on 4 tracks' refs */
        {
            int32_t  d;
            uint32_t trk = (j < 128) ? read_trk_pat[j][k] : 0;

            /* empty track */
            if (trk == 0xffffffff)
                continue;

            /* loop on notes */
            for (d = 0; d < 64; d++)
            {
                uint8_t note = IN((uint32_t)(where + (trk * 256) + d * 4));
                /* poss[] has 37 entries; the note byte is not checked */
                pattern[k * 4 + d * 16] = (note < 37) ? poss[note][0] : 0;
                pattern[k * 4 + d * 16 + 1] = (note < 37) ? poss[note][1] : 0;
                /* samples */
                pattern[k * 4 + d * 16] |= (IN((uint32_t)(where + (trk * 256) + d * 4 + 1))) & 0xf0;
                pattern[k * 4 + d * 16 + 2] = (uint8_t)((IN((uint32_t)(where + (trk * 256) + d * 4 + 1))) << 4);
                pattern[k * 4 + d * 16 + 2] += IN((uint32_t)(where + (trk * 256) + d * 4 + 2));
                pattern[k * 4 + d * 16 + 3] = IN((uint32_t)(where + (trk * 256) + d * 4 + 3));
            }
        }
        pw_fwrite(out, pattern, 1024);
    }

    /* sample data */
    where += (highest_track + 1) * 256;
    pw_fwrite_in(out, where, total_sample_size);

    /* crap */
    pw_crap("   SKYT Packer    ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
