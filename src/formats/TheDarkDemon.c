/*
 * TheDarkDemon.c - The Dark Demon (group name) packed modules
 *                  (upstream r/TheDarkDemon.c).
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

/* (in[a]*256*256*256)+(in[a+1]*256*256)+(in[a+2]*256)+in[a+3], each offset
 * an unsigned 32-bit expression as upstream wrote it */
static uint32_t be32u(const pw_ctx *c, uint32_t a)
{
    return ((uint32_t)IN(a) << 24) | ((uint32_t)IN((uint32_t)(a + 1)) << 16) |
           ((uint32_t)IN((uint32_t)(a + 2)) << 8) | (uint32_t)IN((uint32_t)(a + 3));
}

/* same, for a signed 32-bit offset (upstream relied on int wrap-around) */
static int32_t be32s(const pw_ctx *c, int32_t a)
{
    return (int32_t)(((uint32_t)IN(a) << 24) | ((uint32_t)IN((int32_t)((uint32_t)a + 1)) << 16) |
                     ((uint32_t)IN((int32_t)((uint32_t)a + 2)) << 8) |
                     (uint32_t)IN((int32_t)((uint32_t)a + 3)));
}

int16_t pw_test_TheDarkDemon(pw_ctx *c)
{
    /* test #1 */
    if (c->i < 137)
        return PW_BAD;
    c->start = (int32_t)(c->i - 137);

    /* len of file */
    if ((c->start + 564) >= c->in_size)
        return PW_BAD;

    /* test #2 (volumes,sample addresses and whole sample size) */
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
    {
        /* sample address */
        c->k = be32u(c, c->start + c->j * 14 + 130);
        /* sample size */
        c->l = (((IN(c->start + c->j * 14 + 134) * 256) + IN(c->start + c->j * 14 + 135)) * 2);
        /* loop start address */
        c->m = be32u(c, c->start + c->j * 14 + 138);
        /* loop size (replen) */
        c->n = (((IN(c->start + c->j * 14 + 142) * 256) + IN(c->start + c->j * 14 + 143)) * 2);

        /* finetune > 0Fh ? */
        if (IN(c->start + c->j * 14 + 136) > 0x0F)
            return PW_BAD;

        /* volume > 40h ? */
        if (IN(c->start + c->j * 14 + 137) > 0x40)
            return PW_BAD;

        /* loop start addy < sampl addy ? */
        if (c->m < c->k)
            return PW_BAD;

        /* addy < 564 ? */
        if ((c->k < 564) || (c->m < 564))
            return PW_BAD;

        /* loop start > size ? */
        if ((c->m - c->k) > c->l)
            return PW_BAD;

        /* loop start+replen > size ? */
        if (((c->m - c->k) + c->n) > (c->l + 2))
            return PW_BAD;
        c->wss += c->l;
    }

    if ((c->wss <= 2) || (c->wss > (31 * 65535)))
        return PW_BAD;

    /* test #3 (addresses of pattern in file ... possible ?) */
    /* c->wss is the whole sample size :) */
    if ((c->wss + 564) > (uint32_t)c->in_size)
        return PW_BAD;

    /* test size of pattern list */
    if ((IN(c->start) > 0x7f) || (IN(c->start) == 0x00))
        return PW_BAD;

    /* test pattern list */
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + c->j + 2) > 0x7f)
            return PW_BAD;
        if (IN(c->start + c->j + 2) > c->k)
            c->k = IN(c->start + c->j + 2);
    }
    c->k += 1;
    c->k *= 1024;

    /* test end of pattern list */
    for (c->j = IN(c->start) + 2; c->j < 128; c->j++)
    {
        if (IN(c->start + c->j + 2) != 0)
            return PW_BAD;
    }

    /* test if not out of file range */
    if ((c->start + c->wss + 564 + c->k) > (uint32_t)c->in_size)
        return PW_BAD;

    /* c->wss is the whole sample data size */
    /* c->k is the whole pattern data size */
    /* test pattern data now ... */
    c->l = c->start + 564 + c->wss;
    /* c->l points on pattern data */
    /* upstream: "for (PW_j=0; PW_j<PW_k; PW_j+=4);" - the stray ';' leaves the
     * loop empty, so the block below runs once with c->j == c->k */
    for (c->j = 0; c->j < c->k; c->j += 4)
    {
    }
    {
        /* sample number > 31 ? */
        if (IN(c->l + c->j) > 0x1f)
            return PW_BAD;
        /* note > 0x48 (36*2) */
        if ((IN(c->l + c->j + 1) > 0x48) || ((IN(c->l + c->j + 1) & 0x01) == 0x01))
            return PW_BAD;
        /* fx=C and FxArg > 64 ? */
        if (((IN(c->l + c->j + 2) & 0x0f) == 0x0c) && (IN(c->l + c->j + 3) > 0x40))
            return PW_BAD;
        /* fx=D and FxArg > 64 ? */
        if (((IN(c->l + c->j + 2) & 0x0f) == 0x0d) && (IN(c->l + c->j + 3) > 0x40))
            return PW_BAD;
        /* fx=B and FxArg > 127 ? */
        if (((IN(c->l + c->j + 2) & 0x0f) == 0x0b) && (IN(c->l + c->j + 3) > 0x7f))
            return PW_BAD;
    }

    /* let's get free another var .. */
    c->wss += c->k;

    return PW_GOOD;
}

void pw_rip_TheDarkDemon(pw_ctx *c)
{
    /* c->wss is the WholeSampleSize + pattern data size */

    /* 564 = header */
    c->out_size = c->wss + 564;

    c->convert = PW_GOOD;
    pw_save_rip(c, "The Dark Demon module", PWF_TDD);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 140); /* 137 should do but call it "just to be sure" :) */
}

/*
 *   TDD.c   1999 (c) Asle / ReDoX
 *
 * Converts TDD packed MODs back to PTK MODs
 *
 * Update : 6 apr 2003
 *  - removed fopen() func ... .
 * Update : 26 nov 2003
 *  - used htonl() so that use of addy is now portable on 68k archs
 */
void pw_depack_TheDarkDemon(pw_ctx *c)
{
    uint8_t  c1 = 0x00, c2 = 0x00, c3 = 0x00;
    uint8_t  poss[37][2];
    uint8_t  pattern[1024];
    uint8_t  pat_max = 0x00;
    int32_t  i = 0, j = 0, k = 0;
    int32_t  whole_sample_size = 0;
    int32_t  sample_addresses[31];
    int32_t  sample_sizes[31];
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    pw_fill_ptk_table(poss);

    memset(sample_addresses, 0, sizeof sample_addresses);
    memset(sample_sizes, 0, sizeof sample_sizes);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* write ptk header */
    pw_fzero(out, 1080);

    /* read/write pattern list + size and ntk byte */
    pw_fseek(out, 950, SEEK_SET);
    pw_fwrite_in(out, where, 130);
    pat_max = 0x00;
    for (i = 0; i < 128; i++)
        if (IN(where + i + 2) > pat_max)
            pat_max = IN(where + i + 2);
    where += 130;

    /* sample descriptions */
    for (i = 0; i < 31; i++)
    {
        pw_fseek(out, 42 + (i * 30), SEEK_SET);
        /* sample address */
        sample_addresses[i] = be32s(c, where);
        where += 4;
        /* read/write size */
        whole_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);
        sample_sizes[i] = (((IN(where) * 256) + IN(where + 1)) * 2);
        pw_fwrite_in(out, where, 2);
        where += 2;

        /* read/write finetune */
        /* read/write volume */
        pw_fwrite_in(out, where, 2);
        where += 2;

        /* read loop start address */
        j = be32s(c, where);
        where += 4;
        j = (int32_t)((uint32_t)j - (uint32_t)sample_addresses[i]);
        j /= 2;
        /* htonl(j), then bytes 2 and 3: the low 16 bits, big-endian */
        c1 = (uint8_t)((uint32_t)j >> 8);
        c2 = (uint8_t)j;

        /* write loop start */
        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c2, 1);

        /* read/write replen */
        pw_fwrite_in(out, where, 2);
        where += 2;
    }

    /* bypass Samples datas */
    where += whole_sample_size;

    /* write ptk's ID string */
    pw_fseek(out, 0, SEEK_END);
    c1 = 'M';
    c2 = '.';
    c3 = 'K';
    pw_fwrite(out, &c1, 1);
    pw_fwrite(out, &c2, 1);
    pw_fwrite(out, &c3, 1);
    pw_fwrite(out, &c2, 1);

    /* read/write pattern data */
    for (i = 0; i <= pat_max; i++)
    {
        memset(pattern, 0, sizeof pattern);
        for (j = 0; j < 64; j++)
        {
            for (k = 0; k < 4; k++)
            {
                /* fx arg */
                pattern[j * 16 + k * 4 + 3] = IN(where + j * 16 + k * 4 + 3);

                /* fx */
                pattern[j * 16 + k * 4 + 2] = IN(where + j * 16 + k * 4 + 2) & 0x0f;

                /* smp */
                pattern[j * 16 + k * 4] = IN(where + j * 16 + k * 4) & 0xf0;
                pattern[j * 16 + k * 4 + 2] |= (uint8_t)((IN(where + j * 16 + k * 4) << 4) & 0xf0);

                /* note */
                pattern[j * 16 + k * 4] |= poss_at(poss, IN(where + j * 16 + k * 4 + 1) / 2, 0);
                pattern[j * 16 + k * 4 + 1] = poss_at(poss, IN(where + j * 16 + k * 4 + 1) / 2, 1);
            }
        }
        pw_fwrite(out, pattern, 1024);
        where += 1024;
    }

    /* Sample data */
    for (i = 0; i < 31; i++)
    {
        if (sample_sizes[i] == 0)
            continue;
        where = (int32_t)((uint32_t)c->start + (uint32_t)sample_addresses[i]);
        pw_fwrite_in(out, where, sample_sizes[i]);
    }

    pw_crap("  The Dark Demon  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
