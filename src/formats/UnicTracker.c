/*
 * UnicTracker.c - UNIC Tracker v1, with "M.K."/"UNIC" ID, empty ID or no ID
 * (upstream r/UnicTracker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/*
 * 27 dec 2001 : added some checks to prevent reading outside of input file.
 * 18 mar 2003 : a small bug in first test ...
 */

int16_t pw_test_UNIC_withID(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1080);
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->j = (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);
        c->wss += c->j;
        c->n = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2)
             + (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);
        if ((c->j + 2) < c->n)
            return PW_BAD;
    }
    if (c->wss <= 2)
        return PW_BAD;

    /* test #3  finetunes & volumes */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if ((IN(c->start + 44 + c->k * 30) > 0x0f) || (IN(c->start + 45 + c->k * 30) > 0x40))
            return PW_BAD;
    }

    /* test #4  pattern list size */
    c->l = IN(c->start + 950);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;
    /* c->l holds the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < c->l; c->j++)
    {
        if (IN(c->start + 952 + c->j) > c->k)
            c->k = IN(c->start + 952 + c->j);
        if (IN(c->start + 952 + c->j) > 127)
            return PW_BAD;
    }
    /* c->k holds the highest pattern number */
    /* test last patterns of the pattern list = 0 ? */
    while (c->j != 128)
    {
        if (IN(c->start + 952 + c->j) != 0)
            return PW_BAD;
        c->j += 1;
    }
    /* c->k is the number of pattern in the file (-1) */
    c->k += 1;

    /* test #5 pattern data ... */
    if (((c->k * 768) + 1084 + c->start) > (uint32_t)c->in_size)
        return PW_BAD;
    for (c->j = 0; c->j < (c->k * 256); c->j++)
    {
        /* relative note number + last bit of sample > $34 ? */
        if (IN(c->start + 1084 + c->j * 3) > 0x74)
            return PW_BAD;
    }

    return PW_GOOD;
}

int16_t pw_test_UNIC_withemptyID(pw_ctx *c)
{
    /* test 1 */
    if ((c->i < 45) || ((c->i - 45 + 1084) >= (uint32_t)c->in_size))
        return PW_BAD;

    /* test #2 ID = $00000000 ? */
    c->start = (int32_t)(c->i - 45);
    if ((IN(c->start + 1080) != 00)
        && (IN(c->start + 1081) != 00)
        && (IN(c->start + 1082) != 00)
        && (IN(c->start + 1083) != 00))
        return PW_BAD;

    /* test 2,5 :) */
    c->wss = 0;
    c->o = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->j = (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2);
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);
        c->wss += c->j;

        if ((c->n != 0) && ((c->j + 2) < (c->m + c->n)))
            return PW_BAD;
        if ((c->j > 0xffff) ||
            (c->m > 0xffff) ||
            (c->n > 0xffff))
            return PW_BAD;
        if (IN(c->start + 45 + c->k * 30) > 0x40)
            return PW_BAD;
        /* finetune ... */
        if (((((IN(c->start + 40 + c->k * 30) * 256) + IN(c->start + 41 + c->k * 30)) != 0) && (c->j == 0)) ||
            ((((IN(c->start + 40 + c->k * 30) * 256) + IN(c->start + 41 + c->k * 30)) > 8) &&
             (((IN(c->start + 40 + c->k * 30) * 256) + IN(c->start + 41 + c->k * 30)) < 247)))
            return PW_BAD;
        /* loop start but no replen ? */
        if ((c->m != 0) && (c->n <= 2))
            return PW_BAD;
        if ((IN(c->start + 45 + c->k * 30) != 0) && (c->j == 0))
            return PW_BAD;
        /* get the highest !0 sample */
        if (c->j != 0)
            c->o = c->j + 1;
    }
    if (c->wss <= 2)
        return PW_BAD;

    /* test #4  pattern list size */
    c->l = IN(c->start + 950);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;
    /* c->l holds the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < c->l; c->j++)
    {
        if (IN(c->start + 952 + c->j) > c->k)
            c->k = IN(c->start + 952 + c->j);
        if (IN(c->start + 952 + c->j) > 127)
            return PW_BAD;
    }
    /* c->k holds the highest pattern number */
    /* test last patterns of the pattern list = 0 ? */
    while (c->j != 128)
    {
        if (IN(c->start + 952 + c->j) != 0)
            return PW_BAD;
        c->j += 1;
    }
    /* c->k is the number of pattern in the file (-1) */
    c->k += 1;

    /* test #5 pattern data ... */
    if (((c->k * 768) + 1084 + c->start) > (uint32_t)c->in_size)
        return PW_BAD;
    for (c->j = 0; c->j < (c->k * 256); c->j++)
    {
        /* relative note number + last bit of sample > $34 ? */
        if (IN(c->start + 1084 + c->j * 3) > 0x74)
            return PW_BAD;
        if ((IN(c->start + 1084 + c->j * 3) & 0x3F) > 0x24)
            return PW_BAD;
        if (((IN(c->start + 1084 + c->j * 3 + 1) & 0x0F) == 0x0C) &&
            (IN(c->start + 1084 + c->j * 3 + 2) > 0x40))
            return PW_BAD;
        if (((IN(c->start + 1084 + c->j * 3 + 1) & 0x0F) == 0x0B) &&
            (IN(c->start + 1084 + c->j * 3 + 2) > 0x7F))
            return PW_BAD;
        if (((IN(c->start + 1084 + c->j * 3 + 1) & 0x0F) == 0x0D) &&
            (IN(c->start + 1084 + c->j * 3 + 2) > 0x40))
            return PW_BAD;
        c->n = ((IN(c->start + 1084 + c->j * 3) >> 2) & 0x30) | ((IN(c->start + 1085 + c->j * 3 + 1) >> 4) & 0x0F);
        if (c->n > c->o)
            return PW_BAD;
    }

    return PW_GOOD;
}

int16_t pw_test_UNIC_noID(pw_ctx *c)
{
    /* test 1 */
    if ((c->i < 45) || ((c->i - 45 + 1083) >= (uint32_t)c->in_size))
        return PW_BAD;

    /* test #2 ID = $00000000 ? */
    c->start = (int32_t)(c->i - 45);
    if ((IN(c->start + 1080) == 00)
        && (IN(c->start + 1081) == 00)
        && (IN(c->start + 1082) == 00)
        && (IN(c->start + 1083) == 00))
        return PW_BAD;

    /* test 2,5 :) */
    c->wss = 0;
    c->o = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->j = (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2);
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);
        c->wss += c->j;
        if ((c->n != 0) && ((c->j + 2) < (c->m + c->n)))
            return PW_BAD;
        /* samples too big ? */
        if ((c->j > 0xffff) ||
            (c->m > 0xffff) ||
            (c->n > 0xffff))
            return PW_BAD;
        /* volume too big */
        if (IN(c->start + 45 + c->k * 30) > 0x40)
            return PW_BAD;
        /* finetune ... */
        if (((((IN(c->start + 40 + c->k * 30) * 256) + IN(c->start + 41 + c->k * 30)) != 0) && (c->j == 0)) ||
            ((((IN(c->start + 40 + c->k * 30) * 256) + IN(c->start + 41 + c->k * 30)) > 8) &&
             (((IN(c->start + 40 + c->k * 30) * 256) + IN(c->start + 41 + c->k * 30)) < 247)))
            return PW_BAD;
        /* loop start but no replen ? */
        if ((c->m != 0) && (c->n <= 2))
            return PW_BAD;
        if ((IN(c->start + 45 + c->k * 30) != 0) && (c->j == 0))
            return PW_BAD;
        /* get the highest !0 sample */
        if (c->j != 0)
            c->o = c->j + 1;
    }
    if (c->wss <= 2)
        return PW_BAD;

    /* test #4  pattern list size */
    c->l = IN(c->start + 950);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;
    /* c->l holds the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < c->l; c->j++)
    {
        if (IN(c->start + 952 + c->j) > c->k)
            c->k = IN(c->start + 952 + c->j);
        if (IN(c->start + 952 + c->j) > 127)
            return PW_BAD;
    }
    /* c->k holds the highest pattern number */
    /* test last patterns of the pattern list = 0 ? */
    while (c->j != 128)
    {
        if (IN(c->start + 952 + c->j) != 0)
            return PW_BAD;
        c->j += 1;
    }
    /* c->k is the number of pattern in the file (-1) */
    c->k += 1;

    /* test #5 pattern data ... */
    /* c->o is the highest !0 sample */
    if (((c->k * 768) + 1080 + c->start) > (uint32_t)c->in_size)
        return PW_BAD;
    for (c->j = 0; c->j < (c->k * 256); c->j++)
    {
        /* relative note number + last bit of sample > $34 ? */
        if (IN(c->start + 1080 + c->j * 3) > 0x74)
            return PW_BAD;
        if ((IN(c->start + 1080 + c->j * 3) & 0x3F) > 0x24)
            return PW_BAD;
        if (((IN(c->start + 1080 + c->j * 3 + 1) & 0x0F) == 0x0C) &&
            (IN(c->start + 1080 + c->j * 3 + 2) > 0x40))
            return PW_BAD;
        if (((IN(c->start + 1080 + c->j * 3 + 1) & 0x0F) == 0x0B) &&
            (IN(c->start + 1080 + c->j * 3 + 2) > 0x7F))
            return PW_BAD;
        if (((IN(c->start + 1080 + c->j * 3 + 1) & 0x0F) == 0x0D) &&
            (IN(c->start + 1080 + c->j * 3 + 2) > 0x40))
            return PW_BAD;
        c->n = ((IN(c->start + 1080 + c->j * 3) >> 2) & 0x30) | ((IN(c->start + 1081 + c->j * 3 + 1) >> 4) & 0x0F);
        if (c->n > c->o)
            return PW_BAD;
    }

    /* test #6  title coherent ? */
    for (c->j = 0; c->j < 20; c->j++)
    {
        if (((IN(c->start + c->j) != 0)
             && (IN(c->start + c->j) < 32))
            || (IN(c->start + c->j) > 180))
            return PW_BAD;
    }

    return PW_GOOD;
    /* c->k is still the number of pattern */
}

void pw_rip_UNIC_withID(pw_ctx *c)
{
    /* c->k is still the nbr of pattern */

    c->out_size = c->wss + (c->k * 768) + 1084;

    c->convert = PW_GOOD;
    pw_save_rip(c, "UNIC tracker v1 module", PWF_UNIC_v1);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1081); /* 1080 should do but call it "just to be sure" :) */
}

void pw_rip_UNIC_noID(pw_ctx *c)
{
    /* c->k is still the nbr of pattern */

    c->out_size = c->wss + (c->k * 768) + 1080;

    c->convert = PW_GOOD;
    pw_save_rip(c, "UNIC tracker v1 module", PWF_UNIC_v1);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 46); /* 45 should do but call it "just to be sure" :) */
}

/*
 *   Unic_Tracker.c   1997 (c) Asle / ReDoX
 * Unic tracked MODs to Protracker
 * both with or without ID Unic files will be converted
 */
void pw_depack_UNIC(pw_ctx *c)
{
    uint8_t  c1, c2, c3, c4;
    uint8_t  number_of_pattern;
    uint8_t  poss[37][2];
    uint8_t  max = 0x00;
    uint8_t  smp, note, fx, fxval;
    uint8_t  fine = 0x00;
    uint8_t  pattern[1025];
    uint32_t i = 0, j = 0, k = 0, l = 0;
    uint32_t whole_sample_size = 0;
    uint32_t where = (uint32_t)c->start; /* main pointer to prevent fread() */
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    memset(pattern, 0, sizeof pattern);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* title */
    pw_fwrite_in(out, where, 20);
    where += 20;

    for (i = 0; i < 31; i++)
    {
        /* sample name */
        pw_fwrite_in(out, where, 20);
        c1 = 0x00;
        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c1, 1);
        where += 20;

        /* fine  */
        c1 = IN(where++);
        c2 = IN(where++);
        j = (c1 * 256) + c2;
        if (j != 0)
        {
            if (j < 256)
                fine = (uint8_t)(0x10 - (c2 % 7));
            else
            {
                /* 20130827 - fix for case that are *7 ... weird */
                j = 0x10000 - j;
                if (j > 8)
                    fine = (uint8_t)(j / 7);
                else
                    fine = (uint8_t)j;
            }
        }
        else
            fine = 0x00;

        /* smp size */
        c1 = IN(where++);
        c2 = IN(where++);
        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c2, 1);
        l = ((c1 * 256) + c2) * 2;
        whole_sample_size += l;

        /* fine */
        where += 1;
        pw_fwrite(out, &fine, 1);

        /* vol */
        pw_fwrite_in(out, where++, 1);

        /* loop start */
        c1 = IN(where++);
        c2 = IN(where++);

        /* loop size */
        c3 = IN(where++);
        c4 = IN(where++);

        j = ((c1 * 256) + c2) * 2;
        k = ((c3 * 256) + c4) * 2;
        if ((((j * 2) + k) <= l) && (j != 0))
        {
            c1 *= 2;
            j = c2 * 2;
            if (j > 256)
                c1 += 1;
            c2 *= 2;
        }

        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c2, 1);

        pw_fwrite(out, &c3, 1);
        pw_fwrite(out, &c4, 1);
    }

    /* number of pattern */
    number_of_pattern = IN(where++);
    pw_fwrite(out, &number_of_pattern, 1);

    /* noisetracker byte */
    c1 = 0x7f;
    pw_fwrite(out, &c1, 1);
    where += 1;

    /* Pattern table */
    pw_fwrite_in(out, where, 128);
    where += 128;

    /* get highest pattern number */
    for (i = 0; i < 128; i++)
    {
        if (IN(c->start + 952 + i) > max)
            max = IN(c->start + 952 + i);
    }
    max += 1; /* coz first is $00 */

    c1 = 'M';
    c2 = '.';
    c3 = 'K';
    pw_fwrite(out, &c1, 1);
    pw_fwrite(out, &c2, 1);
    pw_fwrite(out, &c3, 1);
    pw_fwrite(out, &c2, 1);

    /* verify UNIC ID */
    where = (uint32_t)c->start + 1080;
    if (((IN(where) == 'M') && (IN(where + 1) == '.') && (IN(where + 2) == 'K') && (IN(where + 3) == '.')) ||
        ((IN(where) == 'U') && (IN(where + 1) == 'N') && (IN(where + 2) == 'I') && (IN(where + 3) == 'C')) ||
        ((IN(where) == 0x00) && (IN(where + 1) == 0x00) && (IN(where + 2) == 0x00) && (IN(where + 3) == 0x00)))
        where = (uint32_t)c->start + 1084;
    else
        where = (uint32_t)c->start + 1080;

    /* pattern data */
    for (i = 0; i < max; i++)
    {
        for (j = 0; j < 256; j++)
        {
            smp = (uint8_t)(((IN(where + j * 3) >> 2) & 0x10) | ((IN(where + j * 3 + 1) >> 4) & 0x0f));
            note = (uint8_t)(IN(where + j * 3) & 0x3f);
            fx = (uint8_t)(IN(where + j * 3 + 1) & 0x0f);
            fxval = IN(where + j * 3 + 2);

            if (fx == 0x0d) /* pattern break */
            {
                c4 = fxval % 10;
                c3 = fxval / 10;
                fxval = 16;
                fxval *= c3;
                fxval += c4;
            }

            pattern[j * 4] = (uint8_t)(smp & 0xf0);
            /* poss[] has 37 entries; note can reach 63 on odd input */
            pattern[j * 4] |= (note < 37) ? poss[note][0] : 0;
            pattern[j * 4 + 1] = (note < 37) ? poss[note][1] : 0;
            pattern[j * 4 + 2] = (uint8_t)(((smp << 4) & 0xf0) | fx);
            pattern[j * 4 + 3] = fxval;
        }
        pw_fwrite(out, pattern, 1024);
        where += 768;
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    pw_crap("   UNIC Tracker   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
