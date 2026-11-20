/*
 * NoiseFromHeaven.c - Noise From Heaven chiptune diskmag modules ("Fuck" ID)
 *                     (upstream r/NoiseFromHeaven.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* Noise from Heaven Chipdisk (21 oct 2001) by Iris */
int16_t pw_test_NFH(pw_ctx *c)
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
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2);
        /* loop size */
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);

        if (pw_test_smps((int32_t)(c->j * 2), (int32_t)c->m, (int32_t)c->n,
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

/* noise from heaven chiptune mag by Iris'01 */
void pw_rip_NFH(pw_ctx *c)
{
    /* c->wss is still the whole sample size */
    c->l = 0;
    for (c->k = 0; c->k < 128; c->k++)
        if (IN(c->start + 952 + c->k) > c->l)
            c->l = IN(c->start + 952 + c->k);
    c->l += 1;
    c->out_size = (c->l * 1024) + 1084 + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Noise From Heaven module", PWF_NoiseFromHeaven);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1083); /* 1080 could be enough */
}

/*
 *   nfh.c   2003 (c) Asle / ReDoX
 *
 * converts ziks from Noise From Heaven chiptune diskmag by Iris'01
 */
void pw_depack_NFH(pw_ctx *c)
{
    uint8_t  whatever[4];
    uint8_t  poss[37][2];
    uint8_t  max = 0x00;
    uint8_t  note;
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
    pw_fwrite_in(out, where, 1080);

    /* get whole sample size */
    for (i = 0; i < 31; i++)
        whole_sample_size += (((IN(where + 42 + i * 30) * 256) + IN(where + 43 + i * 30)) * 2);

    where += 952; /* after size of pattern list .. before pattern list itself */

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
            whatever[0] = IN(where) & 0xf0;
            whatever[2] = (uint8_t)((IN(where) & 0x0f) << 4);
            whatever[2] |= (uint8_t)(IN(where + 2) / 2);
            whatever[3] = IN(where + 3);
            /* poss[] has 37 entries; the note byte / 2 can reach 127 */
            note = (uint8_t)(IN(where + 1) / 2);
            whatever[0] |= (note < 37) ? poss[note][0] : 0;
            whatever[1] = (note < 37) ? poss[note][1] : 0;
            pw_fwrite(out, whatever, 4);
            where += 4;
        }
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap("Noise From Heaven ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
