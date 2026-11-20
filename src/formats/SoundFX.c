/*
 * SoundFX.c - Sound FX 1.3 module (upstream r/SoundFX.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_SoundFX13(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 0x3C)
        return PW_BAD;

    /* test 2 */
    /* samples tests */
    c->start = (int32_t)(c->i - 0x3C);
    for (c->k = 0; c->k < 15; c->k++)
    {
        /* size */
        c->j = ((IN(c->start + c->k * 4 + 2) * 256) + IN(c->start + c->k * 4 + 3));
        /* loop start */
        c->m = ((IN(c->start + 106 + c->k * 30) * 256) + IN(c->start + 107 + c->k * 30));
        /* loop size */
        c->n = (((IN(c->start + 108 + c->k * 30) * 256) + IN(c->start + 109 + c->k * 30)) * 2);

        /* size,loopstart,replen > 64k ? */
        if ((c->j > 0xFFFF) || (c->m > 0xFFFF) || (c->n > 0xFFFF))
            return PW_BAD;
        /* replen > size ? */
        if (c->n > (c->j + 2))
            return PW_BAD;
        /* loop start > size ? */
        if (c->m > c->j)
            return PW_BAD;
        /* loop size =0 & loop start != 0 ?*/
        if ((c->m != 0) && (c->n == 0))
            return PW_BAD;
        /* size & loopstart !=0 & size=loopstart ? */
        if ((c->j != 0) && (c->j == c->m))
            return PW_BAD;
        /* size =0 & loop start !=0 */
        if ((c->j == 0) && (c->m != 0))
            return PW_BAD;
    }

    /* get real whole sample size */
    c->wss = 0;
    for (c->j = 0; c->j < 15; c->j++)
    {
        c->k = ((IN(c->start + c->j * 4) * 256 * 256 * 256) +
                (IN(c->start + c->j * 4 + 1) * 256 * 256) +
                (IN(c->start + c->j * 4 + 2) * 256) +
                IN(c->start + c->j * 4 + 3));
        if (c->k > 131072)
            return PW_BAD;
        c->wss += c->k;
    }

    /* test #3  finetunes & volumes */
    for (c->k = 0; c->k < 15; c->k++)
    {
        if (IN(c->start + 105 + c->k * 30) > 0x40)
            return PW_BAD;
    }

    /* test #4  pattern list size */
    c->l = IN(c->start + 0x212);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;
    /* c->l holds the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < c->l; c->j++)
    {
        if (IN(c->start + 0x214 + c->j) > c->k)
            c->k = IN(c->start + 0x214 + c->j);
        if (IN(c->start + 0x214 + c->j) > 127)
            return PW_BAD;
    }
    /* c->k is the number of pattern in the file (-1) */
    c->k += 1;

    /* test #5 pattern data ... */
    if (((c->k * 1024) + 0x294 + c->start) > (uint32_t)c->in_size)
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_SoundFX13(pw_ctx *c)
{
    /* c->k is still the nbr of pattern */
    /* c->wss is the WholeSampleSize :) */

    c->out_size = c->wss + (c->k * 1024) + 0x294;

    c->convert = PW_BAD;

    pw_save_rip(c, "Sound FX 1.3 module", PWF_SoundFX);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 0x40); /* 0x3C should do but call it "just to be sure" :) */
}

/*
 *   SoundFX.c   1999 (c) Sylvain "Asle" Chipaux
 *
 * Depacks musics in the SoundFX format and saves in ptk.
 * It works on the rip file that was just saved.
 */
void pw_depack_SoundFX13(pw_ctx *c)
{
    uint8_t  whatever[1024];
    uint8_t *smp;
    uint8_t  c0 = 0x00, c1 = 0x00, c2 = 0x00, c3 = 0x00;
    uint8_t  max = 0x00;
    uint8_t  pat_pos = 0;
    int32_t  whole_sample_size = 0;
    int32_t  i = 0, j = 0;
    pw_file *in, *out;

    if (c->save_status == PW_BAD)
        return;

    in = pw_fopen_last_rip(c);
    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 20);

    /* read and write whole header */
    for (i = 0; i < 15; i++)
    {
        pw_fseek(in, 0x50 + i * 30, 0);
        /* write name */
        for (j = 0; j < 22; j++)
        {
            pw_fread(&c1, 1, 1, in);
            pw_fwrite(out, &c1, 1);
        }
        /* size */
        pw_fseek(in, i * 4 + 1, 0);
        pw_fread(&c0, 1, 1, in);
        pw_fread(&c1, 1, 1, in);
        pw_fread(&c2, 1, 1, in);
        c2 /= 2;
        c3 = c1 / 2;
        if ((c3 * 2) != c1)
            c2 += 0x80;
        if (c0 != 0x00)
            c3 += 0x80;
        pw_fseek(in, 0x50 + i * 30 + 24, 0);
        pw_fwrite(out, &c3, 1);
        pw_fwrite(out, &c2, 1);
        whole_sample_size += (((c3 * 256) + c2) * 2);
        /* finetune */
        pw_fread(&c1, 1, 1, in);
        pw_fwrite(out, &c1, 1);
        /* volume */
        pw_fread(&c1, 1, 1, in);
        pw_fwrite(out, &c1, 1);
        /* loop start */
        pw_fread(&c1, 1, 1, in);
        pw_fread(&c2, 1, 1, in);
        c2 /= 2;
        c3 = c1 / 2;
        if ((c3 * 2) != c1)
            c2 += 0x80;
        pw_fwrite(out, &c3, 1);
        pw_fwrite(out, &c2, 1);
        /* loop size */
        pw_fread(&c1, 1, 1, in);
        pw_fread(&c2, 1, 1, in);
        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c2, 1);
    }
    memset(whatever, 0, 30);
    whatever[29] = 0x01;
    for (i = 0; i < 16; i++)
        pw_fwrite(out, whatever, 30);

    /* pattern list size */
    pw_fread(&pat_pos, 1, 1, in);
    pw_fwrite(out, &pat_pos, 1);

    /* ntk byte */
    pw_fseek(in, 1, 1);
    c1 = 0x7f;
    pw_fwrite(out, &c1, 1);

    /* read and write pattern list */
    max = 0x00;
    for (i = 0; i < pat_pos; i++)
    {
        pw_fread(&c1, 1, 1, in);
        pw_fwrite(out, &c1, 1);
        if (c1 > max)
            max = c1;
    }
    c1 = 0x00;
    while (i != 128)
    {
        pw_fwrite(out, &c1, 1);
        i += 1;
    }

    /* write ID */
    c1 = 'M';
    c2 = '.';
    c3 = 'K';
    pw_fwrite(out, &c1, 1);
    pw_fwrite(out, &c2, 1);
    pw_fwrite(out, &c3, 1);
    pw_fwrite(out, &c2, 1);

    /* pattern data */
    pw_fseek(in, 0x294, 0);
    for (i = 0; i <= max; i++)
    {
        memset(whatever, 0, sizeof whatever);
        pw_fread(whatever, 1024, 1, in);
        for (j = 0; j < 256; j++)
        {
            if (whatever[(j * 4)] == 0xff)
            {
                if (whatever[(j * 4) + 1] != 0xfe)
                    pw_printf(c, "Volume unknown : (at:%d) (fx:%x,%x,%x,%x)\n", (int)pw_ftell(in),
                              whatever[(j * 4)], whatever[(j * 4) + 1],
                              whatever[(j * 4) + 2], whatever[(j * 4) + 3]);
                whatever[(j * 4)]     = 0x00;
                whatever[(j * 4) + 1] = 0x00;
                whatever[(j * 4) + 2] = 0x0C;
                whatever[(j * 4) + 3] = 0x00;
                continue;
            }
            switch (whatever[(j * 4) + 2] & 0x0f)
            {
            case 1: /* arpeggio */
                whatever[(j * 4) + 2] &= 0xF0;
                break;
            case 7: /* slide up */
            case 8: /* slide down */
                whatever[(j * 4) + 2] -= 0x06;
                break;
            case 3: /* empty ... same as followings ... but far too much to "printf" it */
            case 6: /* and Noiseconverter puts 00 instead ... */
                whatever[(j * 4) + 2] &= 0xF0;
                whatever[(j * 4) + 3] = 0x00;
                break;
            case 2:
            case 4:
            case 5:
            case 9:
            case 0x0a:
            case 0x0b:
            case 0x0c:
            case 0x0d:
            case 0x0e:
            case 0x0f:
                pw_printf(c, "unsupported effect : (at:%d) (fx:%d)\n", (int)pw_ftell(in),
                          whatever[(j * 4) + 2] & 0x0f);
                whatever[(j * 4) + 2] &= 0xF0;
                whatever[(j * 4) + 3] = 0x00;
                break;
            default:
                break;
            }
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    smp = (uint8_t *)calloc((size_t)whole_sample_size + 1, 1);
    if (smp == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        pw_fclose(in);
        pw_fdiscard(out);
        return;
    }
    pw_fread(smp, (size_t)whole_sample_size, 1, in);
    pw_fwrite(out, smp, whole_sample_size);
    free(smp);

    /* crap */
    pw_crap("     Sound FX     ", PW_BAD, PW_BAD, out);

    pw_fclose(in);
    pw_fclose(out);

    pw_printf(c, "done\n"
                 "  WARNING: This is only an under development converter !\n"
                 "           output could sound strange...\n");
}
