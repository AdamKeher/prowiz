/*
 * skizzo.c - GnoiPacker by Tax/Gnu Design ("-GD-"), first called Skizzo
 *            (upstream r/skizzo.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_Skizzo(pw_ctx *c)
{
    /* test #1 */
    c->start = (int32_t)(c->i - 24);
    if ((c->i + 38) > (uint32_t)c->in_size)
        return PW_BAD;
    c->j = IN(c->start + 22); /* nbr of samples */
    if ((c->j > 31) || (c->j == 0))
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_Skizzo(pw_ctx *c)
{
    /* c->j is the nbr of samples */
    /* get nbr of stored patts */
    c->k = IN(c->start + 23);
    /* get size of patlist */
    c->m = IN(c->start + 20) & 0x7f;
    c->wss = 0;
    for (c->l = 0; c->l < c->j; c->l++)
        c->wss += ((IN(c->start + 28 + (c->l * 8)) * 256) + IN(c->start + 29 + (c->l * 8))) * 2;

    c->out_size = c->wss + (c->j * 8) + (c->k * 768) + c->m + 28;

    c->convert = PW_GOOD;
    pw_save_rip(c, "GnoiPacker Packed music", PWF_GnoiPacker);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}

/*
 *   Skizzo.c   2007 (c) Asle / ReDoX
 *
 * 20070525 : doesn't convert pattern data
 * ???????? : pattern data converted
 * 20121022 : real player name : GnoiPacker by Tax/Gnu Design (1991)
 */
void pw_depack_Skizzo(pw_ctx *c)
{
    uint8_t  whatever[2048];
    int32_t  i = 0, k = 0;
    int16_t  nbr_sample, siz_patlist, nbr_patstored;
    int32_t  total_sample_size = 0;
    int32_t  where = c->start;
    uint8_t  poss[37][2];
    uint8_t  note;
    pw_file *out;

    /* filling up the possible PTK notes */
    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* title */
    memset(whatever, 0, sizeof whatever);
    /* write title */
    pw_fwrite_in(out, where, 20);
    where += 20;

    /* read size of patternlist */
    siz_patlist = IN(where); /* &0x7F necessary ! */
    where += 2; /* with bypassing an unknown '00' */

    /* read number of sample (at most 31: checked by the test) */
    nbr_sample = IN(where);
    where += 1;

    /* read nbr of pattern saved */
    nbr_patstored = IN(where);
    where += 5; /* bypassing '-GD-' too */

    /* sample header */
    memset(whatever, 0, sizeof whatever);
    for (i = 0; i < nbr_sample; i++)
    {
        /* size */
        whatever[22 + (i * 30)] = IN(where);
        whatever[23 + (i * 30)] = IN(where + 1);
        total_sample_size += ((IN(where) * 256) + IN(where + 1)) * 2;
        /* finetune */
        whatever[24 + (i * 30)] = IN(where + 2);
        /* volume */
        whatever[25 + (i * 30)] = IN(where + 3);
        /* loop start */
        whatever[26 + (i * 30)] = IN(where + 4);
        whatever[27 + (i * 30)] = IN(where + 5);
        /* loop size */
        whatever[28 + (i * 30)] = IN(where + 6);
        whatever[29 + (i * 30)] = IN(where + 7);
        where += 8;
    }
    /* finally, write the PTK header */
    pw_fwrite(out, whatever, 930);

    /* pattern list now */
    /* 'where' is on it */
    memset(whatever, 0, sizeof whatever);
    for (i = 2; i < (siz_patlist & 0x7f) + 2; i++)
    {
        whatever[i] = IN(where);
        where += 1;
    }
    whatever[i + 1] = (uint8_t)(nbr_patstored - 1);
    whatever[0] = (uint8_t)(siz_patlist & 0x7f);
    whatever[1] = 0x7F;
    pw_fwrite(out, whatever, 130);

    /* PTK's tag now */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* pattern data now ... */
    for (i = 0; i < nbr_patstored; i++)
    {
        for (k = 0; k < 256; k++) /* loop on each note for this pattern */
        {
            /* poss[] has 37 entries; the note byte can reach 127 */
            note = (uint8_t)(IN(where) & 0x7f);
            whatever[k * 4] = (note < 37) ? poss[note][0] : 0;
            whatever[k * 4 + 1] = (note < 37) ? poss[note][1] : 0;
            whatever[k * 4 + 2] = IN(where + 1);
            whatever[k * 4 + 3] = IN(where + 2);
            where += 3;
        }
        pw_fwrite(out, whatever, 1024);
    }

    pw_fwrite_in(out, where, total_sample_size);

    pw_crap("    GnoiPacker    ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
