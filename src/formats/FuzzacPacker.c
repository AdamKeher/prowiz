/*
 * FuzzacPacker.c - Fuzzac Packer (upstream r/FuzzacPacker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_FUZZAC(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* test finetune */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if (IN(c->start + 72 + c->k * 68) > 0x0F)
            return PW_BAD;
    }

    /* test volumes */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if (IN(c->start + 73 + c->k * 68) > 0x40)
            return PW_BAD;
    }

    /* test sample sizes */
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->j = (IN(c->start + 66 + c->k * 68) * 256) + IN(c->start + 67 + c->k * 68);
        if (c->j > 0x8000)
            return PW_BAD;
        c->wss += (c->j * 2);
    }

    /* test size of pattern list */
    if (IN(c->start + 2114) == 0x00)
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_Fuzzac(pw_ctx *c)
{
    /* c->wss IS still the whole sample size */

    c->j = IN(c->start + 2114);
    c->k = IN(c->start + 2115);
    c->out_size = c->wss + (c->j * 16) + (c->k * 256) + 2118 + 4;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Fuzzac packer module", PWF_Fuzzac);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 2); /* -1 should do but call it "just to be sure" :) */
}

/*
 *   fuzzac.c   1997 (c) Asle / ReDoX
 *
 * Converts Fuzzac packed MODs back to PTK MODs
 * thanks to Gryzor and his ProWizard tool ! ... without it, this prog
 * would not exist !!!
 *
 * Note: A most worked-up prog ... took some time to finish this !.
 *      there's what lot of my other depacker are missing : the correct
 *      pattern order (most of the time the list is generated badly ..).
 *      Dont know why I did it for this depacker because I've but one
 *      exemple file ! :).
 */

#define ON  1
#define OFF 2

/* The pattern position count is a byte (up to 255) but the track tables
 * have 128 rows: the original read and wrote past them on such input.
 * Out-of-range reads give 0 and writes are dropped. */
static uint8_t tn_get(uint8_t t[128][16], int32_t row, int32_t col)
{
    return (row >= 0 && row < 128) ? t[row][col] : 0;
}

void pw_depack_Fuzzac(pw_ctx *c)
{
    uint8_t  c5;
    uint8_t  pat_pos;
    uint8_t  whatever[1024];
    uint8_t  nbr_tracks;
    uint8_t  track_numbers[128][16];
    uint8_t  track_numbers_real[128][4];
    uint8_t  track_datas[4][256];
    uint8_t  status = ON;
    int32_t  whole_sample_size = 0;
    int32_t  i, j, k, l;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(track_numbers, 0, sizeof track_numbers);
    memset(track_numbers_real, 0, sizeof track_numbers_real);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* bypass ID */
    /* bypass 2 unknown bytes */
    where += 6;

    /* write title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 20);

    for (i = 0; i < 31; i++)
    {
        /*sample name*/
        pw_fwrite_in(out, where, 22);

        whole_sample_size += (((IN(where + 60) * 256) + IN(where + 61)) * 2);
        pw_fwrite_in(out, where + 60, 2);
        pw_fwrite_in(out, where + 66, 2);
        pw_fwrite_in(out, where + 62, 2);

        whatever[0] = IN(where + 65);
        if ((IN(where + 64) == 0x00) && (IN(where + 65) == 0x00))
            whatever[0] = 0x01;
        pw_fwrite_in(out, where + 64, 1);
        pw_fwrite(out, whatever, 1);
        where += 68;
    }

    /* read & write size of pattern list */
    pat_pos = IN(where++);
    pw_fwrite(out, &pat_pos, 1);

    /* read the number of tracks */
    nbr_tracks = IN(where++);

    /* write noisetracker byte */
    whatever[0] = 0x7f;
    pw_fwrite(out, whatever, 1);

    /* place file pointer at track number list address */
    where = c->start + 2118;

    /* read tracks numbers */
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < pat_pos; j++)
        {
            if (j < 128)
            {
                track_numbers[j][i * 4] = IN(where);
                track_numbers[j][i * 4 + 1] = IN(where + 1);
                track_numbers[j][i * 4 + 2] = IN(where + 2);
                track_numbers[j][i * 4 + 3] = IN(where + 3);
            }
            where += 4;
        }
    }

    /* sort tracks numbers */
    c5 = 0x00;
    for (i = 0; i < pat_pos; i++)
    {
        if (i == 0)
        {
            whatever[0] = c5;
            c5 += 0x01;
            continue;
        }
        for (j = 0; j < i; j++)
        {
            status = ON;
            for (k = 0; k < 4; k++)
            {
                if (tn_get(track_numbers, j, k * 4) != tn_get(track_numbers, i, k * 4))
                {
                    status = OFF;
                    break;
                }
            }
            if (status == ON)
            {
                whatever[i] = whatever[j];
                break;
            }
        }
        if (status == OFF)
        {
            whatever[i] = c5;
            c5 += 0x01;
        }
        status = ON;
    }
    /* c5 is the Max pattern number */

    /* create a real list of tracks numbers for the really existing patterns
     * (whatever[129] is the counter, and is part of the list when it is
     * longer than 129, as in the original) */
    whatever[129] = 0x00;
    for (i = 0; i < pat_pos; i++)
    {
        if (i == 0)
        {
            if (whatever[129] < 128)
            {
                track_numbers_real[whatever[129]][0] = tn_get(track_numbers, i, 0);
                track_numbers_real[whatever[129]][1] = tn_get(track_numbers, i, 4);
                track_numbers_real[whatever[129]][2] = tn_get(track_numbers, i, 8);
                track_numbers_real[whatever[129]][3] = tn_get(track_numbers, i, 12);
            }
            whatever[129] += 0x01;
            continue;
        }
        for (j = 0; j < i; j++)
        {
            status = ON;
            if (whatever[i] == whatever[j])
            {
                status = OFF;
                break;
            }
        }
        if (status == OFF)
            continue;
        if (whatever[129] < 128)
        {
            track_numbers_real[whatever[129]][0] = tn_get(track_numbers, i, 0);
            track_numbers_real[whatever[129]][1] = tn_get(track_numbers, i, 4);
            track_numbers_real[whatever[129]][2] = tn_get(track_numbers, i, 8);
            track_numbers_real[whatever[129]][3] = tn_get(track_numbers, i, 12);
        }
        whatever[129] += 0x01;
        status = ON;
    }

    /* write pattern list */
    pw_fwrite(out, whatever, 128);

    /* write ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* pattern data */
    l = c->start + 2118 + (pat_pos * 16);
    for (i = 0; i < c5; i++)
    {
        uint8_t t0 = (i < 128) ? track_numbers_real[i][0] : 0;
        uint8_t t1 = (i < 128) ? track_numbers_real[i][1] : 0;
        uint8_t t2 = (i < 128) ? track_numbers_real[i][2] : 0;
        uint8_t t3 = (i < 128) ? track_numbers_real[i][3] : 0;

        memset(whatever, 0, sizeof whatever);
        memset(track_datas, 0, sizeof track_datas);
        where = l + (t0 * 256);
        for (j = 0; j < 256; j++)
            track_datas[0][j] = IN(where + j);
        where = l + (t1 * 256);
        for (j = 0; j < 256; j++)
            track_datas[1][j] = IN(where + j);
        where = l + (t2 * 256);
        for (j = 0; j < 256; j++)
            track_datas[2][j] = IN(where + j);
        where = l + (t3 * 256);
        for (j = 0; j < 256; j++)
            track_datas[3][j] = IN(where + j);

        for (j = 0; j < 64; j++)
        {
            whatever[j * 16] = track_datas[0][j * 4];
            whatever[j * 16 + 1] = track_datas[0][j * 4 + 1];
            whatever[j * 16 + 2] = track_datas[0][j * 4 + 2];
            whatever[j * 16 + 3] = track_datas[0][j * 4 + 3];
            whatever[j * 16 + 4] = track_datas[1][j * 4];
            whatever[j * 16 + 5] = track_datas[1][j * 4 + 1];
            whatever[j * 16 + 6] = track_datas[1][j * 4 + 2];
            whatever[j * 16 + 7] = track_datas[1][j * 4 + 3];
            whatever[j * 16 + 8] = track_datas[2][j * 4];
            whatever[j * 16 + 9] = track_datas[2][j * 4 + 1];
            whatever[j * 16 + 10] = track_datas[2][j * 4 + 2];
            whatever[j * 16 + 11] = track_datas[2][j * 4 + 3];
            whatever[j * 16 + 12] = track_datas[3][j * 4];
            whatever[j * 16 + 13] = track_datas[3][j * 4 + 1];
            whatever[j * 16 + 14] = track_datas[3][j * 4 + 2];
            whatever[j * 16 + 15] = track_datas[3][j * 4 + 3];
        }

        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    where = l + 4 + nbr_tracks * 256;
    /* l : 2118 + NumberOfPattern*16+PW_Start_Address */
    /* 4 : to bypass the "SEnd" unidentified ID */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap ... */
    pw_crap("  FUZZAC Packer   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
