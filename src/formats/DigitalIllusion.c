/*
 * DigitalIllusion.c - Digital Illusion packed music (upstream r/DigitalIllusion.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* (3rd of April 2000)
 *   bugs pointed out by Thomas Neumann .. thx :)
 * (May 2002)
 *   added test_smps()
 */
int16_t pw_test_DI(pw_ctx *c)
{
    /* test #1 */
    if (c->i < 17)
        return PW_BAD;

    /* test #2  (number of sample) */
    c->start = (int32_t)(c->i - 17);
    c->k = (IN(c->start) * 256) + IN(c->start + 1);
    if (c->k > 31)
        return PW_BAD;

    /* test #3 (finetunes and whole sample size) */
    /* c->k = number of samples */
    c->wss = 0;
    c->l = 0;
    for (c->j = 0; c->j < c->k; c->j++)
    {
        c->o = (((IN(c->start + (c->j * 8) + 14) * 256) + IN(c->start + (c->j * 8) + 15)) * 2);
        c->m = (((IN(c->start + (c->j * 8) + 18) * 256) + IN(c->start + (c->j * 8) + 19)) * 2);
        c->n = (((IN(c->start + (c->j * 8) + 20) * 256) + IN(c->start + (c->j * 8) + 21)) * 2);
        if ((c->o > 0xffff) ||
            (c->m > 0xffff) ||
            (c->n > 0xffff))
            return PW_BAD;
        if (pw_test_smps((int32_t)c->o, (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 17 + c->j * 8), IN(c->start + 16 + c->j * 8)) == PW_BAD)
            return PW_BAD;
        /* gets total size of samples */
        c->wss += c->o;
    }
    if (c->wss <= 2)
        return PW_BAD;

    /* test #4 (addresses of pattern in file ... possible ?) */
    /* c->wss is the whole sample size */
    /* c->k is still the number of sample */
    c->m = c->k;
    c->j = (IN(c->start + 2) * 256 * 256 * 256)
         + (IN(c->start + 3) * 256 * 256)
         + (IN(c->start + 4) * 256)
         + IN(c->start + 5);
    /* c->j is the address of pattern table now */
    c->k = (IN(c->start + 6) * 256 * 256 * 256)
         + (IN(c->start + 7) * 256 * 256)
         + (IN(c->start + 8) * 256)
         + IN(c->start + 9);
    /* c->k is the address of the pattern data */
    c->l = (IN(c->start + 10) * 256 * 256 * 256)
         + (IN(c->start + 11) * 256 * 256)
         + (IN(c->start + 12) * 256)
         + IN(c->start + 13);
    /* c->l is the address of the sample data */
    if ((c->k <= c->j) || (c->l <= c->j) || (c->l <= c->k))
        return PW_BAD;
    if ((c->k - c->j) > 128)
        return PW_BAD;
    if ((c->k > (uint32_t)c->in_size) || (c->l > (uint32_t)c->in_size) || (c->j > (uint32_t)c->in_size))
        return PW_BAD;

    /* test #4,1 :) */
    c->m *= 8;
    c->m += 2;
    if (c->j < c->m)
        return PW_BAD;

    /* test #5 */
    if ((c->k + (uint32_t)c->start) > (uint32_t)c->in_size)
        return PW_BAD;

    /* test pattern table reliability */
    for (c->m = c->j; c->m < (c->k - 1); c->m++)
    {
        if (IN(c->start + c->m) > 0x80)
            return PW_BAD;
    }
    /* test #6  ($FF at the end of pattern list ?) */
    if (IN(c->start + c->k - 1) != 0xFF)
        return PW_BAD;

    /* test #7 (addres of sample data > $FFFF ? ) */
    /* c->l is still the address of the sample data */
    if (c->l > 65535)
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_DI(pw_ctx *c)
{
    /* c->wss is already the whole sample size */

    c->j = (IN(c->start + 10) * 256 * 256 * 256)
         + (IN(c->start + 11) * 256 * 256)
         + (IN(c->start + 12) * 256)
         + IN(c->start + 13);

    c->out_size = c->wss + c->j;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Digital Illusion Packed music", PWF_Digital_illusion);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 15); /* 14 should do but call it "just to be sure" :) */
}

/*
 *   Digital_Illusion.c   1997 (c) Asle / ReDoX
 *
 * Converts DI packed MODs back to PTK MODs
 * thanks to Gryzor and his ProWizard tool ! ... without it, this prog
 * would not exist !!!
 *
 * 20100119 : cleaned up a bit - header is one fwrite.
 */
void pw_depack_DI(pw_ctx *c)
{
    uint8_t  note, smp, fx, fxval;
    uint8_t  poss[37][2];
    uint8_t  whatever[1085];
    uint8_t  c1;
    int32_t  i = 0, k = 0;
    uint16_t pattern_addresses_table[128];
    int32_t  add_pattern_table = 0;
    int32_t  add_pattern_data = 0;
    int32_t  add_sample_data = 0;
    int32_t  total_sample_size = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    pw_fill_ptk_table(poss);

    memset(pattern_addresses_table, 0, sizeof pattern_addresses_table);

    /* title */
    memset(whatever, 0, sizeof whatever);

    k = (IN(where) * 256) + IN(where + 1);
    where += 2;

    add_pattern_table = (IN(where + 1) * 256 * 256) +
                        (IN(where + 2) * 256) +
                        IN(where + 3);
    where += 4;

    add_pattern_data = (IN(where + 1) * 256 * 256) +
                       (IN(where + 2) * 256) +
                       IN(where + 3);
    where += 4;
    pw_printf(c, "Pattern data address : %d\n", add_pattern_data);

    add_sample_data = (IN(where + 1) * 256 * 256) +
                      (IN(where + 2) * 256) +
                      IN(where + 3);
    where += 4;
    pw_printf(c, "Sample data address : %d\n", add_sample_data);

    /* k <= 31 (checked by the test; the bound only keeps the buffer safe) */
    for (i = 0; i < k && i < 31; i++)
    {
        total_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);
        whatever[i * 30 + 42] = IN(where);
        whatever[i * 30 + 43] = IN(where + 1);
        whatever[i * 30 + 44] = IN(where + 2);
        whatever[i * 30 + 45] = IN(where + 3);
        whatever[i * 30 + 46] = IN(where + 4);
        whatever[i * 30 + 47] = IN(where + 5);
        whatever[i * 30 + 48] = IN(where + 6);
        whatever[i * 30 + 49] = IN(where + 7);
        where += 8;
    }

    for (i = k; i < 31; i++)
        whatever[i * 30 + 49] = 0x01;

    k = where;

    /* the test guarantees a 0xff within 128 bytes of the table start; the
     * bounds are a safety net only */
    where = c->start + add_pattern_table;
    i = 0;
    c1 = 0x00;
    while (IN(where) != 0xff && where < c->in_size)
    {
        if (i + 952 < (int32_t)sizeof whatever)
            whatever[i + 952] = IN(where);
        if (IN(where) > c1)
            c1 = IN(where);
        i += 1;
        where += 1;
    }
    whatever[950] = (uint8_t)i;
    whatever[951] = 0x7f;

    pw_printf(c, "\nHighest pattern number : %d (Where:%x)\n", c1, (unsigned)where);

    whatever[1080] = 'M';
    whatever[1081] = '.';
    whatever[1082] = 'K';
    whatever[1083] = '.';

    pw_fwrite(out, whatever, 1084);

    /* c1 can be 0x80: the original then wrote and read one entry past the
     * 128-entry table */
    where = k;
    for (i = 0; i <= c1; i++)
    {
        if (i < 128)
            pattern_addresses_table[i] = (uint16_t)((IN(where) * 256) + IN(where + 1));
        where += 2;
    }

    for (i = 0; i <= c1; i++)
    {
        where = c->start + ((i < 128) ? pattern_addresses_table[i] : 0);
        for (k = 0; k < 256; k++) /* 256 = 4(voices) * 64(rows) */
        {
            whatever[0] = whatever[1] = whatever[2] = whatever[3] = 0x00;
            whatever[10] = IN(where++);
            if ((whatever[10] & 0x80) == 0)
            {
                whatever[11] = IN(where++);
                note = (uint8_t)(((whatever[10] << 4) & 0x30) | ((whatever[11] >> 4) & 0x0f));
                /* poss[] has 37 entries; note can reach 63 */
                whatever[0] = (note < 37) ? poss[note][0] : 0;
                whatever[1] = (note < 37) ? poss[note][1] : 0;
                smp = (uint8_t)((whatever[10] >> 2) & 0x1f);
                whatever[0] |= (smp & 0xf0);
                whatever[2] = (uint8_t)((smp << 4) & 0xf0);
                fx = (uint8_t)(whatever[11] & 0x0f);
                whatever[2] |= fx;
                fxval = 0x00;
                whatever[3] = fxval;
                pw_fwrite(out, whatever, 4);
                continue;
            }
            if (whatever[10] == 0xff)
            {
                whatever[0] = whatever[1] = whatever[2] = whatever[3] = 0x00;
                pw_fwrite(out, whatever, 4);
                continue;
            }
            whatever[11] = IN(where++);
            whatever[12] = IN(where++);
            note = (uint8_t)(((whatever[10] << 4) & 0x30) | ((whatever[11] >> 4) & 0x0f));
            whatever[0] = (note < 37) ? poss[note][0] : 0;
            whatever[1] = (note < 37) ? poss[note][1] : 0;
            smp = (uint8_t)((whatever[10] >> 2) & 0x1f);
            whatever[0] |= (smp & 0xf0);
            whatever[2] = (uint8_t)((smp << 4) & 0xf0);
            fx = (uint8_t)(whatever[11] & 0x0f);
            whatever[2] |= fx;
            fxval = whatever[12];
            whatever[3] = fxval;
            pw_fwrite(out, whatever, 4);
        }
    }

    where = c->start + add_sample_data;
    pw_fwrite_in(out, where, total_sample_size);

    pw_crap(" Digital Illusion ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
