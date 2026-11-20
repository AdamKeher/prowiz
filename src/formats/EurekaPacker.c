/*
 * EurekaPacker.c - Eureka Packer (upstream r/EurekaPacker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* (27 dec 2001)
 *   added some checks to prevent readings outside of input file (in test 1)
 * (May 2002)
 *   added test_smps()
 * (30/08/10)
 *   changed #4.3 as the "remaining" patternlist isn't always 0x00
 */
int16_t pw_test_EUREKA(pw_ctx *c)
{
    /* test 1 (c->start still holds the value of the previous test, as in
     * the original) */
    if ((c->i < 45) || ((c->start + 950) >= c->in_size))
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 45);
    c->j = IN(c->start + 950);
    if ((c->j == 0) || (c->j > 127))
        return PW_BAD;

    /* test #3  finetunes & volumes */
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->o = (IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30);
        c->m = (IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30);
        c->n = (IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30);
        c->o *= 2;
        c->m *= 2;
        c->n *= 2;
        if ((c->o > 0xffff) ||
            (c->m > 0xffff) ||
            (c->n > 0xffff))
            return PW_BAD;
        if (pw_test_smps((int32_t)(c->o + 2), (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 45 + c->k * 30), IN(c->start + 44 + c->k * 30)) == PW_BAD)
            return PW_BAD;
        c->wss += c->o;
    }

    /* test 4 */
    c->l = (IN(c->start + 1080) * 256 * 256 * 256)
         + (IN(c->start + 1081) * 256 * 256)
         + (IN(c->start + 1082) * 256)
         + IN(c->start + 1083);
    if ((c->l + (uint32_t)c->start) > (uint32_t)c->in_size)
        return PW_BAD;
    if (c->l < 1084)
        return PW_BAD;
    c->m = 0;
    /* pattern list */
    for (c->k = 0; c->k < c->j; c->k++)
    {
        c->n = IN(c->start + 952 + c->k);
        if (c->n > c->m)
            c->m = c->n;
        if (c->n > 127)
            return PW_BAD;
    }
    c->k += 2; /* to be sure .. */
    while (c->k != 128)
    {
        if (IN(c->start + 952 + c->k) > 0x64)
            return PW_BAD;
        c->k += 1;
    }
    c->m += 1;
    /* c->m is the highest pattern number */

    /* test #5 */
    /* c->j is still the size if the pattern table */
    /* c->l is still the address of the sample data */
    /* c->m is the highest pattern number */
    c->n = 0;
    c->j = 999999;
    for (c->k = 0; c->k < (c->m * 4); c->k++)
    {
        c->o = (IN(c->start + c->k * 2 + 1084) * 256) + IN(c->start + c->k * 2 + 1085);
        if ((c->o > c->l) || (c->o < 1084))
            return PW_BAD;
        if (c->o > c->n)
            c->n = c->o;
        if (c->o < c->j)
            c->j = c->o;
    }
    /* c->o is the highest track address */
    /* c->j is the lowest track address */

    /* test track datas */
    /* last track wont be tested ... */
    for (c->k = c->j; c->k < c->o; c->k++)
    {
        if ((IN(c->start + c->k) & 0xC0) == 0xC0)
            continue;
        if ((IN(c->start + c->k) & 0xC0) == 0x80)
        {
            c->k += 2;
            continue;
        }
        if ((IN(c->start + c->k) & 0xC0) == 0x40)
        {
            if (((IN(c->start + c->k) & 0x3F) == 0x00) &&
                (IN(c->start + c->k + 1) == 0x00))
                return PW_BAD;
            c->k += 1;
            continue;
        }
        if ((IN(c->start + c->k) & 0xC0) == 0x00)
        {
            if (IN(c->start + c->k) > 0x13)
                return PW_BAD;
            c->k += 3;
            continue;
        }
    }

    return PW_GOOD;
}

void pw_rip_EUREKA(pw_ctx *c)
{
    /* c->l is still the sample data address */
    /* c->wss is already the whole sample size */

    c->out_size = c->wss + c->l;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Eureka Packed module", PWF_Eureka_packer);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 46); /* 45 should do but call it "just to be sure" :) */
}

/*
 *   EurekaPacker.c   1997 (c) Asle / ReDoX
 *
 * Converts MODs packed with Eureka packer back to ptk
 */
void pw_depack_EUREKA(pw_ctx *c)
{
    uint8_t  whatever[1024];
    uint8_t  c1 = 0x00;
    uint8_t  pat_max = 0x00;
    int32_t  sample_start_address = 0;
    int32_t  whole_sample_size = 0;
    int32_t  track_address[128][4];
    int32_t  i = 0, j = 0, k;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(track_address, 0, sizeof track_address);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* read header ... same as ptk */
    pw_fwrite_in(out, where, 1080);

    /* now, let's sort out that a bit :) */
    /* first, the whole sample size */
    for (i = 0; i < 31; i++)
        whole_sample_size += (((IN(where + i * 30 + 42) * 256) + IN(where + i * 30 + 43)) * 2);

    /* now, the pattern list .. and the max */
    pat_max = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(where + 952 + i) > pat_max)
            pat_max = IN(where + 952 + i);
    }
    pat_max += 1;

    /* write ptk's ID */
    memset(whatever, 0, sizeof whatever);
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* read sample data address */
    where = c->start + 1080;
    sample_start_address = (IN(where) * 256 * 256 * 256) +
                           (IN(where + 1) * 256 * 256) +
                           (IN(where + 2) * 256) +
                           IN(where + 3);
    where += 4;

    /* read tracks addresses (the two pattern list bytes after its end are
     * not tested: pat_max can exceed 128, where the original wrote past
     * track_address[]) */
    for (i = 0; i < pat_max; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (i < 128)
                track_address[i][j] = (IN(where) * 256) + IN(where + 1);
            where += 2;
        }
    }

    /* the track data now ... */
    for (i = 0; i < pat_max; i++)
    {
        memset(whatever, 0, sizeof whatever);
        for (j = 0; j < 4; j++)
        {
            where = c->start + ((i < 128) ? track_address[i][j] : 0);
            for (k = 0; k < 64; k++)
            {
                c1 = IN(where++);
                if ((c1 & 0xc0) == 0x00)
                {
                    whatever[k * 16 + j * 4] = c1;
                    whatever[k * 16 + j * 4 + 1] = IN(where++);
                    whatever[k * 16 + j * 4 + 2] = IN(where++);
                    whatever[k * 16 + j * 4 + 3] = IN(where++);
                    continue;
                }
                if ((c1 & 0xc0) == 0xc0)
                {
                    k += (c1 & 0x3f);
                    continue;
                }
                if ((c1 & 0xc0) == 0x40)
                {
                    whatever[k * 16 + j * 4 + 2] = (uint8_t)(c1 & 0x0f);
                    whatever[k * 16 + j * 4 + 3] = IN(where++);
                    continue;
                }
                if ((c1 & 0xc0) == 0x80)
                {
                    whatever[k * 16 + j * 4] = IN(where++);
                    whatever[k * 16 + j * 4 + 1] = IN(where++);
                    whatever[k * 16 + j * 4 + 2] = (uint8_t)((c1 << 4) & 0xf0);
                    continue;
                }
            }
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* go to sample data addy */
    where = c->start + sample_start_address;

    /* read sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap .. */
    pw_crap("  EUREKA Packer   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
