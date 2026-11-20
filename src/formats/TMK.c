/*
 * TMK.c - TMK ("TMK\x01" ID, Timetracker ?) packed modules (upstream r/TMK.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* poss[] has 37 entries; upstream indexed it with note bytes up to 127 */
static uint8_t poss_at(const uint8_t poss[37][2], unsigned n, int b)
{
    return (n < 37) ? poss[n][b] : 0;
}

int16_t pw_test_TMK(pw_ctx *c)
{
    /* test 1 */
    c->start = (int32_t)c->i;
    if ((c->start + 100) > c->in_size)
        return PW_BAD;

    /* test 2  - nbr of smps*/
    c->j = IN(c->start + 5) & 0x7f;
    if (c->j > 0x1f)
        return PW_BAD;

    /* test 2 - size of patlist */
    c->l = IN(c->start + 4);
    if (c->l > 0x80)
        return PW_BAD;

    /* test 2,5 - out of in_file ? */
    if (c->start + 6 + (c->j * 8) + c->l > (uint32_t)c->in_size)
        return PW_BAD;

    /* test 3 - fine (upstream compares an address with a size here) */
    for (c->k = (c->start + 12); c->k < c->j * 8; c->k += 8)
        if (IN(c->k) > 0x0f)
            return PW_BAD;

    /* test 4 - vol */
    for (c->k = (c->start + 13); c->k < c->j * 8; c->k += 8)
        if (IN(c->k) > 0x40)
            return PW_BAD;

    /* get all smp size */
    c->wss = 0;
    for (c->k = c->start + 6; c->k < c->start + 6 + c->j * 8; c->k += 8)
        c->wss += (((IN(c->k) * 256) + IN(c->k + 1)) * 2);

    /* test 5 - max of line pointer */
    /* first, max of patlist */
    c->m = 0;
    for (c->k = c->start + 6 + (8 * c->j); c->k < (c->start + 6 + (8 * c->j) + c->l); c->k++)
        if (IN(c->k) > c->m)
            c->m = IN(c->k) / 2 + 1;

    if ((c->m * 64 * 2) + 6 + (8 * c->j) + c->l + c->start > (uint32_t)c->in_size)
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_TMK(pw_ctx *c)
{
    /* c->j is the number of samples - useless*/
    /* c->l is the patlist size */
    /* c->wss is the whole sample size :) */
    /* c->m is the highest pat nbr */

    /* get highest ref addy */
    /* also taking care of the uneven patlist size */
    if ((c->l / 2) * 2 != c->l)
        c->l += 1;
    c->n = 6 + (8 * c->j) + c->l + c->start;
    /* now c->n points on the first referencer */
    /* hence c->j and c->l are free */
    c->l = 0;
    for (c->k = c->n; c->k < c->n + (c->m * 128); c->k += 2)
    {
        c->o = ((IN(c->k) * 256) + IN(c->k + 1));
        if (c->o > c->l)
            c->l = c->o;
    }
    /* c->l is now the highest ref pointer */
    /* remain to depack the last line */

    c->o = 0;
    for (c->j = 0; c->j < 4; c->j++)
    {
        if (IN(c->l + c->n + c->o) == 0x80)
        {
            c->o += 1;
            continue;
        }
        if ((IN(c->l + c->n + c->o) & 0x80) == 0x80)
        {
            c->o += 2;
            continue;
        }
        c->o += 3;
    }

    c->out_size = c->wss + c->n + c->l + c->o - c->start;

    c->convert = PW_GOOD;
    pw_save_rip(c, "TMK module", PWF_TMK);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1); /* 0 should do but call it "just to be sure" :) */
}

/*
 *   tmk.c   2003 (c) sylvain "Asle" Chipaux
 *
 * Converts TMK packed MODs back to PTK MODs
 * 20040906 - added the uneven case in the patternlist
 */
#define TMK_ON  0
#define TMK_OFF 1

void pw_depack_TMK(pw_ctx *c)
{
    uint8_t  pattern[1024];
    int32_t  i = 0, j = 0, k = 0, l = 0;
    int32_t  total_sample_size = 0;
    uint8_t  poss[37][2];

    int      patdataaddy;
    int      reftableaddy;
    int      maxlineaddy;
    int      sizlastline;
    int32_t  samplesizes[128]; /* NOS is 7 bits */
    uint8_t  patternlist[128];
    uint8_t  maxpat = 0x00;
    uint8_t  nop = 0x00; /* number of pattern */
    uint8_t  nos = 0x00; /* number of sample */
    uint8_t  whatever[128];
    uint8_t  global_delta = TMK_OFF;
    int32_t  where = c->start;
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    memset(patternlist, 0, sizeof patternlist);
    memset(samplesizes, 0, sizeof samplesizes);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(whatever, 0, sizeof whatever);

    /* title */
    pw_fwrite(out, &whatever[0], 20);

    /* bypass TMK.*/
    where += 4;

    /* get nbr of samples/patterns */
    nop = IN(where);
    nos = IN(where + 1) & 0x7f;
    if ((IN(where + 1) & 0x80) == 0x80)
        global_delta = TMK_ON;
    where += 2;

    for (i = 0; i < nos; i++)
    {
        /*sample name*/
        pw_fwrite(out, &whatever[32], 22);

        /* siz */
        pw_fwrite_in(out, where, 2);
        /* fine/vol */
        pw_fwrite_in(out, where + 6, 2);
        /* repstart/replen */
        pw_fwrite_in(out, where + 2, 4);

        /* whole sample size */
        total_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);
        samplesizes[i] = (((IN(where) * 256) + IN(where + 1)) * 2);
        where += 8;
    }
    whatever[29] = 0x01;
    while (i < 31)
    {
        pw_fwrite(out, whatever, 30);
        i++;
    }

    /* write patlist (get also max) */
    pw_fwrite(out, &nop, 1);
    whatever[0] = 0x7f;
    pw_fwrite(out, &whatever[0], 1);
    for (i = 0; i < nop; i++)
    {
        /* the test keeps nop <= 0x80; guard the 128-entry list anyway */
        if (i >= 128)
            break;
        patternlist[i] = IN(where + i) / 2; /*pat list nbrs are *2 */
        if (patternlist[i] > maxpat)
            maxpat = patternlist[i];
    }
    pw_fwrite(out, patternlist, 128);
    maxpat += 1; /* the first value is 0 ... */

    where += nop;

    /* write tag */
    whatever[0] = 'M';
    whatever[1] = whatever[3] = '.';
    whatever[2] = 'K';

    pw_fwrite(out, &whatever[0], 4);

    /* taking care of the uneven case of patlist size */
    if ((nop / 2) * 2 != nop)
        where += 1;
    reftableaddy = where;
    patdataaddy = maxpat * 128 + reftableaddy; /* one ref is a int16_t	:64*2 */

    /*get siz of last 4 notes at max(patdataaddy) */
    maxlineaddy = 0;
    for (i = reftableaddy; i < patdataaddy; i += 2)
    {
        k = (IN(i) * 256 + IN(i + 1));
        if (k > maxlineaddy)
            maxlineaddy = k;
    }

    maxlineaddy += reftableaddy; /* to reach the real addy in mem/file */
    k = maxlineaddy;

    sizlastline = 0;
    for (i = 0; i < 4; i++)
    {
        if (IN(k) == 0x80)
        {
            sizlastline += 1;
            k += 1;
            continue;
        }
        if ((IN(k) & 0x80) == 0x80)
        {
            sizlastline += 2;
            k += 2;
            continue;
        }
        else
            sizlastline += 3;
        k += 3;
    }

    /* pat data */
    for (i = 0; i < maxpat; i += 1)
    {
        memset(pattern, 0, sizeof pattern);
        for (j = 0; j < 64; j++)
        {
            k = (IN(reftableaddy + (i * 128) + (j * 2)) * 256) +
                IN(reftableaddy + (i * 128) + (j * 2) + 1) + reftableaddy;
            for (l = 0; l < 4; l++)
            {
                if (IN(k) == 0x80)
                {
                    k += 1;
                    continue;
                }
                if ((IN(k) & 0xC0) == 0xC0)
                { /* only sfx */
                    pattern[j * 16 + l * 4 + 2] = IN(k) & 0x0f;
                    pattern[j * 16 + l * 4 + 3] = IN(k + 1);
                    k += 2;
                    continue;
                }
                if ((IN(k) & 0xC0) == 0x80)
                { /* note (not *2!) + smp */
                    pattern[j * 16 + l * 4] = IN(k + 1) & 0xf0;
                    pattern[j * 16 + l * 4] |= poss_at(poss, IN(k) & 0x7f, 0);
                    pattern[j * 16 + l * 4 + 1] = poss_at(poss, IN(k) & 0x7f, 1);
                    pattern[j * 16 + l * 4 + 2] = (uint8_t)(IN(k + 1) << 4);
                    k += 2;
                    continue;
                }
                else
                {
                    pattern[j * 16 + l * 4] = (uint8_t)((IN(k) & 0x01) << 4);
                    pattern[j * 16 + l * 4] |= poss_at(poss, IN(k) / 2, 0);
                    pattern[j * 16 + l * 4 + 1] = poss_at(poss, IN(k) / 2, 1);
                    pattern[j * 16 + l * 4 + 2] = IN(k + 1);
                    pattern[j * 16 + l * 4 + 3] = IN(k + 2);
                    k += 3;
                }
            }
            if (k + 8 > where)
                where = k + 8;
        }
        pw_fwrite(out, pattern, 1024);
    }

    /* sample data */
    if (global_delta == TMK_ON)
    {
        uint8_t  c1, c2, c3;
        uint8_t *smp_data_work;
        k = maxlineaddy + sizlastline;
        for (i = 0; i < nos; i++)
        {
            /* at least 1 byte: upstream read smp_data_work[0] even for an
             * empty sample (its zero-filled allocation gave 0) */
            smp_data_work = (uint8_t *)calloc((size_t)samplesizes[i] + 1, 1);
            if (smp_data_work == NULL)
            {
                c->fatal = PW_ERR_NOMEM;
                break;
            }
            for (j = 0; j < samplesizes[i]; j++)
                smp_data_work[j] = IN(k + j);
            c1 = smp_data_work[0];
            for (j = 1; j < samplesizes[i]; j++)
            {
                c2 = smp_data_work[j];
                c3 = (uint8_t)(c1 - c2);
                smp_data_work[j] = c3;
                c1 = c3;
            }
            pw_fwrite(out, smp_data_work, samplesizes[i]);
            free(smp_data_work);
            k += samplesizes[i];
        }
    }
    else
        pw_fwrite_in(out, maxlineaddy + sizlastline, total_sample_size);

    if (global_delta == TMK_ON)
        pw_crap("        TMK       ", PW_GOOD, PW_BAD, out);
    else
        pw_crap("        TMK       ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
