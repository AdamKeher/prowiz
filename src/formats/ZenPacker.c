/*
 * ZenPacker.c - ZEN Packer (upstream r/ZenPacker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* poss[] has 37 entries; upstream indexed it with note numbers up to 63 */
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

int16_t pw_test_ZEN(pw_ctx *c)
{
    /* test #1 */
    if (c->i < 9)
        return PW_BAD;

    /* test #2 */
    c->start = (int32_t)(c->i - 9);
    c->l = (uint32_t)be32s(c, c->start);
    if ((c->l < 502) || (c->l > 2163190l) || (c->l > (uint32_t)(c->in_size - c->start)))
        return PW_BAD;
    /* c->l is the address of the pattern list */

    /* volumes */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if (((c->start + 9 + (16 * c->k)) > (uint32_t)c->in_size) ||
            ((IN(c->start + 9 + (16 * c->k)) > 0x40)))
            return PW_BAD;
    }

    /* fine */
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->j = (IN(c->start + 6 + (c->k * 16)) * 256) + IN(c->start + 7 + (c->k * 16));
        if (((c->j / 72) * 72) != c->j)
            return PW_BAD;
    }

    /* smp sizes .. */
    c->n = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->o = (IN(c->start + 10 + c->k * 16) * 256) + IN(c->start + 11 + c->k * 16);
        c->m = (IN(c->start + 12 + c->k * 16) * 256) + IN(c->start + 13 + c->k * 16);
        c->j = be32u(c, c->start + 14 + c->k * 16);
        c->o *= 2;
        c->m *= 2;
        /* sample size and loop size > 64k ? */
        if ((c->o > 0xFFFF) || (c->m > 0xFFFF))
            return PW_BAD;
        /* sample address < pattern table address ? */
        if (c->j < c->l)
            return PW_BAD;
        /* too big an address ? */
        if (c->j > (uint32_t)c->in_size)
            return PW_BAD;
        /* get the nbr of the highest sample address and its size */
        if (c->j > c->n)
        {
            c->n = c->j;
            c->wss = c->o;
        }
    }
    /* c->n is the highest sample data address */
    /* c->wss is the size of the same sample */

    /* test size of the pattern list */
    c->j = IN(c->start + 5);
    if ((c->j > 0x7f) || (c->j == 0))
        return PW_BAD;

    /* test if the end of pattern list is $FFFFFFFF */
    if ((IN(c->start + c->l + c->j * 4) != 0xFF) ||
        (IN(c->start + c->l + c->j * 4 + 1) != 0xFF) ||
        (IN(c->start + c->l + c->j * 4 + 2) != 0xFF) ||
        (IN(c->start + c->l + c->j * 4 + 3) != 0xFF))
        return PW_BAD;

    /* c->n is the highest address of a sample data */
    /* c->wss is its size */
    return PW_GOOD;
}

void pw_rip_ZEN(pw_ctx *c)
{
    /* c->n is the highest sample data address */
    /* c->wss if its size */

    c->out_size = c->wss + c->n;

    c->convert = PW_GOOD;
    pw_save_rip(c, "ZEN Packer module", PWF_ZEN);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 10); /* 9 should do but call it "just to be sure" :) */
}

/*
 *   Zen_Packer.c   1998 (c) Asle / ReDoX
 *
 * Converts ZEN packed MODs back to PTK MODs
 ********************************************************
 * 13 april 1999 : Update
 *   - no more open() of input file ... so no more fread() !.
 *     It speeds-up the process quite a bit :).
 * 28 Nov 1999 : Update
 *   - Speed an size optimizings.
 * 03 april 2000 : Update
 *   - small bug correction (harmless)
 *     again pointed out by Thomas Neumann
 * Another Update : 26 nov 2003
 *   - used htonl() so that use of addy is now portable on 68k archs
 */
void pw_depack_ZEN(pw_ctx *c)
{
    uint8_t  pat_pos;
    uint8_t  whatever[1024];
    uint8_t  pat_max;
    uint8_t  poss[37][2];
    uint8_t  note, smp, fx, fx_val;
    int32_t  whole_sample_size = 0;
    int32_t  pattern_address[128];
    int32_t  pattern_address_real[128];
    int32_t  pattern_table_address;
    int32_t  sample_data_address = 999999l;
    int32_t  i, j, k;
    int32_t  where = c->start; /* main pointer to prevent fread() */
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    memset(pattern_address, 0, sizeof pattern_address);
    memset(pattern_address_real, 0, sizeof pattern_address_real);
    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* read pattern table address */
    pattern_table_address = be32s(c, where);
    where += 4;

    /* read patmax */
    pat_max = IN(where++);

    /* read size of pattern table */
    pat_pos = IN(where++);

    /* write title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 20);

    for (i = 0; i < 31; i++)
    {
        pw_fwrite(out, whatever, 22);

        /* read sample size */
        whole_sample_size += (((IN(where + 4) * 256) + IN(where + 5)) * 2);
        pw_fwrite_in(out, where + 4, 2);

        /* write fine */
        whatever[32] = (uint8_t)(((IN(where) * 256) + IN(where + 1)) / 0x48);
        pw_fwrite(out, &whatever[32], 1);

        /* write volume */
        pw_fwrite_in(out, where + 3, 1);

        /* read sample start address */
        k = be32s(c, where + 8);

        if (k < sample_data_address)
            sample_data_address = k;

        /* read loop start address */
        j = be32s(c, where + 12);
        j = (int32_t)((uint32_t)j - (uint32_t)k);
        j /= 2;

        /* write loop start */
        /* htonl(j), then bytes 2 and 3: the low 16 bits, big-endian */
        whatever[48] = (uint8_t)((uint32_t)j >> 8);
        whatever[49] = (uint8_t)j;
        pw_fwrite(out, &whatever[48], 2);

        /* write loop size */
        pw_fwrite_in(out, where + 6, 2);

        where += 16;
    }

    /* write size of pattern list */
    pw_fwrite(out, &pat_pos, 1);

    /* write ntk byte */
    whatever[0] = 0x7f;
    pw_fwrite(out, whatever, 1);

    /* read pattern table .. (the test keeps pat_pos <= 0x7f) */
    where = (int32_t)((uint32_t)c->start + (uint32_t)pattern_table_address);
    for (i = 0; (i < pat_pos) && (i < 128); i++)
    {
        pattern_address[i] = be32s(c, where);
        where += 4;
    }

    /* deduce pattern list */
    whatever[256] = 0x00;
    for (i = 0; (i < pat_pos) && (i < 128); i++)
    {
        if (i == 0)
        {
            whatever[0] = 0x00;
            pattern_address_real[0] = pattern_address[0];
            whatever[256] += 0x01;
            continue;
        }
        for (j = 0; j < i; j++)
        {
            if (pattern_address[i] == pattern_address[j])
            {
                whatever[i] = whatever[j];
                break;
            }
        }
        if (j == i)
        {
            if (whatever[256] < 128)
                pattern_address_real[whatever[256]] = pattern_address[i];
            whatever[i] = whatever[256];
            whatever[256] += 0x01;
        }
    }

    /* write pattern table */
    pw_fwrite(out, whatever, 128);

    /* write ptk's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* pattern data */
    for (i = 0; i <= pat_max; i++)
    {
        memset(whatever, 0, sizeof whatever);
        /* pat_max is not tested and goes up to 255: upstream then read past
         * its 128-entry local array; such entries read 0 here */
        where = (int32_t)((uint32_t)c->start + (uint32_t)((i < 128) ? pattern_address_real[i] : 0));
        for (j = 0; j < 256; j++)
        {
            /* the loop only ends on a row byte of 0xFF (j = row byte, then
             * j++). Past the end of the input every byte reads 0, so it
             * never ends (upstream ran off its buffer and crashed): stop. */
            if (where >= c->in_size)
                break;
            /* Before the input every byte reads 0 too: each such row just
             * stores zeros in row 0 and sets j to 0. Do them all at once. */
            if ((int64_t)where + 3 < 0)
            {
                where = (int32_t)((int64_t)where + 4 * ((-((int64_t)where + 3) + 3) / 4));
                whatever[0] = whatever[1] = whatever[2] = whatever[3] = 0;
                j = 0;
                continue;
            }

            note = (IN(where + 1) & 0x7f) / 2;
            fx_val = IN(where + 3);
            smp = (uint8_t)(((IN(where + 1) << 4) & 0x10) | ((IN(where + 2) >> 4) & 0x0f));
            fx = IN(where + 2) & 0x0f;

            k = IN(where);
            whatever[k * 4] = smp & 0xf0;
            whatever[k * 4] |= poss_at(poss, note, 0);
            whatever[k * 4 + 1] = poss_at(poss, note, 1);
            whatever[k * 4 + 2] = (uint8_t)(fx | ((smp << 4) & 0xf0));
            whatever[k * 4 + 3] = fx_val;
            j = IN(where);
            where += 4;
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    pw_fwrite_in(out, (int32_t)((uint32_t)c->start + (uint32_t)sample_data_address), whole_sample_size);

    /* crap ... */
    pw_crap("    ZEN Packer    ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
