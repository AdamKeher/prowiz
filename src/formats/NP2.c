/*
 * NP2.c - NoisePacker v2 (upstream r/NP2.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_Noisepacker2(pw_ctx *c)
{
    if (c->i < 15)
        return PW_BAD;
    c->start = (int32_t)(c->i - 15);

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
        if (IN(c->start + 15 + c->k * 16) > 0x40)
            return PW_BAD;
    }

    /* test sample sizes */
    c->wss = 0;
    for (c->k = 0; c->k < c->l; c->k++)
    {
        c->o = (IN(c->start + c->k * 16 + 12) * 256) + IN(c->start + c->k * 16 + 13);
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
    if (c->l + c->start > (uint32_t)c->in_size)
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
    if (c->l + c->start > (uint32_t)c->in_size)
        return PW_BAD;

    /* test track data size */
    c->k = (IN(c->start + 6) * 256) + IN(c->start + 7);
    if ((c->k < 192) || (((c->k / 192) * 192) != c->k))
        return PW_BAD;

    /* test notes */
    c->j = ((IN(c->start) << 4) & 0xf0) | ((IN(c->start + 1) >> 4) & 0x0f);
    for (c->m = 0; c->m < c->k; c->m += 3)
    {
        if (c->start + c->l + c->m > (uint32_t)c->in_size)
            return PW_BAD;
        if (IN(c->start + c->l + c->m) > 0x49)
            return PW_BAD;
        if ((uint32_t)(((IN(c->start + c->l + c->m) << 4) & 0x10) |
                       ((IN(c->start + c->l + c->m + 1) >> 4) & 0x0f)) > c->j)
            return PW_BAD;
        c->n = (IN(c->start + c->l + c->m + 1) & 0x0F);
        if ((c->n == 0) && (IN(c->start + c->l + c->m + 2) != 0x00))
            return PW_BAD;
    }

    /* c->wss is the size of the sample data */
    /* c->l is the size of the header 'til the track datas */
    /* c->k is the size of the track datas */
    return PW_GOOD;
}

void pw_rip_Noisepacker2(pw_ctx *c)
{
    c->out_size = c->k + c->wss + c->l;

    c->convert = PW_GOOD;
    pw_save_rip(c, "NoisePacker v2 module", PWF_Noisepacker2);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 16); /* 15 should do but call it "just to be sure" :) */
}

/*
 *   NoisePacker_v2.c   1997 (c) Asle / ReDoX
 * Converts NoisePacked MODs back to ptk
 */
void pw_depack_Noisepacker2(pw_ctx *c)
{
    uint8_t  whatever[1024];
    uint8_t  c1, c2, c3, c4;
    uint8_t  nbr_pos;
    uint8_t  nbr_smp;
    uint8_t  poss[37][2];
    uint8_t  pat_max;
    int32_t  where = c->start;
    int32_t  max_add = 0;
    int32_t  whole_sample_size = 0;
    int32_t  track_addresses[256][4];
    int32_t  i, j, k;
    int32_t  track_data_start_address;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    pw_fill_ptk_table(poss);
    memset(track_addresses, 0, sizeof track_addresses);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* read number of sample */
    nbr_smp = (uint8_t)(((IN(where) << 4) & 0xf0) | ((IN(where + 1) >> 4) & 0x0f));

    /* write title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 20);

    /* read size of pattern list */
    nbr_pos = (uint8_t)(IN(where + 3) / 2);

    /* bytes 4-5: unknown; bytes 6-7: track data size (not needed here) */

    /* read sample descriptions */
    where += 8;
    for (i = 0; i < nbr_smp; i++)
    {
        /* sample name */
        pw_fwrite(out, whatever, 22);
        /* size,fine,vol */
        pw_fwrite_in(out, where + 4, 4);
        whole_sample_size += (((IN(where + 4) * 256) + IN(where + 5)) * 2);
        /* write loop start */
        pw_fwrite_in(out, where + 14, 2);
        /* write loop size */
        pw_fwrite_in(out, where + 12, 2);
        where += 16;
    }

    /* fill up to 31 samples */
    whatever[29] = 0x01;
    while (i != 31)
    {
        pw_fwrite(out, whatever, 30);
        i += 1;
    }

    /* write size of pattern list */
    pw_fwrite(out, &nbr_pos, 1);

    /* write noisetracker byte */
    whatever[256] = 0x7f;
    pw_fwrite(out, &whatever[256], 1);

    /* bypass 2 bytes ... seems always the same as in $02 */
    /* & bypass 2 other bytes which meaning is beside me */
    where += 4;

    /* read pattern table */
    pat_max = 0x00;
    memset(whatever, 0, sizeof whatever);
    for (i = 0; i < nbr_pos; i++)
    {
        whatever[i] = (uint8_t)(((IN(where + (i * 2)) * 256) + IN(where + (i * 2) + 1)) / 8);
        if (whatever[i] > pat_max)
            pat_max = whatever[i];
    }
    pat_max += 1;
    where += nbr_pos * 2;

    /* write pattern table */
    pw_fwrite(out, whatever, 128);

    /* write ptk's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* read tracks addresses per pattern (pat_max is a byte: at most 255) */
    for (i = 0; i < pat_max; i++)
    {
        for (j = 0; j < 4; j++)
        {
            track_addresses[i][j] = (IN(where + (i * 8) + j * 2) * 256) + IN(where + (i * 8) + j * 2 + 1);
            if (track_addresses[i][j] > max_add)
                max_add = track_addresses[i][j];
        }
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
                c2 = IN(where);
                where += 1;
                c3 = IN(where);
                where += 1;
                whatever[k * 16 + j * 4] = (uint8_t)((c1 << 4) & 0x10);
                c4 = (uint8_t)((c1 & 0xFE) / 2);
                /* poss[] has 37 entries; c4 can reach 127 on odd input */
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
                    c3 -= 0x01;
                if ((c2 & 0x0f) == 0x0B)
                {
                    c3 += 0x04;
                    c3 /= 2;
                }
                whatever[k * 16 + j * 4 + 2] = c2;
                whatever[k * 16 + j * 4 + 3] = c3;
            }
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    where = max_add + 192 + track_data_start_address;
    pw_fwrite_in(out, where, whole_sample_size);

    pw_crap("  NoisePacker v2  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
