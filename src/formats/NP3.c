/*
 * NP3.c - NoisePacker v3 (upstream r/NP3.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_Noisepacker3(pw_ctx *c)
{
    if (c->i < 9)
        return PW_BAD;
    c->start = (int32_t)(c->i - 9);

    /* size of the pattern table */
    c->j = (IN(c->start + 2) * 256) + IN(c->start + 3);
    if ((((c->j / 2) * 2) != c->j) || (c->j == 0))
        return PW_BAD;
    /* c->j is the size of the pattern list (*2) */

    /* test nbr of samples */
    if ((IN(c->start + 1) & 0x0f) != 0x0C)
        return PW_BAD;
    c->l = ((IN(c->start) << 4) & 0xf0) | ((IN(c->start + 1) >> 4) & 0x0f);
    if ((c->l > 0x1F) || (c->l == 0) || ((c->start + 8 + c->j + c->l * 8) > (uint32_t)c->in_size))
        return PW_BAD;
    /* c->l is the number of samples */

    /* test volumes */
    for (c->k = 0; c->k < c->l; c->k++)
    {
        if (IN(c->start + 9 + c->k * 16) > 0x40)
            return PW_BAD;
    }

    /* test sample sizes */
    c->wss = 0;
    for (c->k = 0; c->k < c->l; c->k++)
    {
        c->o = (IN(c->start + c->k * 16 + 14) * 256) + IN(c->start + c->k * 16 + 15);
        c->m = (IN(c->start + c->k * 16 + 20) * 256) + IN(c->start + c->k * 16 + 21);
        c->n = (IN(c->start + c->k * 16 + 22) * 256) + IN(c->start + c->k * 16 + 23);
        c->o *= 2;
        c->m *= 2;
        c->n *= 2;
        if ((c->o > 0xFFFF) || (c->m > 0xFFFF) || (c->n > 0xFFFF))
            return PW_BAD;
        if ((c->m + c->n) > (c->o + 2))
            return PW_BAD;
        if ((c->n != 0) && (c->m == 0))
            return PW_BAD;
        c->wss += c->o;
    }
    if (c->wss <= 4)
        return PW_BAD;

    /* small shit to gain some vars */
    c->l *= 16;
    c->l += 8;
    c->l += 4;
    /* c->l is now the size of the header 'til the end of sample descriptions */
    if ((c->l + c->start) > (uint32_t)c->in_size)
        return PW_BAD;

    /* test pattern table */
    c->n = 0;
    for (c->k = 0; c->k < c->j; c->k += 2)
    {
        c->m = ((IN(c->start + c->l + c->k) * 256) + IN(c->start + c->l + c->k + 1));
        if (((c->m / 8) * 8) != c->m)
            return PW_BAD;
        if (c->m > c->n)
            c->n = c->m;
    }
    c->l += c->j;
    c->l += c->n;
    c->l += 8; /* paske on a que l'address du dernier pattern .. */
    /* c->l is now the size of the header 'til the end of the track list */
    /* c->n is the highest pattern number (*8) */

    /* test track data size */
    c->k = (IN(c->start + 6) * 256) + IN(c->start + 7);
    if ((c->k <= 63) || ((c->k + c->l + c->start) > (uint32_t)c->in_size))
        return PW_BAD;

    /* test notes */
    /* re-calculate the number of sample */
    /* c->k is the track data size */
    c->j = ((IN(c->start) << 4) & 0xf0) | ((IN(c->start + 1) >> 4) & 0x0f);
    for (c->m = 0; c->m < c->k; c->m++)
    {
        if ((IN(c->start + c->l + c->m) & 0x80) == 0x80)
            continue;
        /* si note trop grande et si effet = A */
        if ((IN(c->start + c->l + c->m) > 0x49) ||
            ((IN(c->start + c->l + c->m + 1) & 0x0f) == 0x0A))
            return PW_BAD;
        /* si effet D et arg > 0x64 */
        if (((IN(c->start + c->l + c->m + 1) & 0x0f) == 0x0D) &&
            (IN(c->start + c->l + c->m + 2) > 0x64))
            return PW_BAD;
        /* all is empty ?!? ... cannot be ! */
        if ((IN(c->start + c->l + c->m) == 0) &&
            (IN(c->start + c->l + c->m + 1) == 0) &&
            (IN(c->start + c->l + c->m + 2) == 0) && (c->m < (c->k - 3)))
            return PW_BAD;
        c->m += 2;
    }

    /* c->wss is the size of the sample data */
    /* c->l is the size of the header 'til the track datas */
    /* c->k is the size of the track datas */
    return PW_GOOD;
}

void pw_rip_Noisepacker3(pw_ctx *c)
{
    c->out_size = c->k + c->wss + c->l;

    c->convert = PW_GOOD;
    pw_save_rip(c, "NoisePacker v3 module", PWF_Noisepacker3);

    if (c->save_status == PW_GOOD)
        c->i += 16; /* 15 should do but call it "just to be sure" :) */
}

/*
 *   NoisePacker_v3.c   1998 (c) Asle / ReDoX
 * Converts NoisePacked MODs back to ptk
 * update : 23/08/10
 *   - Whatever wasn't cleaned up when writing patternlist (showed only if one pattern)
 */
void pw_depack_Noisepacker3(pw_ctx *c)
{
    uint8_t  whatever[1084];
    uint8_t  c1, c2, c3, c4;
    uint8_t  nbr_pos;
    uint8_t  nbr_smp;
    uint8_t  poss[37][2];
    uint8_t  pat_max;
    int32_t  where = c->start;
    int32_t  whole_sample_size = 0;
    int32_t  track_addresses[256][4];
    int32_t  i, j, k;
    int32_t  track_data_start_address;
    int32_t  sample_data_address = 0;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    pw_fill_ptk_table(poss);
    memset(track_addresses, 0, sizeof track_addresses);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* read number of sample */
    nbr_smp = (uint8_t)(((IN(where) << 4) & 0xf0) | ((IN(where + 1) >> 4) & 0x0f));

    /* the whole header is built in whatever[] (title stays empty) */
    memset(whatever, 0, sizeof whatever);

    /* read size of pattern list */
    nbr_pos = (uint8_t)(IN(where + 3) / 2);

    /* bytes 4-5: unknown; bytes 6-7: track data size (not needed here) */

    /* read sample descriptions (the test allows at most 31 samples) */
    where += 8;
    for (i = 0; i < nbr_smp; i++)
    {
        /* size */
        whatever[(i * 30) + 42] = IN(where + 6);
        whatever[(i * 30) + 43] = IN(where + 7);
        whole_sample_size += (((IN(where + 6) * 256) + IN(where + 7)) * 2);
        /* finetune,vol */
        whatever[(i * 30) + 44] = IN(where);
        whatever[(i * 30) + 45] = IN(where + 1);
        /* loop start */
        whatever[(i * 30) + 46] = IN(where + 14);
        whatever[(i * 30) + 47] = IN(where + 15);
        /* loop size */
        whatever[(i * 30) + 48] = IN(where + 12);
        whatever[(i * 30) + 49] = IN(where + 13);
        where += 16;
    }

    /* size of pattern list */
    whatever[950] = nbr_pos;

    /* noisetracker byte */
    whatever[951] = 0x7f;

    /* bypass 2 bytes ... seems always the same as in $02 */
    /* & bypass 2 other bytes which meaning is beside me */
    where += 4;

    /* read pattern table */
    pat_max = 0x00;
    for (i = 0; i < nbr_pos; i++)
    {
        whatever[i + 952] = (uint8_t)(((IN(where + (i * 2)) * 256) + IN(where + (i * 2) + 1)) / 8);
        if (whatever[i + 952] > pat_max)
            pat_max = whatever[i + 952];
    }
    where += nbr_pos * 2;
    pat_max += 1;

    /* ptk's ID */
    whatever[1080] = 'M';
    whatever[1081] = '.';
    whatever[1082] = 'K';
    whatever[1083] = '.';
    pw_fwrite(out, whatever, 1084);

    /* read tracks addresses per pattern (pat_max is a byte: at most 255;
     * the original's table had 128 rows) */
    for (i = 0; i < pat_max; i++)
    {
        for (j = 0; j < 4; j++)
            track_addresses[i][j] = (IN(where + (i * 8) + j * 2) * 256) + IN(where + (i * 8) + j * 2 + 1);
    }
    track_data_start_address = (where + (pat_max * 8));

    /* the track data now ... */
    for (i = 0; i < pat_max; i++)
    {
        memset(whatever, 0, sizeof whatever);
        for (j = 0; j < 4; j++)
        {
            where = track_data_start_address + track_addresses[i][3 - j];
            for (k = 0; k < 64; k++)
            {
                c1 = IN(where);
                where += 1;
                if (c1 >= 0x80)
                {
                    k += ((0x100 - c1) - 1);
                    continue;
                }
                c2 = IN(where);
                where += 1;
                c3 = IN(where);
                where += 1;

                whatever[k * 16 + j * 4] = (uint8_t)((c1 << 4) & 0x10);
                c4 = (uint8_t)((c1 & 0xFE) / 2);
                /* poss[] has 37 entries; c4 can reach 63 on odd input */
                whatever[k * 16 + j * 4] |= (c4 < 37) ? poss[c4][0] : 0;
                whatever[k * 16 + j * 4 + 1] = (c4 < 37) ? poss[c4][1] : 0;
                if ((c2 & 0x0f) == 0x08)
                    c2 &= 0xf0;
                if ((c2 & 0x0f) == 0x07)
                {
                    c2 = (uint8_t)((c2 & 0xf0) + 0x0A);
                    if (c3 > 0x80)
                        c3 = (uint8_t)(0x100 - c3);
                    else
                        c3 = (uint8_t)((c3 << 4) & 0xf0);
                }
                if ((c2 & 0x0f) == 0x06)
                {
                    if (c3 > 0x80)
                        c3 = (uint8_t)(0x100 - c3);
                    else
                        c3 = (uint8_t)((c3 << 4) & 0xf0);
                }
                if ((c2 & 0x0f) == 0x05)
                {
                    if (c3 > 0x80)
                        c3 = (uint8_t)(0x100 - c3);
                    else
                        c3 = (uint8_t)((c3 << 4) & 0xf0);
                }
                if ((c2 & 0x0f) == 0x0E)
                    c3 = 0x01;
                if ((c2 & 0x0f) == 0x0B)
                {
                    c3 += 0x04;
                    c3 /= 2;
                }
                whatever[k * 16 + j * 4 + 2] = c2;
                whatever[k * 16 + j * 4 + 3] = c3;
                if ((c2 & 0x0f) == 0x0D)
                    k = 100; /* to leave the loop */
            }
            if (where > sample_data_address)
                sample_data_address = where;
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    if ((((sample_data_address - c->start) / 2) * 2) != sample_data_address)
        sample_data_address += 1;
    pw_fwrite_in(out, sample_data_address, whole_sample_size);

    pw_crap("  NoisePacker v3  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
