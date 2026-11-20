/*
 * PowerMusic.c - Power Music ("!PM!" ID) (upstream r/PowerMusic.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 *
 * update on the 3rd of april 2000
 * some advices by Thomas Neumann .. thx
 */
#include "formats.h"

/* Power Music */
int16_t pw_test_PM(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1080);
    for (c->j = 0; c->j < 31; c->j++)
    {
        if (IN(c->start + 45 + 30 * c->j) > 0x40)
            return PW_BAD;
    }

    /* test 3 */
    if ((IN(c->start + 951) > 0x7F) && (IN(c->start + 951) != 0xFF))
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_PM(pw_ctx *c)
{
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
        c->wss += (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);

    c->j = IN(c->start + 950);
    c->l = 0;
    for (c->k = 0; c->k < 128; c->k++)
    {
        if (IN(c->start + 952 + c->k) > c->l)
            c->l = IN(c->start + 952 + c->k);
    }
    c->l += 1;
    c->k = 1084 + (c->l * 1024);
    c->out_size = c->k + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Power Music module", PWF_Power_Music);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1083); /* 1080 should do but call it "just to be sure" :) */
}

/*
 *   PowerMusic.c   1996 (c) Asle / ReDoX
 *
 * Converts back to ptk Optimod's power music files
 *
 * Last revision : 26/11/1999 by Sylvain "Asle" Chipaux
 *                 reduced to only one FREAD.
 *                 Speed-up and Binary smaller.
 * update: 01/12/99
 *   - removed fopen() and attached funcs
 */
void pw_depack_PM(pw_ctx *c)
{
    signed char *smp_data;
    uint8_t  c1 = 0x00, c2 = 0x00;
    uint8_t  max = 0x00;
    int32_t  whole_sample_size = 0;
    int32_t  i = 0, j;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* write whole header */
    pw_fwrite_in(out, where, 950);

    /* get whole sample size */
    for (i = 0; i < 31; i++)
        whole_sample_size += (((IN(where + 42 + i * 30) * 256) + IN(where + 43 + i * 30)) * 2);

    /* read and write size of pattern list */
    pw_fwrite_in(out, where + 950, 1);

    /* read and write ntk byte and pattern list */
    c1 = 0x7f;
    pw_fwrite(out, &c1, 1);
    pw_fwrite_in(out, where + 952, 128);

    /* write ID */
    c1 = 'M';
    c2 = '.';
    pw_fwrite(out, &c1, 1);
    pw_fwrite(out, &c2, 1);
    c1 = 'K';
    pw_fwrite(out, &c1, 1);
    pw_fwrite(out, &c2, 1);

    /* get number of pattern */
    max = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(where + i + 952) > max)
            max = IN(where + i + 952);
    }
    j = max += 1;

    /* pattern data */
    j *= 1024;
    pw_fwrite_in(out, where + 1084, j);
    j += 1084;

    /* sample data (delta packed) */
    /* at least one byte: the original stored Smp_Data[0] even for an empty
     * buffer */
    smp_data = (signed char *)calloc((size_t)whole_sample_size + 1, 1);
    if (smp_data == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        pw_fdiscard(out);
        return;
    }
    smp_data[0] = (signed char)IN(where + j);
    for (i = 1; i < whole_sample_size - 1; i++)
        smp_data[i] = (signed char)(smp_data[i - 1] + (signed char)IN(where + j + i));
    pw_fwrite(out, smp_data, whole_sample_size);
    free(smp_data);

    /* crap */
    pw_crap("   Power Music    ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
