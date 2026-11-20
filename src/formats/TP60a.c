/*
 * TP60a.c - The Player 6.0A (upstream r/TP60a.c).
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

/* poss[] has 37 entries; larger notes read 0 */
static uint8_t poss_at(uint8_t poss[37][2], int32_t idx, int col)
{
    return (idx >= 0 && idx < 37) ? poss[idx][col] : 0;
}

int16_t pw_test_P60A_nopack(pw_ctx *c)
{
    int nbr_notes = 0;

    if (c->i < 7)
        return PW_BAD;
    c->start = (int32_t)(c->i - 7);

    /* number of pattern (real) */
    c->m = IN(c->start + 2);
    if ((c->m > 0x7f) || (c->m == 0))
        return PW_BAD;
    /* c->m is the real number of pattern */

    /* number of sample */
    c->k = (IN(c->start + 3) & 0x3F);
    if ((c->k > 0x1F) || (c->k == 0) || ((c->k * 6 + c->start + 7) >= (uint32_t)c->in_size))
        return PW_BAD;
    /* c->k is the number of sample */

    /* test volumes */
    for (c->l = 0; c->l < c->k; c->l++)
    {
        if (IN(c->start + 7 + c->l * 6) > 0x40)
            return PW_BAD;
    }

    /* test fines */
    for (c->l = 0; c->l < c->k; c->l++)
    {
        if (IN(c->start + 6 + c->l * 6) > 0x0F)
            return PW_BAD;
    }

    /* test sample sizes and loop start */
    c->wss = 0;
    for (c->n = 0; c->n < c->k; c->n++)
    {
        c->o = ((IN(c->start + 4 + c->n * 6) * 256) +
                IN(c->start + 5 + c->n * 6));
        if (((c->o < 0xFFDF) && (c->o > 0x8000)) || (c->o == 0))
            return PW_BAD;
        if (c->o < 0xFF00)
            c->wss += (c->o * 2);

        c->j = ((IN(c->start + 8 + c->n * 6) * 256) +
                IN(c->start + 9 + c->n * 6));
        if ((c->j != 0xFFFF) && (c->j >= c->o))
            return PW_BAD;
        if (c->o > 0xFFDF)
        {
            if ((0xFFFF - c->o) > c->k)
                return PW_BAD;
        }
    }

    /* test sample data address */
    c->j = (IN(c->start) * 256) + IN(c->start + 1);
    if (c->j < (c->k * 6 + 4 + c->m * 8))
        return PW_BAD;
    /* c->j is the address of the sample data */

    /* test track table */
    for (c->l = 0; c->l < (c->m * 4); c->l++)
    {
        c->o = ((IN(c->start + 4 + c->k * 6 + c->l * 2) * 256) +
                IN(c->start + 4 + c->k * 6 + c->l * 2 + 1));
        if ((c->o + c->k * 6 + 4 + c->m * 8) > c->j)
            return PW_BAD;
    }

    /* test pattern table */
    c->l = 0;
    c->o = 0;
    /* first, test if we dont oversize the input file */
    if ((c->start + c->k * 6 + 4 + c->m * 8) > (uint32_t)c->in_size)
        return PW_BAD;
    while ((IN(c->start + c->k * 6 + 4 + c->m * 8 + c->l) != 0xFF) && (c->l < 128))
    {
        if (IN(c->start + c->k * 6 + 4 + c->m * 8 + c->l) > (c->m - 1))
            return PW_BAD;
        if (IN(c->start + c->k * 6 + 4 + c->m * 8 + c->l) > c->o)
            c->o = IN(c->start + c->k * 6 + 4 + c->m * 8 + c->l);
        c->l++;
    }
    /* are we beside the sample data address ? */
    if ((c->k * 6 + 4 + c->m * 8 + c->l) > c->j)
        return PW_BAD;
    if ((c->l == 0) || (c->l == 128))
        return PW_BAD;
    c->o += 1;
    /* c->o is the highest number of pattern */

    /* test notes ... pfiew */
    c->l += 1;
    for (c->n = (c->k * 6 + 4 + c->m * 8 + c->l); c->n < c->j; c->n++)
    {
        if ((IN(c->start + c->n) & 0x80) == 0x00)
        {
            if (IN(c->start + c->n) > 0x49)
                return PW_BAD;
            if (IN(c->start + c->n) >= 0x02)
                nbr_notes = 1;
            if ((uint32_t)(((IN(c->start + c->n) << 4) & 0x10) | ((IN(c->start + c->n + 1) >> 4) & 0x0F)) > c->k)
                return PW_BAD;
            c->n += 2;
        }
        else
            c->n += 3;
    }
    if (nbr_notes == 0)
        return PW_BAD;

    /* c->wss is the whole sample data size */
    /* c->j is the address of the sample data */
    return PW_GOOD;
}

/******************/
/* packed samples */
/******************/
int16_t pw_test_P60A_pack(pw_ctx *c)
{
    if (c->i < 11)
        return PW_BAD;
    c->start = (int32_t)(c->i - 11);

    /* number of pattern (real) */
    c->m = IN(c->start + 2);
    if ((c->m > 0x7f) || (c->m == 0))
        return PW_BAD;
    /* c->m is the real number of pattern */

    /* number of sample */
    c->k = IN(c->start + 3);
    if ((c->k & 0x40) != 0x40)
        return PW_BAD;
    c->k &= 0x3F;
    if ((c->k > 0x1F) || (c->k == 0))
        return PW_BAD;
    /* c->k is the number of sample */

    /* test volumes */
    if ((c->start + 11 + (c->k * 6)) > (uint32_t)c->in_size)
        return PW_BAD;
    for (c->l = 0; c->l < c->k; c->l++)
    {
        if (IN(c->start + 11 + c->l * 6) > 0x40)
            return PW_BAD;
    }

    /* test fines */
    for (c->l = 0; c->l < c->k; c->l++)
    {
        if ((IN(c->start + 10 + c->l * 6) & 0x3F) > 0x0F)
            return PW_BAD;
    }

    /* test sample sizes and loop start */
    c->wss = 0;
    for (c->n = 0; c->n < c->k; c->n++)
    {
        c->o = ((IN(c->start + 8 + c->n * 6) * 256) +
                IN(c->start + 9 + c->n * 6));
        if (((c->o < 0xFFDF) && (c->o > 0x8000)) || (c->o == 0))
            return PW_BAD;
        if (c->o < 0xFF00)
            c->wss += (c->o * 2);

        c->j = ((IN(c->start + 12 + c->n * 6) * 256) +
                IN(c->start + 13 + c->n * 6));
        if ((c->j != 0xFFFF) && (c->j >= c->o))
            return PW_BAD;
        if (c->o > 0xFFDF)
        {
            if ((0xFFFF - c->o) > c->k)
                return PW_BAD;
        }
    }

    /* test sample data address */
    c->j = (IN(c->start) * 256) + IN(c->start + 1);
    if (c->j < (c->k * 6 + 8 + c->m * 8))
        return PW_BAD;
    /* c->j is the address of the sample data */

    /* test track table */
    for (c->l = 0; c->l < (c->m * 4); c->l++)
    {
        c->o = ((IN(c->start + 8 + c->k * 6 + c->l * 2) * 256) +
                IN(c->start + 8 + c->k * 6 + c->l * 2 + 1));
        if ((c->o + c->k * 6 + 8 + c->m * 8) > c->j)
            return PW_BAD;
    }

    /* test pattern table */
    c->l = 0;
    c->o = 0;
    /* first, test if we dont oversize the input file */
    if ((c->k * 6 + 8 + c->m * 8) > (uint32_t)c->in_size)
        return PW_BAD;
    while ((IN(c->start + c->k * 6 + 8 + c->m * 8 + c->l) != 0xFF) && (c->l < 128))
    {
        if (IN(c->start + c->k * 6 + 8 + c->m * 8 + c->l) > (c->m - 1))
            return PW_BAD;
        if (IN(c->start + c->k * 6 + 8 + c->m * 8 + c->l) > c->o)
            c->o = IN(c->start + c->k * 6 + 8 + c->m * 8 + c->l);
        c->l++;
    }
    if ((c->l == 0) || (c->l == 128))
        return PW_BAD;
    c->o += 1;
    /* c->o is the highest number of pattern */

    /* test notes ... pfiew */
    c->l += 1;
    for (c->n = (c->k * 6 + 8 + c->m * 8 + c->l); c->n < c->j; c->n++)
    {
        if ((IN(c->start + c->n) & 0x80) == 0x00)
        {
            if (IN(c->start + c->n) > 0x49)
                return PW_BAD;
            if ((uint32_t)(((IN(c->start + c->n) << 4) & 0x10) | ((IN(c->start + c->n + 1) >> 4) & 0x0F)) > c->k)
                return PW_BAD;
            c->n += 2;
        }
        else
            c->n += 3;
    }

    /* c->wss is the whole sample data size */
    /* c->j is the address of the sample data */
    return PW_GOOD;
}

void pw_rip_P60A(pw_ctx *c)
{
    /* c->j is the number of sample */
    /* c->wss is the whole sample data size */

    c->out_size = c->j + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "The Player 6.0A module", PWF_P60A);

    if (c->save_status == PW_GOOD)
        c->i += 8; /* 7 should do but call it "just to be sure" :) */
}

/*
 *   The_Player_6.0a.c   1998 (c) Asle / ReDoX
 *
 * The Player 6.0a to Protracker.
 *
 * Note: It's a REAL mess !. It's VERY badly coded, I know. Just dont forget
 *      it was mainly done to test the description I made of P60a format. I
 *      certainly wont dare to beat Gryzor on the ground :). His Prowiz IS
 *      the converter to use !!!. Though, using the official depacker could
 *      be a good idea too :).
 *
 * Update : 28/11/99
 *   - removed fopen() and all similar functions
 *   - Speed and Size (?) optimizings
 * Update : 26 nov 2003
 *   - used htonl() so that use of addy is now portable on 68k archs
 */

#define ON 1
#define OFF 2

/* SampleSizes[31], SmpSizes[31][2] and SampleAddresses[32] are indexed
 * with i-1 (-1 for the first sample) and with 0xFFFF-size (up to 32) for
 * "same as sample n" entries: out-of-range reads give 0. */
static int32_t get31(const int32_t *a, int32_t idx)
{
    return (idx >= 0 && idx < 31) ? a[idx] : 0;
}

static int32_t get32(const int32_t *a, int32_t idx)
{
    return (idx >= 0 && idx < 32) ? a[idx] : 0;
}

static uint8_t get_smp(uint8_t a[31][2], int32_t idx, int col)
{
    return (idx >= 0 && idx < 31) ? a[idx][col] : 0;
}

void pw_depack_P60A(pw_ctx *c)
{
    uint8_t  c1, c2, c3, c4, c5, c6;
    int32_t  Max;
    uint8_t  whatever[1024];
    uint8_t *smp_data_work;
    uint8_t  pat_pos = 0x00;
    uint8_t  pat_max = 0x00;
    uint8_t  nbr_sample = 0x00;
    uint8_t  poss[37][2];
    uint8_t *track_data;
    uint8_t  smp_sizes[31][2];
    uint8_t  global_delta = OFF;
    int32_t  track_address[128][4];
    int32_t  track_data_address = 0;
    int32_t  sample_data_address = 0;
    int32_t  i = 0, j, k, l, a, b;
    int32_t  sample_sizes[31];
    int32_t  sample_addresses[32];
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(track_address, 0, sizeof track_address);
    memset(sample_sizes, 0, sizeof sample_sizes);
    memset(sample_addresses, 0, sizeof sample_addresses);
    memset(smp_sizes, 0, sizeof smp_sizes);

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

    /* read sample data address */
    sample_data_address = (IN(where) * 256) + IN(where + 1);

    /* read Real number of pattern (<= 0x7f: checked by the test) */
    pat_max = IN(where + 2);
    where += 3;

    /* read number of samples */
    nbr_sample = IN(where);
    where += 1;
    if ((nbr_sample & 0x80) == 0x80)
    {
        /* Samples are saved as delta values ! */
        global_delta = ON;
    }
    if ((nbr_sample & 0x40) == 0x40)
    {
        /* some samples are packed ! Since I could not understand the packing
         * method of the samples, neither could I do a depacker .. mission
         * ends here :)
         * Upstream returns without closing the file: it is left empty and
         * "done" is not printed. */
        free(track_data);
        pw_fclose(out);
        return;
    }
    nbr_sample &= 0x3F;

    /* write title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 20);

    /* sample headers stuff (nbr_sample <= 31: checked by the test) */
    for (i = 0; i < nbr_sample; i++)
    {
        /* write sample name */
        pw_fwrite(out, whatever, 22);

        /* sample size */
        smp_sizes[i][0] = IN(where);
        smp_sizes[i][1] = IN(where + 1);
        j = (smp_sizes[i][0] * 256) + smp_sizes[i][1];
        if (j > 0xFF00)
        {
            sample_sizes[i] = get31(sample_sizes, 0xFFFF - j);
            smp_sizes[i][0] = get_smp(smp_sizes, 0xFFFF - j, 0);
            smp_sizes[i][1] = get_smp(smp_sizes, 0xFFFF - j, 1);
            sample_addresses[i + 1] = get32(sample_addresses, 0xFFFF - j + 1);
        }
        else
        {
            sample_addresses[i + 1] = sample_addresses[i] + get31(sample_sizes, i - 1);
            sample_sizes[i] = j * 2;
        }
        j = sample_sizes[i] / 2;
        pw_fwrite(out, &smp_sizes[i][0], 1);
        pw_fwrite(out, &smp_sizes[i][1], 1);

        /* finetune (bit 6 flags a packed sample; unused) */
        c1 = IN(where + 2);
        c1 &= 0x3F;
        pw_fwrite(out, &c1, 1);

        /* vol */
        pw_fwrite_in(out, where + 3, 1);

        /* loop start */
        if ((IN(where + 4) == 0xFF) && (IN(where + 5) == 0xFF))
        {
            whatever[53] = 0x01;
            pw_fwrite(out, &whatever[50], 4);
            where += 6;
            continue;
        }
        pw_fwrite_in(out, where + 4, 2);
        l = j - ((IN(where + 4) * 256) + IN(where + 5));

        /* low 16 bits of l, big-endian (bytes 2 and 3 of htonl(l)) */
        pw_fput16be(out, (uint32_t)l);

        where += 6;
    }

    /* go up to 31 samples */
    whatever[129] = 0x01;
    while (i != 31)
    {
        pw_fwrite(out, &whatever[100], 30);
        i += 1;
    }

    /* read tracks addresses per pattern */
    for (i = 0; i < pat_max; i++)
    {
        for (j = 0; j < 4; j++)
        {
            track_address[i][j] = (IN(where) * 256) + IN(where + 1);
            where += 2;
        }
    }

    /* pattern table */
    memset(whatever, 0, sizeof whatever);
    for (pat_pos = 0; pat_pos < 128; pat_pos++)
    {
        c1 = IN(where++);
        if (c1 == 0xFF)
            break;
        whatever[pat_pos] = c1;
    }

    /* write size of pattern list */
    pw_fwrite(out, &pat_pos, 1);

    /* write noisetracker byte */
    c1 = 0x7f;
    pw_fwrite(out, &c1, 1);

    /* write pattern table */
    pw_fwrite(out, whatever, 128);

    /* write ptk's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    track_data_address = where;

    /* rewrite the track data */
    for (i = 0; i < pat_max; i++)
    {
        Max = 63;
        for (j = 0; j < 4; j++)
        {
            where = track_address[i][j] + track_data_address;
            for (k = 0; k <= Max; k++)
            {
                /* guard: past the end of the input every byte reads 0, so each
                 * remaining row is a plain empty note; once k is beyond
                 * Track_Data those rows change nothing, so stop. Upstream
                 * counted on up to Max, which a pattern break inside a repeat
                 * can push towards INT32_MAX (where k*4 wraps around back into
                 * the array: not emulated). */
                if (where >= c->in_size && k >= (TD_ROWS * TD_COLS) / 4)
                    break;

                c1 = IN(where++);
                c2 = IN(where++);
                c3 = IN(where++);
                if (((c1 & 0x80) == 0x80) && (c1 != 0x80))
                {
                    c4 = IN(where++);
                    c1 = 0xFF - c1;
                    td_set(track_data, i * 4 + j, k * 4, (uint8_t)(((c1 << 4) & 0x10) | poss_at(poss, c1 / 2, 0)));
                    td_set(track_data, i * 4 + j, k * 4 + 1, poss_at(poss, c1 / 2, 1));
                    c6 = c2 & 0x0f;
                    if (c6 == 0x08)
                        c2 -= 0x08;
                    td_set(track_data, i * 4 + j, k * 4 + 2, c2);
                    if ((c6 == 0x05) || (c6 == 0x06) || (c6 == 0x0a))
                        c3 = (uint8_t)((c3 > 0x7f) ? ((0x100 - c3) << 4) : c3);
                    td_set(track_data, i * 4 + j, k * 4 + 3, c3);
                    if (c6 == 0x0D)
                    {
                        /* PATTERN BREAK !, track ends */
                        Max = k;
                        k = 9999;
                        continue;
                    }
                    if (c6 == 0x0B)
                    {
                        /* PATTERN JUMP !, track ends */
                        Max = k;
                        k = 9999;
                        continue;
                    }

                    if (c4 < 0x80)
                    {
                        /* bypass c4 rows */
                        k += c4;
                        continue;
                    }
                    /* repeat current row 0x100-c4 times */
                    c4 = (uint8_t)(0x100 - c4);
                    for (l = 0; l < c4; l++)
                    {
                        k += 1;
                        td_set(track_data, i * 4 + j, k * 4, (uint8_t)(((c1 << 4) & 0x10) | poss_at(poss, c1 / 2, 0)));
                        td_set(track_data, i * 4 + j, k * 4 + 1, poss_at(poss, c1 / 2, 1));
                        c6 = c2 & 0x0f;
                        if (c6 == 0x08)
                            c2 -= 0x08;
                        td_set(track_data, i * 4 + j, k * 4 + 2, c2);
                        if ((c6 == 0x05) || (c6 == 0x06) || (c6 == 0x0a))
                            c3 = (uint8_t)((c3 > 0x7f) ? ((0x100 - c3) << 4) : c3);
                        td_set(track_data, i * 4 + j, k * 4 + 3, c3);
                    }
                    continue;
                }
                if (c1 == 0x80)
                {
                    c4 = IN(where++);
                    /* repeat c2+1 lines some (c3*256)+c4 bytes before */
                    a = where;
                    c5 = c2;
                    where -= ((c3 * 256) + c4);
                    for (l = 0; l <= c5; l++, k++)
                    {
                        c1 = IN(where++);
                        c2 = IN(where++);
                        c3 = IN(where++);
                        if (((c1 & 0x80) == 0x80) && (c1 != 0x80))
                        {
                            c4 = IN(where++);
                            c1 = 0xFF - c1;
                            td_set(track_data, i * 4 + j, k * 4, (uint8_t)(((c1 << 4) & 0x10) | poss_at(poss, c1 / 2, 0)));
                            td_set(track_data, i * 4 + j, k * 4 + 1, poss_at(poss, c1 / 2, 1));
                            c6 = c2 & 0x0f;
                            if (c6 == 0x08)
                                c2 -= 0x08;
                            td_set(track_data, i * 4 + j, k * 4 + 2, c2);
                            if ((c6 == 0x05) || (c6 == 0x06) || (c6 == 0x0a))
                                c3 = (uint8_t)((c3 > 0x7f) ? ((0x100 - c3) << 4) : c3);
                            td_set(track_data, i * 4 + j, k * 4 + 3, c3);
                            if (c6 == 0x0D)
                            {
                                /* PATTERN BREAK !, track ends */
                                Max = k;
                                k = l = 9999;
                                continue;
                            }
                            if (c6 == 0x0B)
                            {
                                /* PATTERN JUMP !, track ends */
                                Max = k;
                                k = l = 9999;
                                continue;
                            }

                            if (c4 < 0x80)
                            {
                                /* bypass c4 rows */
                                k += c4;
                                continue;
                            }
                            /* repeat current row 0x100-c4 times */
                            c4 = (uint8_t)(0x100 - c4);
                            for (b = 0; b < c4; b++)
                            {
                                k += 1;
                                td_set(track_data, i * 4 + j, k * 4, (uint8_t)(((c1 << 4) & 0x10) | poss_at(poss, c1 / 2, 0)));
                                td_set(track_data, i * 4 + j, k * 4 + 1, poss_at(poss, c1 / 2, 1));
                                c6 = c2 & 0x0f;
                                if (c6 == 0x08)
                                    c2 -= 0x08;
                                td_set(track_data, i * 4 + j, k * 4 + 2, c2);
                                if ((c6 == 0x05) || (c6 == 0x06) || (c6 == 0x0a))
                                    c3 = (uint8_t)((c3 > 0x7f) ? ((0x100 - c3) << 4) : c3);
                                td_set(track_data, i * 4 + j, k * 4 + 3, c3);
                            }
                        }
                        td_set(track_data, i * 4 + j, k * 4, (uint8_t)(((c1 << 4) & 0x10) | poss_at(poss, c1 / 2, 0)));
                        td_set(track_data, i * 4 + j, k * 4 + 1, poss_at(poss, c1 / 2, 1));
                        c6 = c2 & 0x0f;
                        if (c6 == 0x08)
                            c2 -= 0x08;
                        td_set(track_data, i * 4 + j, k * 4 + 2, c2);
                        if ((c6 == 0x05) || (c6 == 0x06) || (c6 == 0x0a))
                            c3 = (uint8_t)((c3 > 0x7f) ? ((0x100 - c3) << 4) : c3);
                        td_set(track_data, i * 4 + j, k * 4 + 3, c3);
                    }
                    where = a;
                    k -= 1;
                    continue;
                }

                td_set(track_data, i * 4 + j, k * 4, (uint8_t)(((c1 << 4) & 0x10) | poss_at(poss, c1 / 2, 0)));
                td_set(track_data, i * 4 + j, k * 4 + 1, poss_at(poss, c1 / 2, 1));
                c6 = c2 & 0x0f;
                if (c6 == 0x08)
                    c2 -= 0x08;
                td_set(track_data, i * 4 + j, k * 4 + 2, c2);
                if ((c6 == 0x05) || (c6 == 0x06) || (c6 == 0x0a))
                    c3 = (uint8_t)((c3 > 0x7f) ? ((0x100 - c3) << 4) : c3);
                td_set(track_data, i * 4 + j, k * 4 + 3, c3);
                if (c6 == 0x0D)
                {
                    /* PATTERN BREAK !, track ends */
                    Max = k;
                    k = 9999;
                    continue;
                }
                if (c6 == 0x0B)
                {
                    /* PATTERN JUMP !, track ends */
                    Max = k;
                    k = 9999;
                    continue;
                }
            }
        }
    }

    /* write pattern data */
    for (i = 0; i < pat_max; i++)
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
        where = c->start + sample_data_address + sample_addresses[i + 1];
        smp_data_work = (uint8_t *)calloc(sample_sizes[i] > 0 ? (size_t)sample_sizes[i] : 1, 1);
        if (smp_data_work == NULL)
        {
            c->fatal = PW_ERR_NOMEM;
            pw_fdiscard(out);
            return;
        }
        for (j = 0; j < sample_sizes[i]; j++)
            smp_data_work[j] = IN(where + j);
        if (global_delta == ON)
        {
            c1 = smp_data_work[0];
            for (j = 1; j < sample_sizes[i]; j++)
            {
                c2 = smp_data_work[j];
                c3 = c1 - c2;
                smp_data_work[j] = c3;
                c1 = c3;
            }
        }
        pw_fwrite(out, smp_data_work, sample_sizes[i]);
        free(smp_data_work);
    }
    if (global_delta == ON)
        pw_crap(" The Player 6.0A  ", PW_GOOD, PW_BAD, out);
    else
        pw_crap(" The Player 6.0A  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
