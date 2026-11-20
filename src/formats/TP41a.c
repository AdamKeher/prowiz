/*
 * TP41a.c - The Player 4.1A (upstream r/TP41a.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* Track_Data[512][256] of the converter, kept as one flat block: a column
 * past 255 runs into the next row as it did in the original. */
#define TD_ROWS 512
#define TD_COLS 256

/* writes outside the block went past the original local array: dropped */
static void td_set(uint8_t *td, int32_t row, int32_t col, uint8_t v)
{
    int64_t off = (int64_t)row * TD_COLS + col;
    if (off >= 0 && off < (int64_t)TD_ROWS * TD_COLS)
        td[off] = v;
}

static uint8_t td_get(const uint8_t *td, int32_t row, int32_t col)
{
    int64_t off = (int64_t)row * TD_COLS + col;
    return (off >= 0 && off < (int64_t)TD_ROWS * TD_COLS) ? td[off] : 0;
}

/* poss[] has 37 entries; a note up to 0x7f indexes up to 63 (reads 0) */
static uint8_t poss_at(uint8_t poss[37][2], int32_t idx, int col)
{
    return (idx >= 0 && idx < 37) ? poss[idx][col] : 0;
}

int16_t pw_test_P41A(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* number of pattern (real) */
    c->j = IN(c->start + 4);
    if (c->j > 0x7f)
        return PW_BAD;

    /* number of sample */
    c->k = IN(c->start + 6);
    if ((c->k > 0x1F) || (c->k == 0))
        return PW_BAD;

    /* test volumes */
    for (c->l = 0; c->l < c->k; c->l++)
    {
        if (IN(c->start + 33 + c->l * 16) > 0x40)
            return PW_BAD;
    }

    /* test sample sizes */
    c->wss = 0;
    for (c->l = 0; c->l < c->k; c->l++)
    {
        /* size */
        c->o = (IN(c->start + 24 + c->l * 16) * 256) + IN(c->start + 25 + c->l * 16);
        /* loop size */
        c->n = (IN(c->start + 30 + c->l * 16) * 256) + IN(c->start + 31 + c->l * 16);
        c->o *= 2;
        c->n *= 2;

        if ((c->o > 0xFFFF) || (c->n > 0xFFFF))
            return PW_BAD;

        if (c->n > (c->o + 2))
            return PW_BAD;
        c->wss += c->o;
    }
    if (c->wss <= 4)
        return PW_BAD;

    /* c->wss is the size of the sample data .. WRONG !! */
    /* c->k is the number of samples */
    return PW_GOOD;
}

void pw_rip_P41A(pw_ctx *c)
{
    int32_t seed = 0;

    /* potential fake address if rip from memory - thus, start address*/
    c->l = ((IN(c->start + 16) * 256 * 256 * 256) +
            (IN(c->start + 17) * 256 * 256) +
            (IN(c->start + 18) * 256) +
            IN(c->start + 19));
    c->m = ((IN(c->start + 12) * 256 * 256 * 256) +
            (IN(c->start + 13) * 256 * 256) +
            (IN(c->start + 14) * 256) +
            IN(c->start + 15));
    /* check here for memory addresses, which are screwed up */
    c->j = (IN(c->start + 6) * 16) + 4 + 12;
    if (c->j != c->m)
    {
        /* it's memory addresses then */
        seed = (int32_t)(c->m - c->j);
        c->l -= seed;
    }

    /* get whole sample size */
    /* c->k is the number of samples */
    c->o = 0;
    for (c->j = 0; c->j < c->k; c->j++)
    {
        c->m = ((IN(c->start + 20 + c->j * 16) * 256 * 256 * 256) +
                (IN(c->start + 21 + c->j * 16) * 256 * 256) +
                (IN(c->start + 22 + c->j * 16) * 256) +
                IN(c->start + 23 + c->j * 16)) - seed;
        if (c->m > c->o)
        {
            c->o = c->m;
            c->n = ((IN(c->start + 24 + c->j * 16) * 256) + IN(c->start + 25 + c->j * 16));
        }
    }

    c->out_size = c->l + c->o + (c->n * 2) + 4;

    c->convert = PW_GOOD;
    pw_save_rip(c, "The Player 4.1A module", PWF_P41A);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 2); /* 1 should do but call it "just to be sure" :) */
}

/*
 *   The_Player_4.1a.c   1997 (c) Asle / ReDoX
 *
 * The Player 4.1a to Protracker.
 *
 * Note: It's a REAL mess !. It's VERY badly coded, I know. Just dont forget
 *      it was mainly done to test the description I made of P41a format. I
 *      certainly wont dare to beat Gryzor on the ground :). His Prowiz IS
 *      the converter to use !!!.
 *
 * Update: 28/11/99
 *     - removed fopen() and all attached functions.
 *     - overall speed and size optimizings.
 * Update : 26 nov 2003
 *     - used htonl() so that use of addy is now portable on 68k archs
 */
void pw_depack_P41A(pw_ctx *c)
{
    uint8_t  c1, c2, c3, c4, c5;
    uint8_t  whatever[1024];
    uint8_t  pat_pos = 0x00;
    uint8_t  nbr_sample = 0x00;
    uint8_t  poss[37][2];
    uint8_t  sample, note, Note[2];
    uint8_t *track_data;
    int16_t  track_addresses[128][4];
    int32_t  track_data_address = 0;
    int32_t  track_table_address = 0;
    int32_t  sample_data_address = 0;
    int32_t  sample_address[31];
    int32_t  sample_size[31];
    int32_t  i = 0, j, k, l, a, cc;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(track_addresses, 0, sizeof track_addresses);
    memset(sample_address, 0, sizeof sample_address);
    memset(sample_size, 0, sizeof sample_size);

    pw_fill_ptk_table(poss);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    track_data = (uint8_t *)calloc((size_t)TD_ROWS * TD_COLS, 1);
    if (track_data == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        pw_fdiscard(out);
        return;
    }

    /* read check ID */
    where += 4;

    /* bypass Real number of pattern */
    where += 1;

    /* read number of pattern in pattern list */
    pat_pos = IN(where++);

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

    /* check here for memory addresses, which are screwed up */
    i = (nbr_sample * 16) + 4 + 12;
    if (i != track_table_address)
    {
        /* it's memory addresses then */
        int32_t seed = (track_table_address - i);
        track_data_address -= seed;
        track_table_address -= seed;
        sample_data_address -= seed;
    }

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
        sample_address[i] = j - sample_data_address; /* just a try with -*/

        /* write sample name */
        pw_fwrite(out, whatever, 22);

        /* read sample size */
        sample_size[i] = ((IN(where + 4) * 256) + IN(where + 5)) * 2;

        /* loop start */
        k = (IN(where + 6) * 256 * 256 * 256) +
            (IN(where + 7) * 256 * 256) +
            (IN(where + 8) * 256) +
            IN(where + 9) - sample_data_address; /* just a try with -*/

        /* writing now */
        pw_fwrite_in(out, where + 4, 2);
        c1 = (uint8_t)(((IN(where + 14) * 256) + IN(where + 15)) / 74);
        pw_fwrite(out, &c1, 1);
        pw_fwrite_in(out, where + 13, 1);
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

    /* reading all the track addresses (pat_pos can reach 255: rows past
     * 127 were written past the local array; dropped, read back as 0) */
    for (i = 0; i < pat_pos; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (i < 128)
                track_addresses[i][j] = (int16_t)((IN(where) * 256) +
                                                  IN(where + 1) +
                                                  track_data_address + 4);
            where += 2;
        }
    }

    /* rewrite the track data */
    for (i = 0; i < pat_pos; i++)
    {
        for (j = 0; j < 4; j++)
        {
            where = c->start + ((i < 128) ? track_addresses[i][j] : 0);
            for (k = 0; k < 64; k++)
            {
                c1 = IN(where++);
                c2 = IN(where++);
                c3 = IN(where++);
                c4 = IN(where++);

                if (c1 != 0x80)
                {
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
                    td_set(track_data, i * 4 + j, k * 4, (uint8_t)((sample & 0xf0) | (Note[0] & 0x0f)));
                    td_set(track_data, i * 4 + j, k * 4 + 1, Note[1]);
                    td_set(track_data, i * 4 + j, k * 4 + 2, c2);
                    td_set(track_data, i * 4 + j, k * 4 + 3, c3);

                    if ((c4 > 0x00) && (c4 < 0x80))
                        k += c4;
                    if (c4 > 0x7f)
                    {
                        k += 1;
                        for (l = 256; l > c4; l--)
                        {
                            td_set(track_data, i * 4 + j, k * 4, (uint8_t)((sample & 0xf0) | (Note[0] & 0x0f)));
                            td_set(track_data, i * 4 + j, k * 4 + 1, Note[1]);
                            td_set(track_data, i * 4 + j, k * 4 + 2, c2);
                            td_set(track_data, i * 4 + j, k * 4 + 3, c3);
                            k += 1;
                        }
                        k -= 1;
                    }
                }
                else
                {
                    a = where;

                    c5 = c2;
                    where = c->start + (c3 * 256) + c4 + track_data_address + 4;
                    for (cc = 0; cc <= c5; cc++)
                    {
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
                        td_set(track_data, i * 4 + j, k * 4, (uint8_t)((sample & 0xf0) | (Note[0] & 0x0f)));
                        td_set(track_data, i * 4 + j, k * 4 + 1, Note[1]);
                        td_set(track_data, i * 4 + j, k * 4 + 2, c2);
                        td_set(track_data, i * 4 + j, k * 4 + 3, c3);

                        if ((c4 > 0x00) && (c4 < 0x80))
                            k += c4;
                        if (c4 > 0x7f)
                        {
                            k += 1;
                            for (l = 256; l > c4; l--)
                            {
                                td_set(track_data, i * 4 + j, k * 4, (uint8_t)((sample & 0xf0) | (Note[0] & 0x0f)));
                                td_set(track_data, i * 4 + j, k * 4 + 1, Note[1]);
                                td_set(track_data, i * 4 + j, k * 4 + 2, c2);
                                td_set(track_data, i * 4 + j, k * 4 + 3, c3);
                                k += 1;
                            }
                            k -= 1;
                        }
                        k += 1;
                    }

                    k -= 1;
                    where = a;
                }
            }
        }
    }

    /* write pattern data */
    for (i = 0; i < pat_pos; i++)
    {
        memset(whatever, 0, sizeof whatever);
        for (j = 0; j < 64; j++)
        {
            for (k = 0; k < 4; k++)
            {
                whatever[j * 16 + k * 4]     = td_get(track_data, k + i * 4, j * 4);
                whatever[j * 16 + k * 4 + 1] = td_get(track_data, k + i * 4, j * 4 + 1);
                whatever[j * 16 + k * 4 + 2] = td_get(track_data, k + i * 4, j * 4 + 2);
                whatever[j * 16 + k * 4 + 3] = td_get(track_data, k + i * 4, j * 4 + 3);
            }
        }
        pw_fwrite(out, whatever, 1024);
    }
    free(track_data);

    /* read and write sample data */
    for (i = 0; i < nbr_sample; i++)
    {
        where = c->start + sample_address[i] + sample_data_address;
        pw_fwrite_in(out, where, sample_size[i]);
    }

    pw_crap(" The Player 4.1A  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
