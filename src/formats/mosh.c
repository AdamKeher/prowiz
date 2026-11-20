/*
 * mosh.c - Mosh Packer, tested after an "M.K." ID (upstream r/mosh.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_MOSH(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 378)
        return PW_BAD;

    c->start = (int32_t)(c->i - 378);

    /* samples */
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        /* size */
        c->j = (((IN(c->start + 6 + c->k * 8) * 256) + IN(c->start + 7 + c->k * 8)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 2 + c->k * 8) * 256) + IN(c->start + 3 + c->k * 8)) * 2);
        /* loop size */
        c->n = (((IN(c->start + c->k * 8) * 256) + IN(c->start + 1 + c->k * 8)) * 2);

        if (pw_test_smps((int32_t)(c->j * 2), (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 5 + 8 * c->k), IN(c->start + 4 + 8 * c->k)) == PW_BAD)
            return PW_BAD;

        c->wss += c->j;
    }

    /* test #4  pattern list size */
    c->l = IN(c->start + 248);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;
    /* c->l holds the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 250 + c->j) > c->k)
            c->k = IN(c->start + 250 + c->j);
        if (IN(c->start + 250 + c->j) > 127)
            return PW_BAD;
    }
    /* c->k is the number of pattern in the file (-1) */
    c->k += 1;

    /* test #5 pattern data ... */
    if (((c->k * 1024) + 382 + c->start) > (uint32_t)c->in_size)
        return PW_BAD;
    for (c->j = 0; c->j < (c->k * 256); c->j++)
    {
        /* sample > 1f   or   pitch > 358 ? */
        if (IN(c->start + 382 + c->j * 4) > 0x13)
            return PW_BAD;
        c->l = ((IN(c->start + 382 + c->j * 4) & 0x0f) * 256) + IN(c->start + 383 + c->j * 4);
        if ((c->l > 0) && (c->l < 0x1C))
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_MOSH(pw_ctx *c)
{
    /* c->k is still the number of patterns */

    c->out_size = (c->k * 1024) + 382 + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "MOSH packed module", PWF_MOSH);

    if (c->save_status == PW_GOOD)
        c->i += 379; /* after the 'M' of M.K. */
}

/*
 *   MOSH.c   2007 (c) Asle
 */
void pw_depack_MOSH(pw_ctx *c)
{
    uint8_t  whatever[1024];
    uint8_t  max = 0x00;
    int32_t  whole_sample_size = 0;
    int32_t  i = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(whatever, 0, sizeof whatever);
    /* title */
    pw_fwrite(out, whatever, 20);

    for (i = 0; i < 31; i++)
    {
        /* loop size */
        whatever[i * 30 + 28] = IN(where);
        whatever[i * 30 + 29] = IN(where + 1);
        /* loop start */
        whatever[i * 30 + 26] = IN(where + 2);
        whatever[i * 30 + 27] = IN(where + 3);
        /* fine */
        whatever[i * 30 + 24] = IN(where + 4);
        /* volume */
        whatever[i * 30 + 25] = IN(where + 5);
        /* size */
        whatever[i * 30 + 22] = IN(where + 6);
        whatever[i * 30 + 23] = IN(where + 7);
        whole_sample_size += (((IN(where + 6) * 256) + IN(where + 7)) * 2);

        where += 8;
    }
    pw_fwrite(out, whatever, 930);

    /* write until after M.K. */
    pw_fwrite_in(out, where, 134);
    where += 2; /* to be on at the patternlist level */

    /* getting highest pattern */
    max = 0x00;
    for (i = 0; i < 128; i++)
    {
        max = (IN(where) > max) ? IN(where) : max;
        where += 1;
    }
    max += 1;
    where += 4; /* M.K. */
    pw_fwrite_in(out, where, (max * 1024));
    where += (max * 1024); /* pattern data */

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap("    Mosh Player   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
