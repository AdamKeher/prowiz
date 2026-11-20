/*
 * ProPacker10.c - ProPacker v1.0 (upstream r/ProPacker10.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "formats.h"

/*
 * 20100902 : better note test to lower the number of fake found
 * 20100902 : test if patternlist ends with 0x00 values
 */
int16_t pw_test_PP10(pw_ctx *c)
{
    /* test #1 */
    if ((c->i < 3) || ((c->i + 246) >= (uint32_t)c->in_size))
        return PW_BAD;
    c->start = (int32_t)(c->i - 3);

    /* noisetracker byte */
    if (IN(c->start + 249) > 0x7f)
        return PW_BAD;

    /* test #2 */
    /* various test about sample headers */
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
    {
        c->k = (((IN(c->start + c->j * 8) * 256) + IN(c->start + 1 + c->j * 8)) * 2);
        c->l = (((IN(c->start + c->j * 8 + 4) * 256) + IN(c->start + 5 + c->j * 8)) * 2);
        /* loop size */
        c->m = (((IN(c->start + c->j * 8 + 6) * 256) + IN(c->start + 7 + c->j * 8)) * 2);
        if (c->m == 0)
            return PW_BAD;
        if ((c->l != 0) && (c->m <= 2))
            return PW_BAD;
        if ((c->l + c->m) > (c->k + 2))
            return PW_BAD;
        if ((c->l != 0) && (c->m == 0))
            return PW_BAD;
        c->wss += c->k;
        /* finetune > 0x0f ? */
        if (IN(c->start + 2 + 8 * c->j) > 0x0f)
            return PW_BAD;
        /* volume > 0x40 ? */
        if (IN(c->start + 3 + 8 * c->j) > 0x40)
            return PW_BAD;
        /* loop start > size ? */
        if ((uint32_t)(((IN(c->start + 4 + c->j * 8) * 256) + IN(c->start + 5 + c->j * 8)) * 2) > c->k)
            return PW_BAD;
        /* size > 0xffff ? */
        if (c->k > 0xFFFF)
            return PW_BAD;
    }

    /* test if sample data is too small */
    if (c->wss <= 2)
        return PW_BAD;
    /* c->wss = whole sample size */

    /* test #3   about size of pattern list */
    c->m = IN(c->start + 248);
    if ((c->m > 127) || (c->m == 0))
        return PW_BAD;

    /* get the highest track value */
    /* c->m is the patterlist size */
    c->k = 0;
    for (c->n = 0; c->n < 4; c->n++)
    {
        for (c->j = 0; c->j < 128; c->j++)
        {
            c->l = IN(c->start + 250 + c->j + (c->n * 128));
            if (c->l > c->k)
                c->k = c->l;
            /* 20100902:test if patternlist ends with 0x00 values */
            if ((c->j > c->m) && (IN(c->start + 250 + c->j + (c->n * 128)) != 0x00))
                return PW_BAD;
        }
    }
    /* c->k is the highest track number */
    c->k += 1;
    c->k *= 64;

    /* test if it's not outside the file */
    if (c->start + 762 + (c->k * 4) > (uint32_t)c->in_size)
        return PW_BAD;
    /* track data test */
    c->l = 0;
    for (c->j = 0; c->j < c->k; c->j++)
    { /*20010902:better note test*/
        if ((IN(c->start + 762 + (c->j * 4)) & 0xf0) > 0x10)
            return PW_BAD;
        if ((IN(c->start + 762 + (c->j * 4)) & 0x0f) > 0x03)
            return PW_BAD;
        if (((IN(c->start + 762 + c->j * 4) & 0x0f) == 0x00) && (IN(c->start + 763 + c->j * 4) < 0x71) &&
            (IN(c->start + 763 + c->j * 4) != 0x00))
            return PW_BAD;
        if (((IN(c->start + 762 + c->j * 4) & 0x0f) != 0x00) || (IN(c->start + 763 + c->j * 4) != 0x00))
            c->l = 1;
    }
    if (c->l == 0)
    {
        /* only some empty patterns */
        return PW_BAD;
    }
    c->k *= 4;

    /* c->wss is the sample data size */
    /* c->k is the track data size */
    return PW_GOOD;
}

void pw_rip_PP10(pw_ctx *c)
{
    /* c->k is still the size of the track data */
    /* c->wss is still the sample data size */

    c->out_size = c->wss + c->k + 762;

    c->convert = PW_GOOD;
    pw_save_rip(c, "ProPacker v1.0 Exe-file", PWF_PP10);

    if (c->save_status == PW_GOOD)
        c->i += 4; /* 3 should do but call it "just to be sure" :) */
}

/*
 *   ProPacker_v1.0   1997 (c) Asle / ReDoX
 *
 * Converts back to ptk ProPacker v1 MODs
 *
 * Update: 28/11/99
 *     - removed fopen() and all attached functions.
 *     - overall speed and size optimizings.
 * Update: 19/04/00 (all pointed out by Thomas Neumann)
 *     - replen bug correction
 * Update: 15/08/10
 *     - rewrote depacker for patternlist generation
 * update : 23 aug 2010
 *   - fixed yet another patternlist bug
 */
void pw_depack_PP10(pw_ctx *c)
{
    uint8_t  Header[1084];
    uint8_t  Pattern[1024];
    uint32_t ReadTrkPat[128][4], ReadPat[128];
    int32_t  Highest_Track = 0;
    int32_t  i = 0, j = 0, k = 0, l = 0, m = 0;
    int32_t  WholeSampleSize = 0;
    int32_t  Where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(ReadTrkPat, 0, sizeof ReadTrkPat);
    memset(ReadPat, 0, sizeof ReadPat);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(Header, 0, sizeof Header);
    memset(Pattern, 0, sizeof Pattern);

    /* read and write sample descriptions */
    for (i = 0; i < 31; i++)
    {
        WholeSampleSize += (((IN(Where) * 256) + IN(Where + 1)) * 2);
        /* siz,fine,vol,lstart,lsize */
        Header[42 + i * 30] = IN(Where);
        Header[43 + i * 30] = IN(Where + 1);
        Header[44 + i * 30] = IN(Where + 2);
        Header[45 + i * 30] = IN(Where + 3);
        Header[46 + i * 30] = IN(Where + 4);
        Header[47 + i * 30] = IN(Where + 5);
        Header[48 + i * 30] = IN(Where + 6);
        Header[49 + i * 30] = IN(Where + 7);
        if ((IN(Where + 6) == 0x00) && (IN(Where + 7) == 0x00))
            Header[49 + i * 30] = 0x01;
        Where += 8;
    }

    /* read and write pattern table lenght */
    Header[950] = IN(Where);
    Where += 1;

    /* read and write NoiseTracker byte */
    Header[951] = IN(Where);
    Where += 1;

    /* now, where = 0xFA */
    /* (the test limited the list to 127 entries) */
    for (i = 0; (i < Header[950]) && (i < 128); i++)
    {
        ReadPat[i] = (IN(Where + i) * 256 * 256 * 256) + (IN(Where + i + 128) * 256 * 256) +
                     (IN(Where + i + 256) * 256) + IN(Where + i + 384);
        ReadTrkPat[i][0] = IN(Where + i);
        ReadTrkPat[i][1] = IN(Where + i + 128);
        ReadTrkPat[i][2] = IN(Where + i + 256);
        ReadTrkPat[i][3] = IN(Where + i + 384);
        if (ReadTrkPat[i][0] > (uint32_t)Highest_Track)
            Highest_Track = (int32_t)ReadTrkPat[i][0];
        if (ReadTrkPat[i][1] > (uint32_t)Highest_Track)
            Highest_Track = (int32_t)ReadTrkPat[i][1];
        if (ReadTrkPat[i][2] > (uint32_t)Highest_Track)
            Highest_Track = (int32_t)ReadTrkPat[i][2];
        if (ReadTrkPat[i][3] > (uint32_t)Highest_Track)
            Highest_Track = (int32_t)ReadTrkPat[i][3];
    }

    /* sorting ?*/
    k = 0; /* next min */
    l = 0;

    /* put the first pattern number */
    /* could be stored several times */
    for (j = 0; j < Header[950]; j++)
    {
        m = 0x7fffffff; /* min */
        /*search for min */
        for (i = 0; i < Header[950]; i++)
            if ((ReadPat[i] < (uint32_t)m) && (ReadPat[i] > (uint32_t)k))
                m = (int32_t)ReadPat[i];
        /* if k == m then an already existing ref was found */
        if (k == m)
            continue;
        /* m is the next minimum */
        k = m;
        for (i = 0; i < Header[950]; i++)
            if (ReadPat[i] == (uint32_t)k)
            {
                Header[952 + i] = (unsigned char)l;
                j++;
            }
        j--;
        l++;
    }

    /* write ptk's ID */
    Header[1080] = 'M';
    Header[1081] = Header[1083] = '.';
    Header[1082] = 'K';
    pw_fwrite(out, Header, 1084);

    /* put 'where' at track data level - after track list */
    Where = c->start + 762;

    /* track/pattern data */

    for (i = 0; i < l; i++)
    {
        memset(Pattern, 0, sizeof Pattern);
        /* which pattern is it now ? */
        for (j = 0; j < Header[950]; j++)
        {
            if (Header[952 + j] == i)
                break; /* found */
        }
        for (k = 0; k < 4; k++) /* loop on 4 tracks' refs*/
        {
            int32_t d;

            /* loop on notes */
            for (d = 0; d < 64; d++)
            {
                /* read one ref value to be fetch in the reference table */
                int32_t val = (int32_t)(Where + (ReadTrkPat[j][k] * 256) + (d * 4));

                Pattern[k * 4 + d * 16] = IN(val);
                Pattern[k * 4 + d * 16 + 1] = IN(val + 1);
                Pattern[k * 4 + d * 16 + 2] = IN(val + 2);
                Pattern[k * 4 + d * 16 + 3] = IN(val + 3);
            }
        }
        pw_fwrite(out, Pattern, 1024);
    }

    /* now, lets put file pointer at the beginning of the sample datas */
    Where = c->start + 762 + ((Highest_Track + 1) * 256);

    /* sample data */
    pw_fwrite_in(out, Where, WholeSampleSize);

    /* crap */
    pw_crap("  ProPacker v1.0  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
