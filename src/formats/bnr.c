/*
 * bnr.c - Binary Packer, "BNR!" at the Protracker ID position (upstream r/bnr.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_BNR(pw_ctx *c)
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
        /* loop size */
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2);
        /* loop start */
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);

        if (pw_test_smps((int32_t)(c->j * 2), (int32_t)c->n, (int32_t)c->m,
                         IN(c->start + 45 + 30 * c->k), IN(c->start + 44 + 30 * c->k)) == PW_BAD)
            return PW_BAD;

        c->wss += c->j;
    }

    /* test #4  pattern list size */
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
    /* test last patterns of the pattern list = 0 ? */
    c->j += 2; /* found some obscure ptk :( */
    while (c->j < 128)
    {
        if (IN(c->start + 952 + c->j) > 0x7f)
            return PW_BAD;
        c->j += 1;
    }
    /* c->k is the number of pattern in the file (-1) */
    c->k += 1;

    /* test #5 pattern data ... */
    if (((c->k * 1024) + 1084 + c->start) > (uint32_t)c->in_size)
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_BNR(pw_ctx *c)
{
    /* c->wss still holds the whole sample size */
    /* c->k is still the number of pattern stored */

    c->out_size = c->wss + (c->k * 1024) + 1084;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Binary Packer module", PWF_BNR);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}

/*
 *   bnr.c   1999 (c) Asle / ReDoX
 * 20071223 - added into PW at Muerto's request
 */
void pw_depack_BNR(pw_ctx *c)
{
    uint8_t  c1, c2;
    uint8_t  whatever[1084];
    uint8_t  nbr_pat = 0;
    uint8_t  poss[37][2];
    int32_t  whole_sample_size = 0;
    int32_t  i, j, note;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    pw_fill_ptk_table(poss);

    /* read header */
    memset(whatever, 0, sizeof whatever);
    for (i = 0; i < 1084; i++)
        whatever[i] = IN(i + where);

    /* reorder loops and sample size */
    for (i = 0; i < 31; i++)
    {
        c1 = whatever[i * 30 + 20 + 26];
        c2 = whatever[i * 30 + 20 + 27];
        whatever[i * 30 + 20 + 26] = whatever[i * 30 + 20 + 28];
        whatever[i * 30 + 20 + 27] = whatever[i * 30 + 20 + 29];
        whatever[i * 30 + 20 + 28] = c1;
        whatever[i * 30 + 20 + 29] = c2;
        whole_sample_size += (((whatever[i * 30 + 20 + 22] * 256) + whatever[i * 30 + 20 + 23]) * 2);
    }

    /* M.K. ... */
    whatever[1080] = 'M';
    whatever[1081] = '.';
    whatever[1082] = 'K';
    whatever[1083] = '.';

    /* write header */
    pw_fwrite(out, whatever, 1084);
    where += 1084;

    /* get number of patterns */
    for (i = 952; i < 1080; i++)
    {
        if (whatever[i] > nbr_pat)
            nbr_pat = whatever[i];
    }
    nbr_pat += 1;

    /* pattern data */
    for (i = 0; i < nbr_pat; i++)
    {
        memset(whatever, 0, sizeof whatever);
        for (j = 0; j < 256; j++)
        {
            /* Fx Arg */
            whatever[j * 4 + 3] = IN(where + i * 1024 + j * 4 + 2);
            /* Fx */
            whatever[j * 4 + 2] = (uint8_t)((IN(where + i * 1024 + j * 4) >> 2) & 0x0f);
            /* Smp Number */
            whatever[j * 4 + 2] |= (uint8_t)((IN(where + i * 1024 + j * 4 + 3) << 1) & 0xf0);
            whatever[j * 4] = (uint8_t)((IN(where + i * 1024 + j * 4 + 3) >> 3) & 0xf0);

            /* test flag #2 */
            if ((IN(where + i * 1024 + j * 4) & 0x40) == 0x40)
                continue;
            /* test flag #1 */
            if ((IN(where + i * 1024 + j * 4) & 0x80) == 0x80)
                whatever[j * 4 + 2] |= 0x0f; /* forcing set BPM */

            /* notes ... poss[] has 37 entries; the index runs from -90 to 37 */
            note = 37 - (IN(where + i * 1024 + j * 4 + 1) >> 1);
            whatever[j * 4] |= (uint8_t)(((note >= 0 && note < 37) ? poss[note][0] : 0) & 0x0f);
            whatever[j * 4 + 1] = (note >= 0 && note < 37) ? poss[note][1] : 0;
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    where += (nbr_pat * 1024);
    pw_fwrite_in(out, where, whole_sample_size);

    /* no crap() as it overwrites sample text */

    pw_fclose(out);

    pw_printf(c, "done\n");
}
