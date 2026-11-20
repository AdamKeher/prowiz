/*
 * pmd3.c - Module-Patterncompressor, "PMD3" (4 channels) and "PMd3"
 *          (8 channels) at the Protracker ID position (upstream r/pmd3.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* (a*256*256*256)+(b*256*256)+(c*256)+d in int arithmetic: the original
 * relied on wrap-around, which uint32_t arithmetic gives without UB. */
static uint32_t pmd3_be32(pw_ctx *c, int64_t at)
{
    return ((uint32_t)IN(at) << 24) + ((uint32_t)IN(at + 1) << 16) +
           ((uint32_t)IN(at + 2) << 8) + (uint32_t)IN(at + 3);
}

int16_t pw_test_PMD3(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1080);

    /* test 1.1 */
    if (c->start + 12 > c->in_size)
        return PW_BAD;

    for (c->k = 0; c->k < 31; c->k++)
    {
        /* size */
        c->j = (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2);
        /* loop size */
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);

        if (pw_test_smps((int32_t)c->j, (int32_t)c->m, (int32_t)c->n,
                         (uint8_t)(IN(c->start + 45 + c->k * 30) / 2), IN(c->start + 44 + c->k * 30)) == PW_BAD)
            return PW_BAD;
    }

    /* test #4  pattern list size */
    c->l = IN(c->start + 950);
    if (c->l > 127)
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

    /* test #5 size ... (c->n is the loop size of the last sample, as upstream) */
    c->m = pmd3_be32(c, c->start + 1084);
    c->o = pmd3_be32(c, c->start + 1088);
    if ((c->o + c->n + c->start) > (uint32_t)c->in_size)
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_PMD3(pw_ctx *c)
{
    uint8_t t = 0x00;
    /* c->k is still the nbr of pattern */
    /* c->m is still the size of note pointers */
    /* c->o is still the size of ref table for notes */

    if (IN(c->start + 1082) == 'D')
        t = 0x20;
    else /* 'd' */
        t = 0x40;

    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
        c->wss += (((IN(c->start + 42 + c->j * 30) * 256) + IN(c->start + 43 + c->j * 30)) * 2);

    /* "+=": the size left over from the previous rip is added in, as upstream */
    c->out_size += c->wss + c->m + c->o + 1092 + (c->k * t);

    c->convert = PW_BAD;
    if (t == 0x20)
        pw_save_rip(c, "Module-Patterncompressor (PMD3)", PWF_PMD3);
    else
        pw_save_rip(c, "Module-Patterncompressor (PMd3)", PWF_PMd3);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}

/*
 *   PMD3.c   20160413 (c) Asle
 *
 * Converts PMD3 MODs back to PTK or OCT
 */
void pw_depack_PMD3(pw_ctx *c)
{
    static const uint8_t id_mk[4] = {'M', '.', 'K', '.'};
    static const uint8_t id_cd81[4] = {'C', 'D', '8', '1'};
    uint8_t  t = 0x00;
    uint8_t  max = 0x00;
    int32_t  whole_sample_size = 0;
    int32_t  i = 0, addy1, addy2, size1, size2, size3, ptr1, ptr2, ptr3;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* get whole sample size */
    for (i = 0; i < 31; i++)
        whole_sample_size += (((IN(where + 42 + i * 30) * 256) + IN(where + 43 + i * 30)) * 2);

    /* read and write whole header */
    pw_fwrite_in(out, where, 1080);

    /* write ID */
    if (IN(c->start + 1082) == 'D')
        t = 0x20;
    else /* 'd' */
        t = 0x40;

    pw_fwrite(out, (t == 0x20) ? id_mk : id_cd81, 4);

    where += 952;

    /* get number of pattern */
    max = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(where + i) > max)
            max = IN(where + i);
    }
    max += 1;

    where += (128 + 4);

    ptr1 = addy1 = where + 8;
    size1 = max * t; /* t says if it's 4 or 8 channels */
    ptr2 = addy2 = addy1 + size1;
    size2 = (int32_t)pmd3_be32(c, where);
    ptr3 = (int32_t)((uint32_t)addy2 + (uint32_t)size2);
    size3 = (int32_t)pmd3_be32(c, where + 4);

    /* pattern data */
    for (i = 0; i < max; i++) /* loop per pattern */
    {
        uint8_t pattern[2048], j, k, l;
        memset(pattern, 0, sizeof pattern);

        /* loop per voice (the original kept these offsets in the scan
         * registers PW_j and PW_k, which the scanner sees afterwards) */
        for (j = 0; j < (t / 8); j++)
        {
            c->j = pmd3_be32(c, ptr1);
            ptr1 += 4;
            c->k = pmd3_be32(c, ptr1);
            ptr1 += 4;

            for (k = 0, l = 0; k < 64;) /* k will count the note occurences */
            {
                uint32_t z, repeat;
                uint8_t  c1 = (uint8_t)((IN(c->j + ptr2 + l) & 0x03) + 1);
                uint8_t  c2 = (uint8_t)(IN(c->j + ptr2 + l) & 0xFC);
                l += 1;

                /* fetch relative note and write it */
                for (repeat = 0; repeat < c1; repeat += 1)
                {
                    z = (j * 4) + ((k + repeat) * (t / 2));

                    /* a run of up to 4 rows starting on row 61..63 reaches
                     * row 66: with 8 channels that is up to 96 bytes past
                     * the 2048 byte pattern; drop those writes */
                    if (z + 3 < sizeof pattern)
                    {
                        pattern[z] = IN(c->k + ptr3 + c2);
                        pattern[z + 1] = IN(c->k + ptr3 + c2 + 1);
                        pattern[z + 2] = IN(c->k + ptr3 + c2 + 2);
                        pattern[z + 3] = IN(c->k + ptr3 + c2 + 3);
                    }
                }

                k += c1;
            }
        }

        /* write pattern */
        if (t == 0x20)
            pw_fwrite(out, pattern, 1024);
        else
            pw_fwrite(out, pattern, 2048);
    }

    /* sample data */
    where = (int32_t)((uint32_t)c->start + 1092 + (uint32_t)size1 + (uint32_t)size2 + (uint32_t)size3);
    pw_fwrite_in(out, where, whole_sample_size);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
