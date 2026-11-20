/*
 * HeatseekerMC10.c - Heatseeker mc1.0 (upstream r/HeatseekerMC10.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/*  (May 2002)
 *    added test_smps()
 */
int16_t pw_test_HEATSEEKER(pw_ctx *c)
{
    int nbr_notes = 0;

    if ((c->i < 3) || ((c->i + 375) >= (uint32_t)c->in_size))
        return PW_BAD;
    c->start = (int32_t)(c->i - 3);

    /* size of the pattern table */
    if ((IN(c->start + 248) > 0x7f) ||
        (IN(c->start + 248) == 0x00))
        return PW_BAD;

    /* test noisetracker byte */
    if (IN(c->start + 249) != 0x7f)
        return PW_BAD;

    /* test samples */
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        /* size */
        c->j = (IN(c->start + c->k * 8) * 256) + IN(c->start + 1 + c->k * 8);
        /* loop start */
        c->m = (IN(c->start + c->k * 8 + 4) * 256) + IN(c->start + 5 + c->k * 8);
        /* loop size */
        c->n = (IN(c->start + c->k * 8 + 6) * 256) + IN(c->start + 7 + c->k * 8);
        c->j *= 2;
        c->m *= 2;
        c->n *= 2;
        if (pw_test_smps((int32_t)c->j, (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 3 + c->k * 8), IN(c->start + 2 + c->k * 8)) == PW_BAD)
            return PW_BAD;
        if ((c->j > 0xFFFF) ||
            (c->m > 0xFFFF) ||
            (c->n > 0xFFFF))
            return PW_BAD;
        c->wss += c->j;
    }
    if (c->wss <= 4)
        return PW_BAD;

    /* test pattern table */
    c->l = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 250 + c->j) > 0x7f)
            return PW_BAD;
        if (IN(c->start + 250 + c->j) > c->l)
            c->l = IN(c->start + 250 + c->j);
    }
    /* c->l = highest pattern number */
    if ((c->start + (c->l * 16) + 378) > (uint32_t)c->in_size)
    {
        /* c->l*16 is the minimum size of all patterns */
        return PW_BAD;
    }

    /* test notes */
    c->k = 0;
    c->j = 0;
    for (c->m = 0; c->m <= c->l; c->m++)
    {
        for (c->n = 0; c->n < 4; c->n++)
        {
            for (c->o = 0; c->o < 64; c->o++)
            {
                switch (IN(c->start + 378 + c->j) & 0xE0)
                {
                case 0x00:
                    if (((IN(c->start + 378 + c->j) & 0x0F) > 0x03) ||
                        (((IN(c->start + 378 + c->j) & 0x0F) == 0x00) &&
                         (IN(c->start + 379 + c->j) < 0x71) &&
                         (IN(c->start + 379 + c->j) != 0x00)))
                        return PW_BAD;
                    if (((IN(c->start + 380 + c->j) & 0x0f) == 0x0d) && (IN(c->start + 381 + c->j) > 0x64))
                        return PW_BAD;
                    c->k += 4;
                    c->j += 4;
                    if (((IN(c->start + 378 + c->j) & 0x0f) != 0x00) || (IN(c->start + 379 + c->j) != 0x00))
                        nbr_notes = 1;
                    break;
                case 0x80:
                    if ((IN(c->start + 379 + c->j) != 0x00) || (IN(c->start + 380 + c->j) != 0x00))
                        return PW_BAD;
                    c->o += IN(c->start + 381 + c->j);
                    c->j += 4;
                    c->k += 4;
                    break;
                case 0xC0:
                    if (IN(c->start + 379 + c->j) != 0x00)
                        return PW_BAD;
                    c->o = 100;
                    c->j += 4;
                    c->k += 4;
                    break;
                default:
                    return PW_BAD;
                }
            }
        }
    }
    if (nbr_notes == 0)
    {
        /* only empty notes */
        return PW_BAD;
    }

    /* c->k is the size of the pattern data */
    /* c->wss is the size of the sample data */
    return PW_GOOD;
}

void pw_rip_HEATSEEKER(pw_ctx *c)
{
    c->out_size = c->k + c->wss + 378;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Heatseeker module", PWF_Heatseeker);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 4); /* 3 should do but call it "just to be sure" :) */
}

/*
 *   Heatseeker_mc1.0.c   1997 (c) Asle / ReDoX
 *
 * Converts back to ptk Heatseeker packed MODs
 *
 * Note: There's a good job ! .. gosh !.
 */
void pw_depack_HEATSEEKER(pw_ctx *c)
{
    uint8_t  c1 = 0x00, c2 = 0x00, c3 = 0x00, c4 = 0x00;
    uint8_t  pat_max = 0x00;
    uint8_t  whatever[1024];
    int32_t  track_addresses[512];
    int32_t  i = 0, j = 0, k = 0, l = 0, m;
    int32_t  whole_sample_size = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(track_addresses, 0, sizeof track_addresses);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* write title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 20);

    /* read and write sample descriptions */
    for (i = 0; i < 31; i++)
    {
        /*sample name*/
        pw_fwrite(out, whatever, 22);

        whole_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);
        pw_fwrite_in(out, where, 6);
        whatever[32] = IN(where + 6);
        whatever[33] = IN(where + 7);
        if ((whatever[32] == 0x00) && (whatever[33] == 0x00))
            whatever[33] = 0x01;
        pw_fwrite(out, &whatever[32], 2);
        where += 8;
    }

    /* read and write pattern table lenght */
    /* read and write NoiseTracker byte */
    pw_fwrite_in(out, where, 2);
    where += 2;

    /* read and write pattern list and get highest patt number */
    for (i = 0; i < 128; i++)
    {
        whatever[i] = IN(where++);
        if (whatever[i] > pat_max)
            pat_max = whatever[i];
    }
    pw_fwrite(out, whatever, 128);
    pat_max += 1;

    /* write ptk's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* pattern data (pat_max <= 128: the test checks the list; the bound
     * below only keeps the table safe) */
    for (i = 0; i < pat_max; i++)
    {
        memset(whatever, 0, sizeof whatever);
        for (j = 0; j < 4; j++)
        {
            if (i * 4 + j < 512)
                track_addresses[i * 4 + j] = where;
            for (k = 0; k < 64; k++)
            {
                c1 = IN(where++);
                if (c1 == 0x80)
                {
                    c2 = IN(where++);
                    c3 = IN(where++);
                    c4 = IN(where++);
                    k += c4;
                    continue;
                }
                if (c1 == 0xc0)
                {
                    int32_t t;

                    c2 = IN(where++);
                    c3 = IN(where++);
                    c4 = IN(where++);
                    l = where;
                    /* the index goes up to 16383: past the 512 entries the
                     * original read neighbouring stack memory */
                    t = ((c3 * 256) + c4) / 4;
                    where = (t < 512) ? track_addresses[t] : 0;
                    for (m = 0; m < 64; m++)
                    {
                        c1 = IN(where++);
                        if (c1 == 0x80)
                        {
                            c2 = IN(where++);
                            c3 = IN(where++);
                            c4 = IN(where++);
                            m += c4;
                            continue;
                        }
                        c2 = IN(where++);
                        c3 = IN(where++);
                        c4 = IN(where++);
                        whatever[m * 16 + j * 4] = c1;
                        whatever[m * 16 + j * 4 + 1] = c2;
                        whatever[m * 16 + j * 4 + 2] = c3;
                        whatever[m * 16 + j * 4 + 3] = c4;
                    }
                    where = l;
                    k += 100;
                    continue;
                }
                c2 = IN(where++);
                c3 = IN(where++);
                c4 = IN(where++);
                whatever[k * 16 + j * 4] = c1;
                whatever[k * 16 + j * 4 + 1] = c2;
                whatever[k * 16 + j * 4 + 2] = c3;
                whatever[k * 16 + j * 4 + 3] = c4;
            }
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap(" Heatseeker mc1.0 ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
