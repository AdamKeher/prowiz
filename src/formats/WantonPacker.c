/*
 * WantonPacker.c - Wanton Packer ("WN" ID) (upstream r/WantonPacker.c).
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

int16_t pw_test_WN(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1080);
    if (IN(c->start + 951) != 0x7f)
        return PW_BAD;

    /* test 3 */
    if (IN(c->start + 950) > 0x7f)
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_WN(pw_ctx *c)
{
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
        c->wss += (((IN(c->start + 42 + 30 * c->k) * 256) + IN(c->start + 43 + 30 * c->k)) * 2);
    c->j = IN(c->start + 1083);
    c->out_size = c->wss + (c->j * 1024) + 1084;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Wanton Packer module", PWF_Wanton_packer);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1082); /* 1081 should do but call it "just to be sure" :) */
}

/*
 *   Wanton_Packer.c   1997 (c) Asle / ReDoX
 *
 * Converts MODs converted with Wanton packer
 ********************************************************
 * 13 april 1999 : Update
 *   - no more open() of input file ... so no more fread() !.
 *     It speeds-up the process quite a bit :).
 * 28 nov 1999 : Update
 *   - small size optimizing.
 */
void pw_depack_WN(pw_ctx *c)
{
    uint8_t  poss[37][2];
    uint8_t  whatever[5];
    int32_t  whole_sample_size = 0;
    int32_t  i = 0, j = 0;
    int32_t  where = c->start; /* main pointer to prevent fread() */
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    memset(whatever, 0, sizeof whatever);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* read header */
    pw_fwrite_in(out, where, 950);

    /* get whole sample size */
    for (i = 0; i < 31; i++)
        whole_sample_size += (((IN(where + 42 + i * 30) * 256) + IN(where + 43 + i * 30)) * 2);

    /* read size of pattern list */
    where += 950;
    pw_fwrite_in(out, where, 1);
    where += 1;

    pw_fwrite_in(out, where, 129);
    where += 129;

    /* write ptk's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* get highest pattern number */
    whatever[4] = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(c->start + 952 + i) > whatever[4])
            whatever[4] = IN(c->start + 952 + i);
    }

    /* pattern data */
    where = c->start + 1084;
    for (i = 0; i <= whatever[4]; i++)
    {
        for (j = 0; j < 256; j++)
        {
            whatever[0] = IN(where + 1) & 0xf0;
            whatever[0] |= poss_at(poss, IN(where) / 2, 0);
            whatever[1] = poss_at(poss, IN(where) / 2, 1);
            whatever[2] = (uint8_t)((IN(where + 1) << 4) & 0xf0);
            whatever[2] |= IN(where + 2);
            whatever[3] = IN(where + 3);

            pw_fwrite(out, whatever, 4);
            where += 4;
        }
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap("  Wanton Packer   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
