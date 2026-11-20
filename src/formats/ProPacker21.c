/*
 * ProPacker21.c - ProPacker v2.1 (upstream r/ProPacker21.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 *
 * 5th, oct 2001 : strenghtened a test that hanged the prog sometimes
 */
#include <stdio.h>

#include "formats.h"

int16_t pw_test_PP21(pw_ctx *c)
{
    /* test #1 */
    if (c->i < 3)
        return PW_BAD;
    if ((c->i + 891) >= (uint32_t)c->in_size)
        return PW_BAD;

    /* test #2 */
    c->start = (int32_t)(c->i - 3);
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
    {
        c->k = (((IN(c->start + c->j * 8) * 256) + IN(c->start + 1 + c->j * 8)) * 2);
        c->wss += c->k;
        /* finetune > 0x0f ? */
        if (IN(c->start + 2 + 8 * c->j) > 0x0f)
            return PW_BAD;
        /* loop start > size ? */
        c->l = ((IN(c->start + 4 + c->j * 8) * 256) + IN(c->start + 5 + c->j * 8)) * 2;
        if (c->l != 0xFFFF)
        {
            if ((uint32_t)(((IN(c->start + 4 + c->j * 8) * 256) + IN(c->start + 5 + c->j * 8)) * 2) > c->k)
                return PW_BAD;
        }
    }
    if (c->wss <= 2)
        return PW_BAD;

    /* test #3   about size of pattern list */
    c->l = IN(c->start + 248);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;

    /* get the highest track value */
    c->k = 0;
    for (c->j = 0; c->j < 512; c->j++)
    {
        c->l = IN(c->start + 250 + c->j);
        if (c->l > c->k)
            c->k = c->l;
    }
    /* c->k is the highest track number */
    c->k += 1;
    c->k *= 64;
    if ((c->k * 2) + c->start + 763 > (uint32_t)c->in_size)
        return PW_BAD;

    /* test #4  track data value > $4000 ? */
    c->m = 0;
    for (c->j = 0; c->j < c->k; c->j++)
    {
        c->l = (IN(c->start + 762 + c->j * 2) * 256) + IN(c->start + 763 + c->j * 2);
        if (c->l > c->m)
            c->m = c->l;
        if (c->l > 0x4000)
            return PW_BAD;
    }

    /* test #5  reference table size *4 ? */
    c->k *= 2;
    c->l = (IN(c->start + c->k + 762) * 256 * 256 * 256) + (IN(c->start + c->k + 763) * 256 * 256) +
           (IN(c->start + c->k + 764) * 256) + IN(c->start + c->k + 765);
    if (c->l != ((c->m + 1) * 4))
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_PP21(pw_ctx *c)
{
    /* c->k is still the size of the track "data" ! */
    /* c->wss is still the whole sample size */

    c->l = (IN(c->start + 762 + c->k) * 256 * 256 * 256) + (IN(c->start + 763 + c->k) * 256 * 256) +
           (IN(c->start + 764 + c->k) * 256) + IN(c->start + 765 + c->k);

    c->out_size = c->wss + c->k + c->l + 766;

    c->convert = PW_GOOD;
    pw_save_rip(c, "ProPacker v2.1 module", PWF_Propacker_21);

    if (c->save_status == PW_GOOD)
        c->i += 4; /* 3 should do but call it "just to be sure" :) */
}

/*
 *   ProPacker_21.c   1997 (c) Asle / ReDoX
 *
 * Converts PP21 packed MODs back to PTK MODs
 * thanks to Gryzor and his ProWizard tool ! ... without it, this prog
 * would not exist !!!
 *
 * update : 26/11/1999 by Sylvain "Asle" Chipaux
 *   - reduced to only one FREAD.
 *   - Speed-up and Binary smaller.
 * update : 8 dec 2003
 *   - no more fopen ()
 * update : 20100814
 *   - rewrote depacker (no more useless patterns)
 * update : 20100815
 *   - removed unused var and ;
 * update : 23 aug 2010
 *   - fixed yet another patternlist bug
 */
void pw_depack_PP21(pw_ctx *c)
{
    uint8_t  Header[1084];
    uint8_t  Pattern[1024];
    uint32_t ReadTrkPat[128][4], ReadPat[128];
    int32_t  Highest_Track = 0;
    int32_t  whereTableRef;
    int32_t  i = 0, j = 0, k = 0, l = 0, m = 0;
    int32_t  Total_Sample_Size = 0;
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

    for (i = 0; i < 31; i++)
    {
        Total_Sample_Size += (((IN(Where) * 256) + IN(Where + 1)) * 2);
        /* siz,fine,vol,lstart,lsize */
        Header[42 + i * 30] = IN(Where);
        Header[43 + i * 30] = IN(Where + 1);
        Header[44 + i * 30] = IN(Where + 2);
        Header[45 + i * 30] = IN(Where + 3);
        Header[46 + i * 30] = IN(Where + 4);
        Header[47 + i * 30] = IN(Where + 5);
        Header[48 + i * 30] = IN(Where + 6);
        Header[49 + i * 30] = IN(Where + 7);
        Where += 8;
    }

    /* pattern table lenght */
    Header[950] = IN(Where);
    Where += 1;

    /* NoiseTracker restart byte */
    Header[951] = IN(Where);
    Where += 1;

    /* now, where = 0xFA */
    for (i = 0; i < 128; i++)
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
    /* (the test limited the list to 127 entries) */
    for (j = 0; j < Header[950]; j++)
    {
        m = 0x7fffffff; /* min */
        /*search for min */
        for (i = 0; (i < Header[950]) && (i < 128); i++)
            if ((ReadPat[i] < (uint32_t)m) && (ReadPat[i] > (uint32_t)k))
                m = (int32_t)ReadPat[i];
        /* if k == m then an already existing ref was found */
        if (k == m)
            continue;
        /* m is the next minimum */
        k = m;
        for (i = 0; (i < Header[950]) && (i < 128); i++)
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

    /* rebuild patterns now */
    /* l is the number of stored patterns */
    /* rebuild pattern data now */
    whereTableRef = ((Highest_Track + 1) * 128) + 4 + Where;
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
                int32_t val = (IN(Where + (ReadTrkPat[j][k] * 128) + (d * 2))) * 256 +
                              IN(Where + (ReadTrkPat[j][k] * 128) + (d * 2) + 1);

                Pattern[k * 4 + d * 16] = IN(whereTableRef + (val * 4));
                Pattern[k * 4 + d * 16 + 1] = IN(whereTableRef + (val * 4) + 1);
                Pattern[k * 4 + d * 16 + 2] = IN(whereTableRef + (val * 4) + 2);
                Pattern[k * 4 + d * 16 + 3] = IN(whereTableRef + (val * 4) + 3);
            }
        }
        pw_fwrite(out, Pattern, 1024);
    }

    /* locate sample start addy - after reference table */
    j = ((IN(whereTableRef - 4) * 256 * 256 * 256) + (IN(whereTableRef - 3) * 256 * 256) +
         (IN(whereTableRef - 2) * 256) + IN(whereTableRef - 1));

    /* sample data */
    pw_fwrite_in(out, j + whereTableRef, Total_Sample_Size);

    pw_crap("  ProPacker v2.1  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
