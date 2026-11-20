/*
 * XannPlayer.c - XANN Packer (upstream r/XannPlayer.c).
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

int16_t pw_test_XANN(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 3)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 3);
    for (c->l = 0; c->l < 128; c->l++)
    {
        c->j = be32u(c, c->start + c->l * 4);
        c->k = (c->j / 4) * 4;
        if ((c->k != c->j) || (c->j > 132156))
            return PW_BAD;
    }

    /* test 3 */
    if ((c->in_size - c->start) < 2108)
        return PW_BAD;

    /* test 4 */
    for (c->j = 0; c->j < 64; c->j++)
    {
        if ((IN(c->start + 3 + c->j * 4) != 0x3c) &&
            (IN(c->start + 3 + c->j * 4) != 0x00))
            return PW_BAD;
    }

    /* test 5 */
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
    {
        if (IN(c->start + 519 + 16 * c->j) > 0x40)
            return PW_BAD;
        c->wss += (((IN(c->start + 530 + c->j * 16) * 256) + IN(c->start + 531 + c->j * 16)) * 2);
    }

    /* test #6  (address of samples) */
    for (c->l = 0; c->l < 30; c->l++)
    {
        c->k = be32u(c, c->start + 526 + 16 * c->l);
        c->j = (((IN(c->start + 524 + 16 * c->l) * 256) + IN(c->start + 525 + 16 * c->l)) * 2);
        c->m = be32u(c, c->start + 520 + 16 * (c->l + 1));
        if ((c->k < 2108) || (c->m < 2108))
            return PW_BAD;
        if (c->k > c->m)
            return PW_BAD;
    }

    /* test #7  first pattern data .. */
    for (c->j = 0; c->j < 256; c->j++)
    {
        c->k = (IN(c->start + c->j * 4 + 1085) / 2);
        c->l = c->k * 2;
        if (IN(c->start + c->j * 4 + 1085) != c->l)
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_XANN(pw_ctx *c)
{
    /* c->wss is the whole sample size */

    c->k = 0;
    for (c->j = 0; c->j < 64; c->j++)
    {
        c->l = IN(c->start + 2 + c->j * 4);
        if (c->l > c->k)
            c->k = c->l;
    }
    c->k /= 4;
    c->out_size = c->wss + 1084 + (c->k * 1024);

    c->convert = PW_GOOD;
    pw_save_rip(c, "Xann Packer module", PWF_XANN_packer);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 4); /* 3 should do but call it "just to be sure" :) */
}

/*
 *   XANN_Packer.c   1997 (c) Asle / ReDoX
 *
 * XANN Packer to Protracker.
 *
 *
 * 13 april 1999 : Update
 *   - no more open() of input file ... so no more fread() !.
 *     It speeds-up the process quite a bit :).
 * 28 Nov 1999 : Update
 *   - Speed & Size optimizings
 * Another Update : 26 nov 2003
 *   - used htonl() so that use of addy is now portable on 68k archs
 */
#define SMP_DESC_ADDRESS 0x206
#define PAT_DATA_ADDRESS 0x43C

void pw_depack_XANN(pw_ctx *c)
{
    uint8_t  c1 = 0x00, c2 = 0x00;
    uint8_t  poss[37][2];
    uint8_t  max = 0x00;
    uint8_t  note, smp, fx, fx_val;
    uint8_t  whatever[1024];
    int32_t  i = 0, j = 0, l = 0;
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

    /* 31 samples */
    where = c->start + SMP_DESC_ADDRESS;
    for (i = 0; i < 31; i++)
    {
        /* sample name */
        pw_fwrite(out, &whatever[2], 22);

        /* read loop start address */
        j = be32s(c, where + 2);

        /* read sample address */
        l = be32s(c, where + 8);

        /* read & write sample size */
        pw_fwrite_in(out, where + 12, 2);
        whole_sample_size += (((IN(where + 12) * 256) + IN(where + 13)) * 2);

        /* calculate loop start value */
        j = (int32_t)((uint32_t)j - (uint32_t)l);

        /* write fine & vol */
        pw_fwrite_in(out, where, 2);

        /* write loop start */
        /* htonl(j), then bytes 2 and 3: the low 16 bits, big-endian */
        j /= 2;
        whatever[0] = (uint8_t)((uint32_t)j >> 8);
        whatever[1] = (uint8_t)j;
        pw_fwrite(out, whatever, 2);

        /* write loop size */
        pw_fwrite_in(out, where + 6, 2);

        where += 16;
    }

    /* pattern table */
    max = 0x00;
    where = c->start;
    for (c1 = 0; c1 < 128; c1++)
    {
        l = be32s(c, where);
        where += 4;
        if (l == 0)
            break;
        whatever[c1] = (uint8_t)((((int32_t)((uint32_t)l - 0x3c)) / 1024) - 1);
        if (whatever[c1] > max)
            max = whatever[c1];
    }
    max += 1; /* starts at $00 */

    /* write number of pattern */
    pw_fwrite(out, &c1, 1);

    /* write noisetracker byte */
    c1 = 0x7f;
    pw_fwrite(out, &c1, 1);

    /* write pattern list */
    pw_fwrite(out, whatever, 128);

    /* write Protracker's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* pattern data */
    where = c->start + PAT_DATA_ADDRESS;
    for (i = 0; i < max; i++)
    {
        memset(whatever, 0, sizeof whatever);
        for (j = 0; j < 256; j++)
        {
            smp = (IN(where + j * 4) >> 3) & 0x1f;
            note = IN(where + j * 4 + 1);
            fx = IN(where + j * 4 + 2);
            fx_val = IN(where + j * 4 + 3);
            switch (fx)
            {
            case 0x00: /* no Fx */
                fx = 0x00;
                break;

            case 0x04: /* arpeggio */
                fx = 0x00;
                break;

            case 0x08: /* portamento up */
                fx = 0x01;
                break;

            case 0x0C: /* portamento down */
                fx = 0x02;
                break;

            case 0x10: /* tone portamento with no FxVal */
                fx = 0x03;
                break;

            case 0x14: /* tone portamento */
                fx = 0x03;
                break;

            case 0x18: /* vibrato with no FxVal */
                fx = 0x04;
                break;

            case 0x1C: /* vibrato */
                fx = 0x04;
                break;

            case 0x24: /* tone portamento + vol slide DOWN */
                fx = 0x05;
                break;

            case 0x28: /* vibrato + volume slide UP */
                fx = 0x06;
                c1 = (uint8_t)((fx_val << 4) & 0xf0);
                c2 = (fx_val >> 4) & 0x0f;
                fx_val = c1 | c2;
                break;

            case 0x2C: /* vibrato + volume slide DOWN */
                fx = 0x06;
                break;

            case 0x38: /* sample offset */
                fx = 0x09;
                break;

            case 0x3C: /* volume slide up */
                fx = 0x0A;
                c1 = (uint8_t)((fx_val << 4) & 0xf0);
                c2 = (fx_val >> 4) & 0x0f;
                fx_val = c1 | c2;
                break;

            case 0x40: /* volume slide down */
                fx = 0x0A;
                break;

            case 0x44: /* position jump */
                fx = 0x0B;
                break;

            case 0x48: /* set volume */
                fx = 0x0C;
                break;

            case 0x4C: /* pattern break */
                fx = 0x0D;
                break;

            case 0x50: /* set speed */
                fx = 0x0F;
                break;

            case 0x58: /* set filter */
                fx = 0x0E;
                fx_val = 0x01;
                break;

            case 0x5C: /* fine slide up */
                fx = 0x0E;
                fx_val |= 0x10;
                break;

            case 0x60: /* fine slide down */
                fx = 0x0E;
                fx_val |= 0x20;
                break;

            case 0x74: /* set loop start */
            case 0x78: /* set loop value */
                fx = 0x0E;
                fx_val |= 0x60;
                break;

            case 0x84: /* retriger */
                fx = 0x0E;
                fx_val |= 0x90;
                break;

            case 0x88: /* fine volume slide up */
                fx = 0x0E;
                fx_val |= 0xa0;
                break;

            case 0x8C: /* fine volume slide down */
                fx = 0x0E;
                fx_val |= 0xb0;
                break;

            case 0x94: /* note delay */
                fx = 0x0E;
                fx_val |= 0xd0;
                break;

            case 0x98: /* pattern delay */
                fx = 0x0E;
                fx_val |= 0xe0;
                break;

            default:
                fx = 0x00;
                break;
            }
            whatever[j * 4] = (smp & 0xf0);
            whatever[j * 4] |= poss_at(poss, note / 2, 0);
            whatever[j * 4 + 1] = poss_at(poss, note / 2, 1);
            whatever[j * 4 + 2] = (uint8_t)((smp << 4) & 0xf0);
            whatever[j * 4 + 2] |= fx;
            whatever[j * 4 + 3] = fx_val;
        }
        where += 1024;
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    pw_crap("   XANN Packer    ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
