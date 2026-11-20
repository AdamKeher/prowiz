/*
 * NovoTrade.c - NovoTrade Packer (upstream r/NovoTrade.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 *
 * (5th of may 2007)
 * (1st of January 2010 - completed)
 */
#include "formats.h"

int16_t pw_test_NovoTrade(pw_ctx *c)
{
    /* test #1 */
    c->start = (int32_t)c->i;
    if ((c->i + 38) > (uint32_t)c->in_size)
        return PW_BAD;
    c->j = (IN(c->start + 20) * 256) + IN(c->start + 21) + 4;
    if ((c->i + c->j + 4) > (uint32_t)c->in_size)
        return PW_BAD;
    c->k = (IN(c->start + 28) * 256) + IN(c->start + 29) + c->j + 4;
    if ((c->i + c->k + 2) > (uint32_t)c->in_size)
        return PW_BAD;
    /* c->j is on "BODY" tag */
    /* c->k is on "SAMP" tag */

    /* test #2 let's verify (the original tests with && ...) */
    if ((IN(c->start + c->j) != 'B') &&
        (IN(c->start + c->j + 1) != 'O') &&
        (IN(c->start + c->j + 2) != 'D') &&
        (IN(c->start + c->j + 3) != 'Y'))
        return PW_BAD;
    if ((IN(c->start + c->k) != 'S') &&
        (IN(c->start + c->k + 1) != 'A') &&
        (IN(c->start + c->k + 2) != 'M') &&
        (IN(c->start + c->k + 3) != 'P'))
        return PW_BAD;

    /* no much but should be enough :) */
    return PW_GOOD;
}

void pw_rip_NovoTrade(pw_ctx *c)
{
    /* get nbr sample */
    c->j = (IN(c->start + 22) * 256) + IN(c->start + 23);
    /* get BODY addy */
    c->m = (IN(c->start + 20) * 256) + IN(c->start + 21) + 4;
    /* get SAMP addy */
    c->k = (IN(c->start + 28) * 256) + IN(c->start + 29) + c->m + 4;
    c->wss = 0;
    /* (the original reads the sample sizes from the start of the input,
     * not from the start of the module) */
    for (c->l = 0; c->l < c->j; c->l++)
        c->wss += ((IN(32 + (c->l * 8)) * 256) + IN(33 + (c->l * 8))) * 2;

    c->out_size = c->wss + c->k + 4;

    c->convert = PW_GOOD;
    pw_save_rip(c, "NovoTrade Packed music", PWF_NovoTrade);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}

/*
 *   NovoTrade.c   2007-2010 (c) Sylvain "Asle" Chipaux
 *
 * 20070505 : doesn't convert pattern data
 * 20100101 : Thanks to Claudio of XMP team. It's ok now.
 */

/* Whatever[] of the original is a 2048 byte heap block that sample numbers
 * and pattern list sizes from the file can push writes past; those bytes are
 * never read back, so the writes are dropped. */
#define NOVO_BUF 2048
#define BUF_SET(x, v)                                  \
    do                                                 \
    {                                                  \
        int32_t bx_ = (x);                             \
        if (bx_ >= 0 && bx_ < NOVO_BUF)                \
            whatever[bx_] = (uint8_t)(v);              \
    } while (0)

void pw_depack_NovoTrade(pw_ctx *c)
{
    uint8_t  whatever[NOVO_BUF];
    uint8_t  c1;
    int32_t  i = 0, k = 0;
    uint16_t pattern_addresses_table[128];
    int16_t  body_addy, samp_addy, nbr_sample, siz_patlist, nbr_patstored;
    int32_t  total_sample_size = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    where += 4;
    /* title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite_in(out, where, 16);
    pw_fwrite(out, whatever, 4);
    where += 16;

    /* get 'BODY' addy */
    body_addy = (int16_t)((IN(where) * 256) + IN(where + 1) + 4);
    where += 2;

    /* number of sample */
    nbr_sample = (int16_t)((IN(where) * 256) + IN(where + 1));
    where += 2;

    /* size of the pattern list */
    siz_patlist = (int16_t)((IN(where) * 256) + IN(where + 1));
    where += 2;

    /* number of pattern stored */
    nbr_patstored = (int16_t)((IN(where) * 256) + IN(where + 1));
    where += 2;

    /* get 'SAMP' addy */
    samp_addy = (int16_t)((IN(where) * 256) + IN(where + 1) + body_addy + 4);
    where += 2;

    /* sample header */
    memset(whatever, 0, sizeof whatever);
    for (i = 0; i < nbr_sample; i++)
    {
        /* IN(where) is the sample ref */
        /* volume */
        BUF_SET(25 + (IN(where) * 30), IN(where + 1));
        /* size */
        BUF_SET(22 + (IN(where) * 30), IN(where + 2));
        BUF_SET(23 + (IN(where) * 30), IN(where + 3));
        total_sample_size += ((IN(where + 2) * 256) + IN(where + 3)) * 2;
        /* loop start */
        BUF_SET(26 + (IN(where) * 30), IN(where + 4));
        BUF_SET(27 + (IN(where) * 30), IN(where + 5));
        /* loop size */
        BUF_SET(28 + (IN(where) * 30), IN(where + 6));
        BUF_SET(29 + (IN(where) * 30), IN(where + 7));
        where += 8;
    }
    pw_fwrite(out, whatever, 930);

    /* pattern list now */
    /* where is on it */
    memset(whatever, 0, sizeof whatever);
    for (i = 2; i < siz_patlist + 2; i++)
    {
        BUF_SET(i, IN(where + 1));
        where += 2;
    }
    whatever[0] = (uint8_t)siz_patlist;
    whatever[1] = 0x7F;
    /* PTK's tag now */
    whatever[130] = 'M';
    whatever[131] = '.';
    whatever[132] = 'K';
    whatever[133] = '.';
    pw_fwrite(out, whatever, 134);

    /* pattern addresses now */
    /* where is on it */
    memset(pattern_addresses_table, 0, sizeof pattern_addresses_table);
    for (i = 0; i < nbr_patstored; i++)
    {
        /* the table has 128 entries; the count comes from the file */
        if (i < 128)
            pattern_addresses_table[i] = (uint16_t)((IN(where) * 256) + IN(where + 1));
        where += 2;
    }

    /* pattern data now ... *gee* */
    where += 4;
    for (i = 0; i < nbr_patstored; i++)
    {
        memset(whatever, 0, sizeof whatever);
        /* (relative to the start of the input, as in the original) */
        where = body_addy + 4 + ((i < 128) ? pattern_addresses_table[i] : 0);
        for (k = 0; k < 64; k++)
        {
            c1 = IN(where + 1);
            if (c1 == 0x80)
            {
                /* pattern ends */
                where += 2;
                k += 1;
                break;
            }
            if (c1 == 0x00)
            {
                /* empty line */
                where += 2;
                continue;
            }
            if (c1 > 0x0F)
            {
                /* bypass notes .. guess */
                where += 2;
                continue;
            }

            where += 2;
            if ((c1 & 0x01) == 0x01)
            {
                whatever[k * 16] = IN(where);
                whatever[k * 16 + 1] = IN(where + 1);
                whatever[k * 16 + 2] = IN(where + 2);
                whatever[k * 16 + 3] = IN(where + 3);
                where += 4;
            }
            if ((c1 & 0x02) == 0x02)
            {
                whatever[k * 16 + 4] = IN(where);
                whatever[k * 16 + 5] = IN(where + 1);
                whatever[k * 16 + 6] = IN(where + 2);
                whatever[k * 16 + 7] = IN(where + 3);
                where += 4;
            }
            if ((c1 & 0x04) == 0x04)
            {
                whatever[k * 16 + 8] = IN(where);
                whatever[k * 16 + 9] = IN(where + 1);
                whatever[k * 16 + 10] = IN(where + 2);
                whatever[k * 16 + 11] = IN(where + 3);
                where += 4;
            }
            if ((c1 & 0x08) == 0x08)
            {
                whatever[k * 16 + 12] = IN(where);
                whatever[k * 16 + 13] = IN(where + 1);
                whatever[k * 16 + 14] = IN(where + 2);
                whatever[k * 16 + 15] = IN(where + 3);
                where += 4;
            }
        }
        pw_fwrite(out, whatever, 1024);
    }

    where = c->start + samp_addy + 4;
    pw_fwrite_in(out, where, total_sample_size);

    pw_crap(" NovoTrade Packer ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
