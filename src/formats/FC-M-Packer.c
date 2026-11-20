/*
 * FC-M-Packer.c - FC-M Packer (upstream r/FC-M-Packer.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_FC_M(pw_ctx *c)
{
    /* test 1 */
    c->start = (int32_t)c->i;
    if (IN(c->start + 4) != 0x01)
        return PW_BAD;

    /* test 2 */
    if (IN(c->start + 5) != 0x00)
        return PW_BAD;

    /* test 3 */
    for (c->j = 0; c->j < 31; c->j++)
    {
        if (IN(c->start + 37 + 8 * c->j) > 0x40)
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_FC_M(pw_ctx *c)
{
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
        c->wss += (((IN(c->start + 34 + c->k * 8) * 256) + IN(c->start + 35 + c->k * 8)) * 2);
    c->j = IN(c->start + 286);
    c->l = 0;
    for (c->k = 0; c->k < c->j; c->k++)
    {
        if (IN(c->start + 292 + c->k) > c->l)
            c->l = IN(c->start + 292 + c->k);
    }
    c->l += 1;
    c->out_size = (c->l * 1024) + c->wss + 292 + c->j + 8;

    c->convert = PW_GOOD;
    pw_save_rip(c, "FC-M packed module", PWF_FC_M_packer);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1); /* 0 should do but call it "just to be sure" :) */
}

/*
 *   FC-M_Packer.c   1997 (c) Asle / ReDoX
 *
 * Converts back to ptk FC-M packed MODs
 */
void pw_depack_FC_M(pw_ctx *c)
{
    uint8_t  whatever[256];
    int32_t  i = 0;
    int32_t  whole_sample_size = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* bypass "FC-M" ID */
    /* bypass what looks like the version number .. */
    /* bypass "NAME" chunk */

    /* read and write title */
    pw_fwrite_in(out, where + 10, 20);
    /* bypass "INST" chunk */
    where += 34;

    /* read and write sample descriptions */
    memset(whatever, 0, sizeof whatever);
    for (i = 0; i < 31; i++)
    {
        /*sample name*/
        pw_fwrite(out, whatever, 22);

        whole_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);
        pw_fwrite_in(out, where, 6);
        whatever[32] = IN(where + 7);
        if ((IN(where + 6) == 0x00) && (IN(where + 7) == 0x00))
            whatever[32] = 0x01;
        pw_fwrite_in(out, where + 6, 1);
        pw_fwrite(out, &whatever[32], 1);
        where += 8;
    }

    /* bypass "LONG" chunk */
    where += 4;

    /* read and write pattern table lenght */
    whatever[128] = IN(where++);
    pw_fwrite(out, &whatever[128], 1);

    /* read and write NoiseTracker byte */
    pw_fwrite_in(out, where, 1);

    /* bypass "PATT" chunk */
    where += 5;

    /* read and write pattern list and get highest patt number
     * (a list longer than 128 overwrites its own length and maximum in
     * whatever[128] and [129], as in the original) */
    for (i = 0; i < whatever[128]; i++)
    {
        whatever[i] = IN(where);
        if (IN(where) > whatever[129])
            whatever[129] = IN(where);
        where += 1;
    }
    pw_fwrite(out, whatever, 128);

    /* write ptk's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* bypass "SONG" chunk */
    where += 4;

    /* pattern data */
    for (i = 0; i <= whatever[129]; i++)
    {
        pw_fwrite_in(out, where, 1024);
        where += 1024;
    }

    /* bypass "SAMP" chunk */
    where += 4;

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap("   FC-M packer    ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
