/*
 * HornetPacker.c - Hornet Packer (upstream r/HornetPacker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_HRT(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1080);
    for (c->j = 0; c->j < 31; c->j++)
    {
        if (IN(45 + c->j * 30 + c->start) > 0x40)
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_HRT(pw_ctx *c)
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
    c->k = 1084 + c->l * 1024;
    c->out_size = c->k + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "HORNET packed module", PWF_Hornet_packer);

    if (c->save_status == PW_GOOD)
        c->i += 1084;
}

/*
 *   Hornet_Packer.c   1997 (c) Asle / ReDoX
 *
 * Converts MODs converted with Hornet packer
 */
void pw_depack_HRT(pw_ctx *c)
{
    uint8_t  whatever[1024];
    uint8_t  poss[37][2];
    uint8_t  max = 0x00;
    int32_t  whole_sample_size = 0;
    int32_t  i = 0, j = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    pw_fill_ptk_table(poss);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* read header */
    memset(whatever, 0, sizeof whatever);
    for (i = 0; i < 950; i++)
        whatever[i] = IN(where++);

    /* empty-ing those adresse values ... */
    for (i = 0; i < 31; i++)
    {
        whatever[38 + (30 * i)] = 0x00;
        whatever[38 + (30 * i) + 1] = 0x00;
        whatever[38 + (30 * i) + 2] = 0x00;
        whatever[38 + (30 * i) + 3] = 0x00;
    }

    /* write header */
    pw_fwrite(out, whatever, 950);

    /* get whole sample size */
    for (i = 0; i < 31; i++)
        whole_sample_size += (((whatever[42 + (30 * i)] * 256) + whatever[43 + 30 * i]) * 2);

    /* number of pattern */
    pw_fwrite_in(out, where++, 1);

    /* read noisetracker byte and pattern list
     * (the original writes the input byte at the absolute offset 256) */
    where += 1;
    whatever[256] = 0x7f;
    pw_fwrite_in(out, 256, 1);
    pw_fwrite_in(out, where, 128);

    /* get number of pattern */
    max = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(where + i) > max)
            max = IN(where + i);
    }

    /* write ptk's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* pattern data */
    where = c->start + 1084;
    for (i = 0; i <= max; i++)
    {
        for (j = 0; j < 256; j++)
        {
            whatever[0] = IN(where);
            whatever[1] = IN(where + 1);
            whatever[2] = IN(where + 2);
            whatever[3] = IN(where + 3);
            whatever[0] /= 2;
            whatever[16] = (uint8_t)(whatever[0] & 0xf0);
            if (whatever[1] == 0x00)
                whatever[17] = 0x00;
            else
            {
                /* poss[] has 37 entries; the index can reach 127 */
                whatever[16] |= ((whatever[1] / 2) < 37) ? poss[(whatever[1] / 2)][0] : 0;
                whatever[17] = ((whatever[1] / 2) < 37) ? poss[(whatever[1] / 2)][1] : 0;
            }
            whatever[18] = (uint8_t)((whatever[0] << 4) & 0xf0);
            whatever[18] |= whatever[2];
            whatever[19] = whatever[3];

            pw_fwrite(out, &whatever[16], 4);
            where += 4;
        }
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap("  Hornet Packer   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
