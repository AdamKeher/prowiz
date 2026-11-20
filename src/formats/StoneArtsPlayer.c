/*
 * StoneArtsPlayer.c - Stone Arts Player ("FAST" ID) (upstream r/StoneArtsPlayer.c).
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

int16_t pw_test_StoneArtsPlayer(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 - ntk byte */
    c->start = (int32_t)(c->i - 1080);
    if ((IN(c->start + 951) != 0x7f) && (IN(c->start + 951) != 0x00))
        return PW_BAD;

    /* test 3 - patternlist size */
    if (IN(c->start + 950) > 0x7f)
        return PW_BAD;

    for (c->k = 0; c->k < 31; c->k++)
    {
        /* size */
        c->j = (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2);
        /* loop size */
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);

        if (pw_test_smps((int32_t)(c->j * 2), (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 45 + 30 * c->k), IN(c->start + 44 + 30 * c->k)) == PW_BAD)
            return PW_BAD;

        /* upstream never resets c->wss here: it keeps adding to the old value */
        c->wss += c->j;
    }

    return PW_GOOD;
}

void pw_rip_StoneArtsPlayer(pw_ctx *c)
{
    /* c->wss is already the whole sample size */
    c->l = 0;
    for (c->k = 0; c->k < 128; c->k++)
        if (IN(c->start + 952 + c->k) > c->l)
            c->l = IN(c->start + 952 + c->k);
    c->l += 1;
    c->out_size = (c->l * 1024) + 1084 + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Stone Arts Player module", PWF_StoneArtsPlayer);

    if (c->save_status == PW_GOOD)
        c->i += 1082; /* 1080 could be enough */
}

/*
 *   StoneArtsPlayer.c   2007 (c) Asle
 *
 * Converts MODs converted with Stone Arts Player
 *
 * example module provided by Muerto ;)
 */
void pw_depack_StoneArtsPlayer(pw_ctx *c)
{
    uint8_t  whatever[4];
    uint8_t  poss[37][2];
    uint8_t  max = 0x00;
    int32_t  whole_sample_size = 0;
    int32_t  i = 0, j = 0;
    int32_t  where = c->start;
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* read and write whole header */
    pw_fwrite_in(out, where, 950);

    /* get whole sample size */
    for (i = 0; i < 31; i++)
        whole_sample_size += (((IN(where + 42 + i * 30) * 256) + IN(where + 43 + i * 30)) * 2);

    /* read and write size of pattern list */
    /* read and write ntk byte and pattern list */
    pw_fwrite_in(out, where + 950, 130);
    where += 952;

    /* write ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* get number of pattern */
    max = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(where + i) > max)
            max = IN(where + i);
    }

    /* pattern data */
    where = c->start + 1084;
    for (i = 0; i <= max; i++)
    {
        for (j = 0; j < 256; j++)
        {
            whatever[0] = IN(where);
            whatever[2] = IN(where + 2);
            whatever[3] = IN(where + 3);
            whatever[0] |= poss_at(poss, IN(where + 1) / 2, 0);
            whatever[1] = poss_at(poss, IN(where + 1) / 2, 1);
            pw_fwrite(out, whatever, 4);
            where += 4;
        }
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap("Stone Arts Player ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
