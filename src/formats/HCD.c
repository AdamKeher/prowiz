/*
 * HCD.c - HCD-Protector, tested after an "M.K." ID (upstream r/HCD.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_HCD(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1080);
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        /* size */
        c->j = (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2) - 0x68A;
        /* loop size */
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);

        if (pw_test_smps((int32_t)(c->j * 2), (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 45 + 30 * c->k), IN(c->start + 44 + 30 * c->k)) == PW_BAD)
            return PW_BAD;

        c->wss += c->j;
    }

    /* test #3  pattern list size */
    c->l = IN(c->start + 950);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;
    /* c->l holds the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 952 + c->j) > c->k)
            c->k = IN(c->start + 952 + c->j);
        if (IN(c->start + 952 + c->j) > 127)
            return PW_BAD;
    }
    /* c->k holds the highest pattern number */
    c->k += 1;

    return PW_GOOD;
}

void pw_rip_HCD(pw_ctx *c)
{
    c->out_size = (c->k * 1024) + 1084 + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "HCD-Protector module", PWF_HCD);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}

/*
 *   HCD.c   2008 (c) Sylvain "Asle" Chipaux
 *
 * same header as PTK save for the sample loop start which is +0x0345
 * pattern data is <<3
 */
void pw_depack_HCD(pw_ctx *c)
{
    uint8_t  whatever[2048];
    uint8_t  max = 0x00;
    int32_t  whole_sample_size = 0;
    int32_t  i = 0, j = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* header */
    memset(whatever, 0, sizeof whatever);
    for (i = 0; i < 1084; i++)
        whatever[i] = IN(where + i);

    /* fix loop start */
    for (i = 46; i < 950; i += 30)
    {
        whatever[i] -= 0x03;
        whatever[i + 1] -= 0x45;
        whole_sample_size += (((IN(where + i - 4) * 256) + IN(where + i - 3)) * 2);
    }

    /* write header */
    pw_fwrite(out, whatever, 1084);

    /* get number of pattern */
    max = 0x00;
    for (i = 952; i < 1080; i++)
    {
        if (whatever[i] > max)
            max = whatever[i];
    }

    /* pattern data */
    where += 1084;
    for (i = 0; i <= max; i++)
    {
        for (j = 0; j < 256; j++)
        {
            whatever[j * 4] = (uint8_t)(IN(where) >> 3);
            whatever[j * 4 + 1] = (uint8_t)((IN(where + 1) >> 3) | (IN(where) << 5));
            whatever[j * 4 + 2] = (uint8_t)((IN(where + 2) >> 3) | (IN(where + 1) << 5));
            whatever[j * 4 + 3] = (uint8_t)((IN(where + 3) >> 3) | (IN(where + 2) << 5));
            where += 4;
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap("  HCD Protector   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
