/*
 * TP22a.c - The Player 2.2A (upstream r/TP22a.c).
 * The test is pw_test_P40A() (TP40.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* Pattern_Data[128][1024] of the converter, kept as one flat block */
#define PD_ROWS 128
#define PD_COLS 1024

/* writes outside the block went past the original local array: dropped */
static void pd_set(uint8_t *pd, int32_t row, int32_t col, uint8_t v)
{
    int64_t off = (int64_t)row * PD_COLS + col;
    if (off >= 0 && off < (int64_t)PD_ROWS * PD_COLS)
        pd[off] = v;
}

/* poss[] has 37 entries; a note up to 0x7f indexes up to 63 (reads 0) */
static uint8_t poss_at(uint8_t poss[37][2], int32_t idx, int col)
{
    return (idx >= 0 && idx < 37) ? poss[idx][col] : 0;
}

void pw_rip_P22A(pw_ctx *c)
{
    /* c->k is the number of sample */

    c->l = ((IN(c->start + 16) * 256 * 256 * 256) +
            (IN(c->start + 17) * 256 * 256) +
            (IN(c->start + 18) * 256) +
            IN(c->start + 19));

    /* get whole sample size */
    /* starting from the highest addy and adding the sample size */
    c->o = 0;
    for (c->j = 0; c->j < c->k; c->j++)
    {
        c->m = ((IN(c->start + 20 + c->j * 16) * 256 * 256 * 256) +
                (IN(c->start + 21 + c->j * 16) * 256 * 256) +
                (IN(c->start + 22 + c->j * 16) * 256) +
                IN(c->start + 23 + c->j * 16));
        if (c->m > c->o)
        {
            c->o = c->m;
            c->n = ((IN(c->start + 24 + c->j * 16) * 256) + IN(c->start + 25 + c->j * 16));
        }
    }

    c->out_size = c->l + c->o + (c->n * 2) + 4;

    c->convert = PW_GOOD;
    pw_save_rip(c, "The Player 2.2A module", PWF_ThePlayer22a);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 2); /* 1 should do but call it "just to be sure" :) */
}

/*
 *   The_Player_2.2.c   2003 (c) Asle / ReDoX
 *
 * The Player 2.2a to Protracker.
 *
 * Update : 26 nov 2003
 *   - used htonl() so that use of addy is now portable on 68k archs
 */
void pw_depack_P22(pw_ctx *c)
{
    uint8_t  c1, c2, c3, c4;
    uint8_t  whatever[1024];
    uint8_t  pat_pos = 0x00;
    uint8_t  nbr_sample = 0x00;
    uint8_t  poss[37][2];
    uint8_t  sample, note, Note[2];
    uint8_t *pattern_data;
    int16_t  pattern_addresses[128];
    int32_t  track_data_address = 0;
    int32_t  track_table_address = 0;
    int32_t  sample_data_address = 0;
    int32_t  sample_address[31];
    int32_t  sample_size[31];
    int32_t  i = 0, j, k;
    int32_t  voice[4];
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(pattern_addresses, 0, sizeof pattern_addresses);
    memset(sample_address, 0, sizeof sample_address);
    memset(sample_size, 0, sizeof sample_size);

    pw_fill_ptk_table(poss);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    pattern_data = (uint8_t *)calloc((size_t)PD_ROWS * PD_COLS, 1);
    if (pattern_data == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        pw_fdiscard(out);
        return;
    }

    /* read check ID */
    where += 4;

    /* bypass Real number of pattern */
    where += 1;

    /* read number of pattern in pattern list (0 or 1 gives 255) */
    pat_pos = (uint8_t)((IN(where++) / 2) - 1);

    /* read number of samples */
    nbr_sample = IN(where++);

    /* bypass empty byte */
    where += 1;

    /* read track data address */
    track_data_address = (IN(where) * 256 * 256 * 256) +
                         (IN(where + 1) * 256 * 256) +
                         (IN(where + 2) * 256) +
                         IN(where + 3);
    where += 4;

    /* read track table address */
    track_table_address = (IN(where) * 256 * 256 * 256) +
                          (IN(where + 1) * 256 * 256) +
                          (IN(where + 2) * 256) +
                          IN(where + 3);
    where += 4;

    /* read sample data address */
    sample_data_address = (IN(where) * 256 * 256 * 256) +
                          (IN(where + 1) * 256 * 256) +
                          (IN(where + 2) * 256) +
                          IN(where + 3);
    where += 4;

    /* write title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 20);

    /* sample headers stuff (nbr_sample <= 31: checked by the test) */
    for (i = 0; i < nbr_sample; i++)
    {
        /* read sample data address */
        j = (IN(where) * 256 * 256 * 256) +
            (IN(where + 1) * 256 * 256) +
            (IN(where + 2) * 256) +
            IN(where + 3);
        sample_address[i] = j;

        /* write sample name */
        pw_fwrite(out, whatever, 22);

        /* read sample size */
        sample_size[i] = ((IN(where + 4) * 256) + IN(where + 5)) * 2;

        /* loop start */
        k = (IN(where + 6) * 256 * 256 * 256) +
            (IN(where + 7) * 256 * 256) +
            (IN(where + 8) * 256) +
            IN(where + 9);

        /* writing now */
        pw_fwrite_in(out, where + 4, 2);
        c1 = (uint8_t)(((IN(where + 12) * 256) + IN(where + 13)) / 74);
        pw_fwrite(out, &c1, 1);
        pw_fwrite_in(out, where + 15, 1);
        k -= j;
        k /= 2;
        /* low 16 bits of k, big-endian (bytes 2 and 3 of htonl(k)) */
        pw_fput16be(out, (uint32_t)k);
        pw_fwrite_in(out, where + 10, 2);

        where += 16;
    }

    /* go up to 31 samples */
    whatever[29] = 0x01;
    while (i != 31)
    {
        pw_fwrite(out, whatever, 30);
        i += 1;
    }

    /* write size of pattern list */
    pw_fwrite(out, &pat_pos, 1);

    /* write noisetracker byte */
    c1 = 0x7f;
    pw_fwrite(out, &c1, 1);

    /* place file pointer at the pattern list address ... should be */
    /* useless, but then ... */
    where = c->start + track_table_address + 4;

    /* create and write pattern list .. no optimization ! */
    for (c1 = 0x00; c1 < pat_pos; c1++)
        pw_fwrite(out, &c1, 1);
    c2 = 0x00;
    while (c1 < 128)
    {
        pw_fwrite(out, &c2, 1);
        c1 += 0x01;
    }

    /* write ptk's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* reading all the track addresses .. which seem to be pattern addys ...
     * (pat_pos can be 255: entries past 127 were written past the local
     * array; dropped, read back as 0) */
    for (i = 0; i < pat_pos; i++)
    {
        if (i < 128)
            pattern_addresses[i] = (int16_t)((IN(where) * 256) + IN(where + 1) + track_data_address + 4);
        where += 2;
    }

    /* rewrite the track data */
    for (i = 0; i < pat_pos; i++)
    {
        where = c->start + ((i < 128) ? pattern_addresses[i] : 0);
        voice[0] = voice[1] = voice[2] = voice[3] = 0;
        for (k = 0; k < 64; k++)
        {
            for (j = 0; j < 4; j++)
            {
                if (voice[j] > k)
                    continue;

                c1 = IN(where++);
                c2 = IN(where++);
                c3 = IN(where++);
                c4 = IN(where++);

                sample = (uint8_t)(((c1 << 4) & 0x10) | ((c2 >> 4) & 0x0f));
                note = c1 & 0x7f;
                Note[0] = poss_at(poss, note / 2, 0);
                Note[1] = poss_at(poss, note / 2, 1);
                switch (c2 & 0x0f)
                {
                case 0x08:
                    c2 -= 0x08;
                    break;
                case 0x05:
                case 0x06:
                case 0x0A:
                    c3 = (uint8_t)((c3 > 0x7f) ? ((0x100 - c3) << 4) : c3);
                    break;
                default:
                    break;
                }
                pd_set(pattern_data, i, voice[j] * 16 + j * 4, (uint8_t)((sample & 0xf0) | (Note[0] & 0x0f)));
                pd_set(pattern_data, i, voice[j] * 16 + j * 4 + 1, Note[1]);
                pd_set(pattern_data, i, voice[j] * 16 + j * 4 + 2, c2);
                pd_set(pattern_data, i, voice[j] * 16 + j * 4 + 3, c3);

                if ((c4 > 0x00) && (c4 < 0x80))
                    voice[j] += c4;
                voice[j] += 1;
            }
        }
    }

    /* write pattern data (rows past 127 were read past the local array) */
    memset(whatever, 0, sizeof whatever);
    for (i = 0; i < pat_pos; i++)
    {
        if (i < PD_ROWS)
            pw_fwrite(out, pattern_data + (size_t)i * PD_COLS, 1024);
        else
            pw_fwrite(out, whatever, 1024);
    }
    free(pattern_data);

    /* read and write sample data */
    for (i = 0; i < nbr_sample; i++)
    {
        where = c->start + sample_address[i] + sample_data_address;
        pw_fwrite_in(out, where, sample_size[i]);
    }

    pw_crap(" The Player 2.2A  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
