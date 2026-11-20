/*
 * UnicTracker2.c - UNIC Tracker v2 (upstream r/UnicTracker2.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* update on the 3rd of april 2000 */
/* bugs pointed out by Thomas Neumann .. thx :) */

int16_t pw_test_UNIC2(pw_ctx *c)
{
    uint32_t a;

    /* test 1 */
    if ((c->i < 25) || ((c->i + 1828) >= (uint32_t)c->in_size)) /* 1828=Head+1 pat */
        return PW_BAD;

    /* test #2 ID = $00000000 ? */
    c->start = (int32_t)(c->i - 25);
    if ((IN(c->start + 1060) == 00)
        && (IN(c->start + 1061) == 00)
        && (IN(c->start + 1062) == 00)
        && (IN(c->start + 1063) == 00))
        return PW_BAD;

    /* test 2,5 :) */
    c->o = 0;
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->j = ((IN(c->start + 22 + c->k * 30) * 256) + IN(c->start + 23 + c->k * 30));
        c->m = ((IN(c->start + 26 + c->k * 30) * 256) + IN(c->start + 27 + c->k * 30));
        c->n = ((IN(c->start + 28 + c->k * 30) * 256) + IN(c->start + 29 + c->k * 30));
        c->wss += (c->j * 2);
        if (pw_test_smps((int32_t)c->j, (int32_t)c->m, (int32_t)c->n, IN(c->start + 25 + c->k * 30), 0) == PW_BAD)
            return PW_BAD;
        if ((c->j > 0x7fff) ||
            (c->m > 0x7fff) ||
            (c->n > 0x7fff))
            return PW_BAD;
        if ((((IN(c->start + 20 + c->k * 30) * 256) + IN(c->start + 21 + c->k * 30)) != 0) && (c->j == 0))
            return PW_BAD;
        if ((IN(c->start + 25 + c->k * 30) != 0) && (c->j == 0))
            return PW_BAD;
        /* get the highest !0 sample */
        if (c->j != 0)
            c->o = c->j + 1;
    }
    if (c->wss <= 2)
        return PW_BAD;

    /* test #4  pattern list size */
    c->l = IN(c->start + 930);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;
    /* c->l holds the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < c->l; c->j++)
    {
        if (IN(c->start + 932 + c->j) > c->k)
            c->k = IN(c->start + 932 + c->j);
        if (IN(c->start + 932 + c->j) > 127)
            return PW_BAD;
    }
    /* c->k holds the highest pattern number */
    /* test last patterns of the pattern list = 0 ? */
    c->j += 2; /* just to be sure .. */
    while (c->j != 128)
    {
        /* guard: a list of 127 entries starts c->j at 129, and the original
         * then ran on through the 32-bit wrap-around of c->j. Bytes outside
         * the input read as 0: jump over them to where the address wraps. */
        a = c->start + 932 + c->j;
        if (c->j > 128 && a >= (uint32_t)c->in_size)
        {
            c->j += 0u - a;
            continue;
        }
        if (IN(a) != 0)
            return PW_BAD;
        c->j += 1;
    }
    /* c->k is the number of pattern in the file (-1) */
    c->k += 1;

    /* test #5 pattern data ... */
    if (((c->k * 768) + 1060 + c->start + c->wss) > (uint32_t)c->in_size)
        return PW_BAD;
    for (c->j = 0; c->j < (c->k * 256); c->j++)
    {
        /* relative note number + last bit of sample > $34 ? */
        if (IN(c->start + 1060 + c->j * 3) > 0x74)
            return PW_BAD;
        if ((IN(c->start + 1060 + c->j * 3) & 0x3F) > 0x24)
            return PW_BAD;
        if (((IN(c->start + 1060 + c->j * 3 + 1) & 0x0F) == 0x0C) &&
            (IN(c->start + 1060 + c->j * 3 + 2) > 0x40))
            return PW_BAD;
        if (((IN(c->start + 1060 + c->j * 3 + 1) & 0x0F) == 0x0B) &&
            (IN(c->start + 1060 + c->j * 3 + 2) > 0x7F))
            return PW_BAD;
        if (((IN(c->start + 1060 + c->j * 3 + 1) & 0x0F) == 0x0D) &&
            (IN(c->start + 1060 + c->j * 3 + 2) > 0x40))
            return PW_BAD;
        c->n = ((IN(c->start + 1060 + c->j * 3) >> 2) & 0x30) | ((IN(c->start + 1061 + c->j * 3 + 1) >> 4) & 0x0F);
        if (c->n > c->o)
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_UNIC2(pw_ctx *c)
{
    /* c->k is still the nbr of pattern */

    c->out_size = c->wss + (c->k * 768) + 1060;

    c->convert = PW_GOOD;
    pw_save_rip(c, "UNIC tracker v2 module", PWF_UNIC_v2);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 26); /* 25 should do but call it "just to be sure" :) */
}

/*
 *   Unic_Tracker_2.c   1997 (c) Asle / ReDoX
 * Unic tracked 2 MODs to Protracker
 */
void pw_depack_UNIC2(pw_ctx *c)
{
    uint8_t  poss[37][2];
    uint8_t  smp, note, fx, fxval;
    uint8_t  whatever[1028];
    int32_t  i = 0, j = 0, k = 0, l = 0;
    int32_t  whole_sample_size = 0;
    int32_t  where = c->start; /* main pointer to prevent fread() */
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 20);

    for (i = 0; i < 31; i++)
    {
        /* sample name */
        pw_fwrite_in(out, where, 20);
        pw_fwrite(out, &whatever[32], 2);

        /* fine on ? (whatever[48] keeps its value otherwise) */
        j = (IN(where + 20) * 256) + IN(where + 21);
        if (j != 0)
        {
            if (j < 256)
                whatever[48] = (uint8_t)(0x10 - IN(where + 21));
            else
                whatever[48] = (uint8_t)(0x100 - IN(where + 21));
        }

        /* smp size */
        pw_fwrite_in(out, where + 22, 2);
        l = ((IN(where + 22) * 256) + IN(where + 23)) * 2;
        whole_sample_size += l;

        /* fine */
        pw_fwrite(out, &whatever[48], 1);

        /* vol */
        pw_fwrite_in(out, where + 25, 1);

        /* loop start */
        whatever[64] = IN(where + 26);
        whatever[65] = IN(where + 27);

        /* loop size */
        j = ((whatever[64] * 256) + whatever[65]) * 2;
        k = ((IN(where + 28) * 256) + IN(where + 29)) * 2;
        if ((((j * 2) + k) <= l) && (j != 0))
        {
            whatever[64] *= 2;
            j = whatever[65] * 2;
            if (j > 256)
                whatever[64] += 1;
            whatever[65] *= 2;
        }

        pw_fwrite(out, &whatever[64], 2);

        pw_fwrite_in(out, where + 28, 2);

        where += 30;
    }

    /* number of pattern */
    pw_fwrite_in(out, where++, 1);

    /* noisetracker byte */
    whatever[32] = 0x7f;
    pw_fwrite(out, &whatever[32], 1);
    where += 1;

    /* Pattern table */
    pw_fwrite_in(out, where, 128);
    where += 128;

    /* get highest pattern number */
    for (i = 0; i < 128; i++)
    {
        if (IN(c->start + 932 + i) > whatever[1026])
            whatever[1026] = IN(c->start + 932 + i);
    }

    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* pattern data */
    for (i = 0; i <= whatever[1026]; i++)
    {
        for (j = 0; j < 256; j++)
        {
            smp = (uint8_t)(((IN(where + j * 3) >> 2) & 0x10) | ((IN(where + j * 3 + 1) >> 4) & 0x0f));
            note = (uint8_t)(IN(where + j * 3) & 0x3f);
            fx = (uint8_t)(IN(where + j * 3 + 1) & 0x0f);
            fxval = IN(where + j * 3 + 2);
            if (fx == 0x0d) /* pattern break */
            {
                whatever[1024] = fxval % 10;
                whatever[1025] = fxval / 10;
                fxval = 16;
                fxval *= whatever[1025];
                fxval += whatever[1024];
            }

            whatever[j * 4] = (uint8_t)(smp & 0xf0);
            /* poss[] has 37 entries; note can reach 63 on odd input */
            whatever[j * 4] |= (note < 37) ? poss[note][0] : 0;
            whatever[j * 4 + 1] = (note < 37) ? poss[note][1] : 0;
            whatever[j * 4 + 2] = (uint8_t)(((smp << 4) & 0xf0) | fx);
            whatever[j * 4 + 3] = fxval;
        }
        pw_fwrite(out, whatever, 1024);
        where += 768;
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    pw_crap("  UNIC Tracker 2  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
