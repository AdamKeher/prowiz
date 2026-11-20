/*
 * NoiseRunner.c - NoiseRunner (upstream r/NoiseRunner.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_Noiserunner(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1080);
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->j = (((IN(c->start + 6 + c->k * 16) * 256) + IN(c->start + 7 + c->k * 16)) * 2);
        if (c->j > 0xFFFF)
            return PW_BAD;
        /* volumes */
        if (IN(c->start + 1 + c->k * 16) > 0x40)
            return PW_BAD;
        c->wss += c->j;
    }
    if (c->wss == 0)
        return PW_BAD;
    /* c->wss is the size of all the sample data */

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

    /* test if we read outside of the file */
    if ((c->start + c->k * 256) > (uint32_t)c->in_size)
        return PW_BAD;

    /* test #5 pattern data ... */
    for (c->j = 0; c->j < (c->k * 256); c->j++)
    {
        /* note > 48h ? */
        if (IN(c->start + 1086 + c->j * 4) > 0x48)
            return PW_BAD;
        c->l = IN(c->start + 1087 + c->j * 4);
        if (((c->l / 8) * 8) != c->l)
            return PW_BAD;
        c->l = IN(c->start + 1084 + c->j * 4);
        if (((c->l / 4) * 4) != c->l)
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_Noiserunner(pw_ctx *c)
{
    /* c->k is still the nbr of pattern */
    c->out_size = c->wss + (c->k * 1024) + 1084;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Noiserunner music", PWF_Noiserunner);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1081); /* 1080 should do but call it "just to be sure" :) */
}

/*
 *   NoiseRunner.c   1997 (c) Asle / ReDoX
 *
 * NoiseRunner to Protracker.
 *
 * Last revision : 26/11/1999 by Sylvain "Asle" Chipaux
 *                 reduced to only one FREAD.
 *                 Speed-up, Clean-up and Binary smaller.
 * update: 01/12/99
 *   - removed fopen() and attached funcs
 * Another Update : 26 nov 2003
 *   - used htonl() so that use of addy is now portable on 68k archs
 */

#define PAT_DATA_ADDRESS 0x43C

void pw_depack_Noiserunner(pw_ctx *c)
{
    uint8_t  poss[37][2];
    uint8_t  max = 0x00;
    uint8_t  note, smp, fx, fxval;
    uint8_t  whatever[1024];
    int32_t  where = c->start;
    int32_t  i = 0, j = 0, l = 0;
    int32_t  whole_sample_size = 0;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    pw_fill_ptk_table(poss);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(whatever, 0, sizeof whatever);

    /* title */
    pw_fwrite(out, whatever, 20);

    /* 31 samples */
    for (i = 0; i < 31; i++)
    {
        /* sample name */
        pw_fwrite(out, whatever, 22);

        /* read sample address */
        j = (int32_t)(((uint32_t)IN(where + 2) << 24) +
                      ((uint32_t)IN(where + 3) << 16) +
                      ((uint32_t)IN(where + 4) << 8) +
                      IN(where + 5));

        /* read and write sample size */
        pw_fwrite_in(out, where + 6, 2);
        whole_sample_size += (((IN(where + 6) * 256) + IN(where + 7)) * 2);

        /* read loop start address */
        l = (int32_t)(((uint32_t)IN(where + 8) << 24) +
                      ((uint32_t)IN(where + 9) << 16) +
                      ((uint32_t)IN(where + 10) << 8) +
                      IN(where + 11));

        /* calculate loop start value */
        j = (int32_t)((uint32_t)l - (uint32_t)j);

        /* read finetune ?!? */
        whatever[32] = IN(where + 14);
        whatever[33] = IN(where + 15);
        if (whatever[32] > 0xf0)
        {
            if ((whatever[32] == 0xFB) && (whatever[33] == 0xC8))
                whatever[32] = 0x0f;
            if ((whatever[32] == 0xFC) && (whatever[33] == 0x10))
                whatever[32] = 0x0E;
            if ((whatever[32] == 0xFC) && (whatever[33] == 0x58))
                whatever[32] = 0x0D;
            if ((whatever[32] == 0xFC) && (whatever[33] == 0xA0))
                whatever[32] = 0x0C;
            if ((whatever[32] == 0xFC) && (whatever[33] == 0xE8))
                whatever[32] = 0x0B;
            if ((whatever[32] == 0xFD) && (whatever[33] == 0x30))
                whatever[32] = 0x0A;
            if ((whatever[32] == 0xFD) && (whatever[33] == 0x78))
                whatever[32] = 0x09;
            if ((whatever[32] == 0xFD) && (whatever[33] == 0xC0))
                whatever[32] = 0x08;
            if ((whatever[32] == 0xFE) && (whatever[33] == 0x08))
                whatever[32] = 0x07;
            if ((whatever[32] == 0xFE) && (whatever[33] == 0x50))
                whatever[32] = 0x06;
            if ((whatever[32] == 0xFE) && (whatever[33] == 0x98))
                whatever[32] = 0x05;
            if ((whatever[32] == 0xFE) && (whatever[33] == 0xE0))
                whatever[32] = 0x04;
            if ((whatever[32] == 0xFF) && (whatever[33] == 0x28))
                whatever[32] = 0x03;
            if ((whatever[32] == 0xFF) && (whatever[33] == 0x70))
                whatever[32] = 0x02;
            if ((whatever[32] == 0xFF) && (whatever[33] == 0xB8))
                whatever[32] = 0x01;
        }
        else
            whatever[32] = 0x00;

        /* write fine */
        pw_fwrite(out, &whatever[32], 1);

        /* write vol */
        pw_fwrite_in(out, where + 1, 1);

        /* write loop start */
        /* The original did k = htonl(j) and wrote byte 2 of k and byte 3 of
         * l as they lie in (little-endian) memory: bits 8-15 of j and bits
         * 24-31 of l. */
        j /= 2;
        whatever[32] = (uint8_t)((uint32_t)j >> 8);
        whatever[33] = (uint8_t)((uint32_t)l >> 24);
        pw_fwrite(out, &whatever[32], 2);

        /* write loop size */
        pw_fwrite_in(out, where + 12, 2);
        where += 16;
    }

    /* pattablesiz & Ntk Byte & pattern table */
    where = c->start + 950;
    pw_fwrite_in(out, where, 130);
    where += 2;

    max = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(where) > max)
            max = IN(where);
        where += 1;
    }
    max += 1; /* starts at $00 */

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
            smp = (IN(where + j * 4 + 3) >> 3) & 0x1f;
            note = IN(where + j * 4 + 2);
            fx = IN(where + j * 4);
            fxval = IN(where + j * 4 + 1);
            switch (fx)
            {
            case 0x00: /* tone portamento */
                fx = 0x03;
                break;
            case 0x04: /* slide up */
                fx = 0x01;
                break;
            case 0x08: /* slide down */
                fx = 0x02;
                break;
            case 0x0C: /* no Fx */
                fx = 0x00;
                break;
            case 0x10: /* set vibrato */
                fx = 0x04;
                break;
            case 0x14: /* portamento + volume slide */
                fx = 0x05;
                break;
            case 0x18: /* vibrato + volume slide */
                fx = 0x06;
                break;
            case 0x20: /* set panning ?!?!? not PTK ! Heh, Gryzor ... */
                fx = 0x08;
                break;
            case 0x24: /* sample offset */
                fx = 0x09;
                break;
            case 0x28: /* volume slide */
                fx = 0x0A;
                break;
            case 0x30: /* set volume */
                fx = 0x0C;
                break;
            case 0x34: /* pattern break */
                fx = 0x0D;
                break;
            case 0x38: /* extended command */
                fx = 0x0E;
                break;
            case 0x3C: /* set speed */
                fx = 0x0F;
                break;
            default:
                fx = 0x00;
                break;
            }
            whatever[j * 4] = (smp & 0xf0);
            /* poss[] has 37 entries; note / 2 can reach 127 */
            whatever[j * 4] |= (note / 2 < 37) ? poss[note / 2][0] : 0;
            whatever[j * 4 + 1] = (note / 2 < 37) ? poss[note / 2][1] : 0;
            whatever[j * 4 + 2] = (uint8_t)((smp << 4) & 0xf0);
            whatever[j * 4 + 2] |= fx;
            whatever[j * 4 + 3] = fxval;
        }
        where += 1024;
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    pw_crap("   Noiserunner    ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
