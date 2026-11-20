/*
 * ChipTracker.c - KRIS tracker / Chip Tracker ("KRIS" at offset 952)
 *                 (upstream r/ChipTracker.c, bugs pointed out by Thomas Neumann).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_KRIS(pw_ctx *c)
{
    /* test 1 (the size check uses the start address of the previous test) */
    if ((c->i < 952) || ((c->start + 977) > c->in_size))
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 952);
    for (c->j = 0; c->j < 31; c->j++)
    {
        /* volume > 64 ? */
        if (IN(c->start + 47 + c->j * 30) > 0x40)
            return PW_BAD;
        /* finetune > 15 ? */
        if (IN(c->start + 46 + c->j * 30) > 0x0f)
            return PW_BAD;
    }

    return PW_GOOD;
}

/*
 * Update: 20/12/2000
 *  - debug .. correct size calculated now.
 * Update: 01/09/2010
 *  - debug .. pattern data size scan only on patternlist size
 */
void pw_rip_KRIS(pw_ctx *c)
{
    /* nothing was calculated in the test part */

    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
        c->wss += (((IN(c->start + 44 + c->k * 30) * 256) + IN(c->start + 45 + c->k * 30)) * 2);
    c->l = 0;

    /* 20100901 : 128 replaced by patternlist size */
    for (c->k = 0; c->k < (uint32_t)(IN(c->start + 956) * 4); c->k++)
    {
        if ((uint32_t)((IN(c->start + 958 + c->k * 2) * 256) + IN(c->start + 959 + c->k * 2)) > c->l)
            c->l = (IN(c->start + 958 + c->k * 2) * 256) + IN(c->start + 959 + c->k * 2);
    }
    c->k = 1984 + c->l + 256;
    c->out_size = c->k + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "KRIS Tracker module", PWF_KRIS_tracker);

    if (c->save_status == PW_GOOD)
        c->i += 954; /* 953 should do but call it "just to be sure" :) */
}

/*
 *   Kris_packer.c   1997-2000 (c) Asle / ReDoX
 *
 * Kris Tracker to Protracker.
 *
 * Update: 28/11/1999
 *     - removed fopen()
 *     - Overall Speed and Size optimizings.
 * Update: 03/04/2000
 *     - no more sample beginning with $01 :) (Thomas Neumann again ... thx)
 * Update: 20/12/2000
 *     - major debugging around the patterntable ...
 * Update: 02/10/2005
 *     - testing fopen()
 */

/* The pattern list holds up to 255 entries but the tables below have 128
 * rows: rows past the end read as 0 and writes to them are dropped. */
static int16_t kris_tat(int16_t tat[128][4], int32_t row, int32_t col)
{
    return (row >= 0 && row < 128) ? tat[row][col] : 0;
}

/* A track address is an int16_t, so address/256 runs from -128 to 127. */
static uint8_t kris_track(const uint8_t *track_data, int32_t track, int32_t pos)
{
    return (track >= 0 && track < 512) ? track_data[track * 256 + pos] : 0;
}

void pw_depack_KRIS(pw_ctx *c)
{
    uint8_t  whatever[1024];
    uint8_t  c1 = 0x00, c2 = 0x00;
    uint8_t  poss[37][2];
    uint8_t  max = 0x00;
    uint8_t  note, smp, fx, fx_val;
    uint8_t *track_data; /* [512][256] */
    uint8_t  pattern_table_size = 0x00;
    uint8_t  pattern_table[128];
    uint8_t  pt;
    int16_t  track_address_table[128][4];
    int32_t  i = 0, j = 0, k = 0, n;
    int32_t  whole_sample_size = 0;
    int32_t  where = c->start;
    int16_t  max_track_address = 0;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    pw_fill_ptk_table(poss);

    track_data = (uint8_t *)calloc(512 * 256, 1);
    if (track_data == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        pw_fdiscard(out);
        return;
    }
    memset(track_address_table, 0, sizeof track_address_table);
    memset(pattern_table, 0, sizeof pattern_table);

    /* title */
    pw_fwrite_in(out, where, 20);
    where += 22; /* 20 + 2 */

    /* 31 samples */
    memset(whatever, 0, sizeof whatever);
    for (i = 0; i < 31; i++)
    {
        /* sample name,siz,fine,vol */
        if (IN(where) == 0x01)
        {
            pw_fwrite(out, whatever, 20);
            pw_fwrite_in(out, where + 20, 6);
        }
        else
            pw_fwrite_in(out, where, 26);

        /* size */
        whole_sample_size += (((IN(where + 22) * 256) + IN(where + 23)) * 2);

        /* loop start */
        c1 = (uint8_t)(IN(where + 26) / 2);
        c2 = (uint8_t)(IN(where + 27) / 2);
        if ((c1 * 2) != IN(where + 26))
            c2 += 1;
        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c2, 1);

        /* loop size */
        pw_fwrite_in(out, where + 28, 2);

        where += 30;
    }

    /* bypass ID "KRIS" */
    where += 4;

    /* number of pattern in pattern list */
    /* Noisetracker restart byte */
    pattern_table_size = IN(where);
    pw_fwrite_in(out, where, 2);
    where += 2;

    /* pattern table (read,count and write) */
    c1 = 0x00;
    k = 0;
    for (i = 0; i < pattern_table_size; i++, k++)
    {
        for (j = 0; j < 4; j++)
        {
            if (k < 128)
                track_address_table[k][j] = (int16_t)((IN(where) * 256) + IN(where + 1));
            if (kris_tat(track_address_table, k, j) > max_track_address)
                max_track_address = kris_tat(track_address_table, k, j);
            where += 2;
        }
        for (j = 0; j < k; j++)
        {
            if ((kris_tat(track_address_table, j, 0) == kris_tat(track_address_table, k, 0)) &&
                (kris_tat(track_address_table, j, 1) == kris_tat(track_address_table, k, 1)) &&
                (kris_tat(track_address_table, j, 2) == kris_tat(track_address_table, k, 2)) &&
                (kris_tat(track_address_table, j, 3) == kris_tat(track_address_table, k, 3)))
            {
                if (i < 128)
                    pattern_table[i] = (uint8_t)j;
                k -= 1;
                j = 9999; /* hum ... sure now ! */
                break;
            }
        }
        if (k == j)
        {
            if (i < 128)
                pattern_table[i] = c1;
            c1 += 0x01;
        }
        pt = (i < 128) ? pattern_table[i] : 0;
        pw_fwrite(out, &pt, 1);
    }

    max = c1;
    c1 = 0x00;
    while (i++ < 128)
    {
        pw_fwrite(out, &c1, 1);
        where += 8;
    }

    /* ptk ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* bypass two unknown bytes */
    where += 2;

    /* Track data ... (max_track_address/256 is at most 127) */
    for (i = 0; i <= (max_track_address / 256); i += 1)
    {
        memset(whatever, 0, sizeof whatever);
        for (j = 0; j < 256; j++)
            whatever[j] = IN(where + j);
        where += 256;

        for (j = 0; j < 64; j++)
        {
            note = whatever[j * 4];
            smp = whatever[j * 4 + 1];
            fx = whatever[j * 4 + 2] & 0x0f;
            fx_val = whatever[j * 4 + 3];

            track_data[i * 256 + j * 4] = (smp & 0xf0);

            if (note != 0xa8)
            {
                /* poss[] has 37 entries; the index runs from -35 to 92 */
                n = (note / 2) - 35;
                track_data[i * 256 + j * 4] |= (n >= 0 && n < 37) ? poss[n][0] : 0;
                track_data[i * 256 + j * 4 + 1] |= (n >= 0 && n < 37) ? poss[n][1] : 0;
            }
            track_data[i * 256 + j * 4 + 2] = (uint8_t)((smp << 4) & 0xf0);
            track_data[i * 256 + j * 4 + 2] |= (fx & 0x0f);
            track_data[i * 256 + j * 4 + 3] = fx_val;
        }
    }

    for (i = 0; i < max; i++)
    {
        memset(whatever, 0, sizeof whatever);
        for (j = 0; j < 64; j++)
        {
            for (k = 0; k < 4; k++)
            {
                n = kris_tat(track_address_table, i, k) / 256;
                whatever[j * 16 + k * 4] = kris_track(track_data, n, j * 4);
                whatever[j * 16 + k * 4 + 1] = kris_track(track_data, n, j * 4 + 1);
                whatever[j * 16 + k * 4 + 2] = kris_track(track_data, n, j * 4 + 2);
                whatever[j * 16 + k * 4 + 3] = kris_track(track_data, n, j * 4 + 3);
            }
        }
        pw_fwrite(out, whatever, 1024);
    }
    free(track_data);

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    pw_crap("   Kris tracker   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
