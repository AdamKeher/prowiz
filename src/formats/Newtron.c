/*
 * Newtron.c - Newtron packer (upstream r/Newtron.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_Newtron(pw_ctx *c)
{
    /* test #1 */
    if ((c->i < 7) || ((c->i + 373 + 1024 + 2) > (uint32_t)c->in_size))
        return PW_BAD;

    /* test #1.5 */
    if (IN(c->i - 6) != 0x00)
        return PW_BAD;

    /* test #2 */
    c->start = (int32_t)(c->i - 7);
    c->l = 0;
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j += 1)
    {
        /* size */
        c->k = (((IN(c->start + 4 + 8 * c->j) * 256) + IN(c->start + 5 + 8 * c->j)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 8 + 8 * c->j) * 256) + IN(c->start + 9 + 8 * c->j)) * 2);
        /* loop size */
        c->n = (((IN(c->start + 10 + 8 * c->j) * 256) + IN(c->start + 11 + 8 * c->j)) * 2);
        c->wss += c->k;

        if (pw_test_smps((int32_t)c->k, (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 7 + 8 * c->j), IN(c->start + 6 + 8 * c->j)) == PW_BAD)
            return PW_BAD;
    }

    if (c->wss <= 2)
        return PW_BAD;

    /* test #4 */
    c->l = IN(c->start);
    if ((c->l > 0x7f) || (c->l == 0x00))
        return PW_BAD;

    /* test #5 */
    /* c->l contains the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 252 + c->j) > c->k)
            c->k = IN(c->start + 252 + c->j);
        if (IN(c->start + 252 + c->j) > 0x7f)
            return PW_BAD;
    }
    c->k += 1;

    /* #6 */
    if (((c->k * 1024) + 380) != (uint32_t)((IN(c->start + 2) * 256) + IN(c->start + 3) + 4))
        return PW_BAD;

    /* test #7  ptk notes .. gosh ! (testing all patterns !) */
    /* c->k contains the number of pattern saved */
    /* c->wss is the whole sample size */
    for (c->j = 0; c->j < (256 * c->k); c->j++)
    {
        c->l = IN(c->start + 380 + c->j * 4);
        if (c->l > 19) /* 0x13 */
            return PW_BAD;
        c->m = IN(c->start + 380 + c->j * 4) & 0x0f;
        c->m *= 256;
        c->m += IN(c->start + 381 + c->j * 4);
        if ((c->m > 0) && (c->m < 0x71))
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_Newtron(pw_ctx *c)
{
    /* c->wss is the whole sample size :) */
    c->k = (IN(c->start + 2) * 256) + IN(c->start + 3);

    c->out_size = c->k + c->wss + 4;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Newtron module", PWF_Newtron);

    if (c->save_status == PW_GOOD)
        c->i += 7;
}

/*
 *   newtron.c   2003-2007 (c) Asle / ReDoX
 *
 * Converts Newtron packed MODs back to PTK MODs
 *
 * Last update: 09 mar 2003
 * fixed again : 21 jan 2007
 */
void pw_depack_Newtron(pw_ctx *c)
{
    uint8_t  whatever[64];
    int32_t  i = 0;
    int32_t  total_sample_size = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(whatever, 0, sizeof whatever);

    /* title */
    pw_fwrite(out, whatever, 20);

    where += 4;

    for (i = 0; i < 31; i++)
    {
        /* sample name */
        pw_fwrite(out, whatever, 22);

        total_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);
        pw_fwrite_in(out, where, 8);
        where += 8;
    }

    /* pattern table lenght & Ntk byte */
    pw_fwrite_in(out, c->start, 1);
    whatever[0] = 0x7f;
    pw_fwrite(out, &whatever[0], 1);

    whatever[32] = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(where + i) > whatever[32])
            whatever[32] = IN(where + i);
    }
    pw_fwrite_in(out, where, 128);
    where += 128;

    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* pattern data */
    i = (whatever[32] + 1) * 1024;
    pw_fwrite_in(out, where, i);
    where += i;

    /* sample data */
    pw_fwrite_in(out, where, total_sample_size);

    pw_crap("      Newtron     ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
