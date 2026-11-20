/*
 * NewtronOld.c - Newtron packer, old version (upstream r/NewtronOld.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 *
 * Newtron Old format, created by ?!?, analysed by Sylvain "Asle" Chipaux.
 * Source: Little Joe & Newtron Musicdisk.
 *
 * Offset    size (byte)    Comment
 * ------    -----------    -------
 *  0             2         patternlist address (-8) [A]
 *  2             2         size of patternlist [B]
 *  4             4         ?
 *
 *  then [A]/8 sample descriptions of 8 bytes each:
 *  8             2         Sample Size / 2
 *  10            1         Finetune (0 -> F)
 *  11            1         Volume (0 - 40h)
 *  12            2         Loop Start / 2
 *  14            2         Loop Size / 2
 *
 * [A]           [B]        Pattern table
 * [A]+[B]        ?         Pattern datas, stored like Protracker
 *                          (one pattern is 1024 ($400) bytes).
 *
 * Follow the sample datas stored like Protracker. Nothing is packed.
 */
#include "formats.h"

/*
 * additional tests - 20100207
 */
int16_t pw_test_NewtronOld(pw_ctx *c)
{
    /* test #1 */
    if ((c->i < 11) || ((c->i + 6 + 1 + 1024 + 2) > (uint32_t)c->in_size))
        return PW_BAD;

    /* test #1.5 */
    c->start = (int32_t)(c->i - 11);
    if (IN(c->start + 2) != 0x00)
        return PW_BAD;

    c->l = (IN(c->start) * 256) + IN(c->start + 1) + 8;
    /* test #2 samples */
    c->wss = 0;
    c->l = (c->l / 8) - 1;
    for (c->j = 0; c->j < c->l; c->j += 1)
    {
        /* size */
        c->k = (((IN(c->start + 8 + 8 * c->j) * 256) + IN(c->start + 9 + 8 * c->j)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 12 + 8 * c->j) * 256) + IN(c->start + 13 + 8 * c->j)) * 2);
        /* loop size */
        c->n = (((IN(c->start + 14 + 8 * c->j) * 256) + IN(c->start + 15 + 8 * c->j)) * 2);
        c->wss += c->k;

        if (pw_test_smps((int32_t)c->k, (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 11 + 8 * c->j), IN(c->start + 10 + 8 * c->j)) == PW_BAD)
            return PW_BAD;
        if ((c->k == 0) && (c->m == 0) && (c->n == 0))
            return PW_BAD;
    }

    if (c->wss <= 2)
        return PW_BAD;

    /* test #4 - patternlist size */
    c->l = IN(c->start + 3);
    if ((c->l > 0x7f) || (c->l == 0x00))
        return PW_BAD;

    /* test #5 */
    /* c->l is the size of the patternlist */
    c->k = (IN(c->start) * 256) + IN(c->start + 1) + 8;
    /* c->k is the patternlist addy */
    if (((c->k / 8) * 8) != c->k)
        return PW_BAD;

    c->m = 0;
    for (c->j = 0; c->j < c->l; c->j++)
    {
        if (IN(c->start + c->k + c->j) > c->m)
            c->m = IN(c->start + c->k + c->j);
        if (IN(c->start + c->k + c->j) > 0x7f)
            return PW_BAD;
    }
    c->m += 1;

    /* #6 */
    /* c->l is the size of the patternlist */
    /* c->m contains the number of pattern saved */
    if (((c->m * 1024) + c->k + c->l + c->start) > (uint32_t)c->in_size)
        return PW_BAD;

    /* test #7  ptk notes .. gosh ! (testing all patterns !) */
    /* c->k is the patternlist addy */
    /* c->l is the size of the patternlist */
    /* c->m contains the number of pattern saved */
    /* c->wss is the whole sample size */
    c->k += c->l;
    /* c->k is now the pat data addy */
    for (c->j = 0; c->j < (256 * c->m); c->j++)
    {
        uint8_t b = IN(c->start + c->k + (c->j * 4));
        if ((b & 0x0f) > 0x03)
            return PW_BAD;
        if ((b & 0xf0) > 0x10)
            return PW_BAD;
        c->n = IN(c->start + c->k + c->j * 4) & 0x0f;
        c->n *= 256;
        c->n += IN(c->start + c->k + 1 + c->j * 4);
        if ((c->n > 0) && (c->n < 0x71))
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_NewtronOld(pw_ctx *c)
{
    /* c->wss is the whole sample size :) */
    /* c->m contains the number of pattern saved */
    /* c->k is the pat data addy */
    c->out_size = c->k + c->wss + (c->m * 1024);

    c->convert = PW_GOOD;
    pw_save_rip(c, "Newtron module Old", PWF_NewtronOld);

    if (c->save_status == PW_GOOD)
        c->i += 11;
}

/*
 *   newtronold.c   2007 (c) Asle / ReDoX
 *
 * Converts Newtron Old packed MODs back to PTK MODs
 *
 * clean up - 20100207
 */

/* Whatever[] of the original is a 1085 byte heap block; more than 31 sample
 * descriptions write past its end (dropped here, never read back). */
#define NEWTRONOLD_HDR 1085
#define HDR_SET(x, v)                                  \
    do                                                 \
    {                                                  \
        int32_t hx_ = (x);                             \
        if (hx_ >= 0 && hx_ < NEWTRONOLD_HDR)          \
            whatever[hx_] = (uint8_t)(v);              \
    } while (0)

void pw_depack_NewtronOld(pw_ctx *c)
{
    uint8_t  whatever[NEWTRONOLD_HDR];
    int32_t  i = 0, j = 0;
    int32_t  total_sample_size = 0;
    int32_t  where = c->start;
    int32_t  patlistaddy = 0;
    uint8_t  patsize = 0;
    uint8_t  max = 0x00;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(whatever, 0, sizeof whatever);

    /* size of header */
    patlistaddy = (IN(where) * 256) + IN(where + 1) + 8;
    /* nbr of samples */
    j = (patlistaddy / 8) - 1;

    where += 8;

    for (i = 0; i < j; i++)
    {
        HDR_SET(20 + 30 * i + 22, IN(where));
        HDR_SET(20 + 30 * i + 23, IN(where + 1));
        HDR_SET(20 + 30 * i + 24, IN(where + 2));
        HDR_SET(20 + 30 * i + 25, IN(where + 3));
        HDR_SET(20 + 30 * i + 26, IN(where + 4));
        HDR_SET(20 + 30 * i + 27, IN(where + 5));
        HDR_SET(20 + 30 * i + 28, IN(where + 6));
        HDR_SET(20 + 30 * i + 29, IN(where + 7));
        total_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);
        where += 8;
    }
    while (i < 31)
    {
        whatever[20 + 30 * i + 29] = 0x01;
        i++;
    }

    /* pattern table lenght & Ntk byte */
    patsize = IN(c->start + 3);
    whatever[950] = patsize;
    whatever[951] = 0x7f;

    where = patlistaddy + c->start;
    for (i = 0; i < patsize; i++)
    {
        if (IN(where + i) > max)
            max = IN(where + i);
        HDR_SET(952 + i, IN(where + i));
    }
    where += patsize;

    whatever[1080] = 'M';
    whatever[1081] = '.';
    whatever[1082] = 'K';
    whatever[1083] = '.';
    pw_fwrite(out, whatever, 1084);

    /* pattern data */
    i = (max + 1) * 1024;
    pw_fwrite_in(out, where, i);
    where += i;

    /* sample data */
    pw_fwrite_in(out, where, total_sample_size);

    pw_crap("    Newtron old   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
