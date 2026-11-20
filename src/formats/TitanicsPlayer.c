/*
 * TitanicsPlayer.c - Titanics Player modules (upstream r/TitanicsPlayer.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* (in[a]*256*256*256)+(in[a+1]*256*256)+(in[a+2]*256)+in[a+3], each offset
 * an unsigned 32-bit expression as upstream wrote it */
static uint32_t be32u(const pw_ctx *c, uint32_t a)
{
    return ((uint32_t)IN(a) << 24) | ((uint32_t)IN((uint32_t)(a + 1)) << 16) |
           ((uint32_t)IN((uint32_t)(a + 2)) << 8) | (uint32_t)IN((uint32_t)(a + 3));
}

/* same, for a signed 32-bit offset (upstream relied on int wrap-around) */
static int32_t be32s(const pw_ctx *c, int32_t a)
{
    return (int32_t)(((uint32_t)IN(a) << 24) | ((uint32_t)IN((int32_t)((uint32_t)a + 1)) << 16) |
                     ((uint32_t)IN((int32_t)((uint32_t)a + 2)) << 8) |
                     (uint32_t)IN((int32_t)((uint32_t)a + 3)));
}

int16_t pw_test_TitanicsPlayer(pw_ctx *c)
{
    if (c->i < 7)
        return PW_BAD;
    c->start = (int32_t)(c->i - 7);

    /* test samples */
    c->n = c->wss = 0;
    for (c->k = 0; c->k < 15; c->k++)
    {
        if (IN(c->start + 7 + c->k * 12) > 0x40)
            return PW_BAD;
        if (IN(c->start + 6 + c->k * 12) != 0x00)
            return PW_BAD;

        c->o = be32u(c, c->start + c->k * 12);
        if ((c->o > (uint32_t)c->in_size) || ((c->o < 180) && (c->o != 0)))
            return PW_BAD;

        /* size */
        c->j = (IN(c->start + 4 + c->k * 12) * 256) + IN(c->start + 5 + c->k * 12);
        /* loop start */
        c->l = (IN(c->start + 8 + c->k * 12) * 256) + IN(c->start + 9 + c->k * 12);
        /* loop size */
        c->m = (IN(c->start + 10 + c->k * 12) * 256) + IN(c->start + 11 + c->k * 12);
        if ((c->l > c->j) || (c->m > c->j + 1) || (c->j > 32768))
            return PW_BAD;
        if (c->m == 0)
            return PW_BAD;
        if ((c->j == 0) && ((c->l != 0) || (c->m != 1)))
            return PW_BAD;
        if ((c->j != 0) && (c->j == c->l))
            return PW_BAD;
        c->wss += c->j;
    }
    if (c->wss < 2)
        return PW_BAD;

    /* test pattern addresses */
    c->o = PW_BAD;
    c->l = 0;
    for (c->k = 0; c->k < 256; c->k += 2)
    {
        if ((IN(c->start + c->k + 180) == 0xFF) && (IN(c->start + c->k + 181) == 0xFF))
        {
            c->o = PW_GOOD;
            break;
        }
        c->j = ((IN(c->start + c->k + 180) * 256) + IN(c->start + c->k + 181));
        if (c->j < 180)
            return PW_BAD;
        if (c->j > c->l)
            c->l = c->j;
    }
    if (c->o == PW_BAD)
        return PW_BAD;

    /* c->l is the max addy of the pattern addies */
    return PW_GOOD;
}

/* With the help of Xigh :) .. thx */
static int cmp_int32(const void *a, const void *b)
{
    const int32_t *aa = (const int32_t *)a;
    const int32_t *bb = (const int32_t *)b;
    if (*aa == *bb)
        return 0;
    if (*aa > *bb)
        return +1;
    return -1;
}

/* 20100101 : little fix in a test of the nbr of row being too big
 * pb pointed out by Claudio of XMP team. Thanks !
 */
void pw_rip_TitanicsPlayer(pw_ctx *c)
{
    /* retrieve highest smp addy */
    c->j = 0;
    c->n = 0;
    for (c->k = 0; c->k < 15; c->k++)
    {
        c->o = be32u(c, c->start + c->k * 12);
        if (c->o > c->n)
        {
            c->n = c->o;
            c->j = c->k;
        }
    }
    /* c->n is the highest smp addy ; c->j is its ref */
    c->out_size = c->n + (((IN(c->start + c->j * 12 + 4) * 256) + IN(c->start + c->j * 12 + 5)) * 2);

    c->convert = PW_GOOD;
    pw_save_rip(c, "Titanics Player module", PWF_TitanicsPlayer);

    if (c->save_status == PW_GOOD)
        c->i += 180; /* after smp desc */
}

/*
 *   TitanicsPlayer.c   2007 (c) Sylvain "Asle" Chipaux
 */
void pw_depack_TitanicsPlayer(pw_ctx *c)
{
    uint8_t  whatever[2048];
    uint8_t  c1 = 0x00;
    int32_t  pat_addresses[128];
    int32_t  pat_addresses_ord[128];
    int32_t  pat_addresses_final[128];
    int32_t  max = 0;
    uint8_t  poss[37][2];
    uint8_t  pat_pos;
    int32_t  where = c->start;
    int32_t  smp_addresses[15];
    int32_t  sample_sizes[15];
    uint32_t i = 0, j = 0, k = 0;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(pat_addresses, 0, sizeof pat_addresses);
    memset(pat_addresses_ord, 0, sizeof pat_addresses_ord);
    memset(pat_addresses_final, 0, sizeof pat_addresses_final);
    memset(smp_addresses, 0, sizeof smp_addresses);
    memset(sample_sizes, 0, sizeof sample_sizes);

    pw_fill_ptk_table(poss);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.stk", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, &whatever[0], 20);

    /* read and write whole header */
    for (i = 0; i < 15; i++) /* only 15 samples */
    {
        /* retrieve addresses of samples */
        smp_addresses[i] = be32s(c, where);

        /* write name */
        pw_fwrite(out, &whatever[0], 22);
        /* size */
        k = (IN(where + 4) * 256) + IN(where + 5);
        k *= 2;
        sample_sizes[i] = (int32_t)k;
        pw_fwrite_in(out, where + 4, 1);
        pw_fwrite_in(out, where + 5, 1);
        /* finetune */
        pw_fwrite_in(out, where + 6, 1);
        /* volume */
        pw_fwrite_in(out, where + 7, 1);
        /* loop start */
        pw_fwrite_in(out, where + 8, 1);
        pw_fwrite_in(out, where + 9, 1);
        pw_fwrite_in(out, where + 10, 1);
        pw_fwrite_in(out, where + 11, 1);

        where += 12;
    }

    /* pattern list */
    for (pat_pos = 0x00; pat_pos < 128; pat_pos++)
    {
        if (IN(where + pat_pos * 2) == 0xFF)
            break;
        pat_addresses_ord[pat_pos] = pat_addresses[pat_pos] =
            (IN(where + pat_pos * 2) * 256) + IN(where + pat_pos * 2 + 1);
    }

    /* write patpos */
    pw_fwrite(out, &pat_pos, 1);

    /* restart byte */
    c1 = 0x78; /* 20130420 fix submitted by Warren Willmey */
    pw_fwrite(out, &c1, 1);

    /* With the help of Xigh :) .. thx */
    qsort(pat_addresses_ord, pat_pos, sizeof(int32_t), cmp_int32);
    j = 0;
    for (i = 0; i < pat_pos; i++)
    {
        /* j <= i < 128 */
        pat_addresses_final[j++] = pat_addresses_ord[i];
        /* upstream read pat_addresses_ord[i+1] before testing i; the test
         * guarantees pat_pos <= 127 and sorted entries >= 180, so i+1 < 128 */
        while ((i + 1 < 128) && (pat_addresses_ord[i + 1] == pat_addresses_ord[i]) && (i < pat_pos))
            i += 1;
    }

    /* write pattern list */
    for (i = 0; i < pat_pos; i++)
    {
        /* always found within the first pat_pos entries; bounded for safety */
        for (j = 0; (j < 128) && (pat_addresses[i] != pat_addresses_final[j]); j++)
        {
        }
        whatever[i] = (uint8_t)j;
        if (j > (uint32_t)max)
            max = (int32_t)j;
    }
    pw_fwrite(out, whatever, 128);

    /* pattern data */
    for (i = 0; i <= (uint32_t)max; i++)
    {
        k = 0;
        where = ((i < 128) ? pat_addresses_final[i] : 0) + c->start;
        memset(whatever, 0, sizeof whatever);
        do
        {
            /* k is row nbr */
            c1 = (uint8_t)(((IN(where + 1) >> 6) & 0x03) * 4); /* voice */

            /* no note ... */
            if ((IN(where + 1) & 0x3f) <= 36)
            {
                whatever[(k * 16) + c1] = poss[IN(where + 1) & 0x3f][0];
                whatever[(k * 16) + c1 + 1] = poss[IN(where + 1) & 0x3F][1];
            }
            whatever[(k * 16) + c1 + 2] = IN(where + 2);
            whatever[(k * 16) + c1 + 3] = IN(where + 3);
            where += 4;

            if ((IN(where) & 0x7f) != 0x00)
                k += (IN(where) & 0x7f);
            if (k > 64)
                break;
            /* past the input everything reads 0: upstream would never leave
             * this loop (it ran off its buffer and crashed), so stop here */
            if (where - 4 >= c->in_size)
                break;
        } while ((IN(where - 4) & 0x80) != 0x80); /* pattern break it seems */
        pw_fwrite(out, &whatever[0], 1024);
    }
    where -= 4;
    /* where is now on the first smp */

    /* sample data */
    for (i = 0; i < 15; i++)
    {
        if (smp_addresses[i] != 0)
            pw_fwrite_in(out, (int32_t)((uint32_t)c->start + (uint32_t)smp_addresses[i]), sample_sizes[i]);
    }

    /* crap */
    pw_crap15("  Titanics Player ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
