/*
 * Promizer01.c - Promizer 0.1 (upstream r/Promizer01.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 *
 * - 22 sept 2001 -
 * bugfix : test #5 was fake since first pat addy can be != $00000000
 */
#include <stdio.h>

#include "formats.h"

int16_t pw_test_PM01(pw_ctx *c)
{
    /* test #1 (c->start is still the value left by the previous handler) */
    if ((c->i < 3) || ((c->start + 766) > c->in_size))
        return PW_BAD;

    /* test #2 */
    c->start = (int32_t)(c->i - 3);
    c->l = 0;
    for (c->j = 0; c->j < 31; c->j++)
    {
        c->k = (((IN(c->start + c->j * 8) * 256) + IN(c->start + 1 + c->j * 8)) * 2);
        c->l += c->k;
        /* finetune > 0x0f ? */
        if (IN(c->start + 2 + 8 * c->j) > 0x0f)
            return PW_BAD;
        /* loop start > size ? */
        if ((uint32_t)(((IN(c->start + 4 + c->j * 8) * 256) + IN(c->start + 5 + c->j * 8)) * 2) > c->k)
            return PW_BAD;
    }
    if (c->l <= 2)
        return PW_BAD;

    /* test #3   about size of pattern list */
    c->l = (IN(c->start + 248) * 256) + IN(c->start + 249);
    c->k = c->l / 4;
    if ((c->k * 4) != c->l)
        return PW_BAD;
    if (c->k > 127)
        return PW_BAD;
    if (c->l == 0)
        return PW_BAD;

    /* test #4  size of all the pattern data */
    /* c->k contains the size of the pattern list */
    /* (byte 743 instead of 763: sic) */
    c->l = (IN(c->start + 762) * 256 * 256 * 256) + (IN(c->start + 743) * 256 * 256) +
           (IN(c->start + 764) * 256) + IN(c->start + 765);
    if ((c->l < 1024) || (c->l > 131072) || ((c->l + c->start) > (uint32_t)c->in_size))
        return PW_BAD;

    /* test #5  first pattern address != $00000000 ? */
    /* bugfix : removed coz first addy can be != $00000000 ! */

    /* test #6  pattern addresses */
    /* c->k is still ths size of the pattern list */
    for (c->j = 0; c->j < c->k; c->j++)
    {
        c->l = (IN(c->start + 250 + c->j * 4) * 256 * 256 * 256) + (IN(c->start + 251 + c->j * 4) * 256 * 256) +
               (IN(c->start + 252 + c->j * 4) * 256) + IN(c->start + 253 + c->j * 4);
        if (c->l > 131072)
            return PW_BAD;
        if (((c->l / 1024) * 1024) != c->l)
            return PW_BAD;
    }

    /* test #7  last patterns in pattern table != $00000000 ? */
    c->j += 4; /* just to be sure */
    while (c->j != 128)
    {
        /* With a 125..127 entry list c->j starts past 128 and the original
         * scanned on through memory until it met a non-zero long (or
         * crashed). Past the end of the input it can only read zeros here,
         * so stop there: the result would be BAD (or a crash) anyway. */
        if ((int64_t)(uint32_t)(c->start + 250 + c->j * 4) >= (int64_t)c->in_size)
            return PW_BAD;
        c->l = (IN(c->start + 250 + c->j * 4) * 256 * 256 * 256) + (IN(c->start + 251 + c->j * 4) * 256 * 256) +
               (IN(c->start + 252 + c->j * 4) * 256) + IN(c->start + 253 + c->j * 4);
        if (c->l != 0)
            return PW_BAD;
        c->j += 1;
    }

    /* test #8 : first pattern data */
    for (c->j = 0; c->j < 256; c->j += 4)
    {
        if ((255 - IN(c->start + 766 + c->j)) > 0x13)
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_PM01(pw_ctx *c)
{
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
        c->wss += (((IN(c->start + c->j * 8) * 256) + IN(c->start + 1 + c->j * 8)) * 2);

    c->k = (IN(c->start + 762) * 256 * 256 * 256) + (IN(c->start + 763) * 256 * 256) +
           (IN(c->start + 764) * 256) + IN(c->start + 765);

    c->out_size = c->wss + c->k + 766;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Promizer 0.1 music", PWF_Promizer_01);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 4); /* 3 should do but call it "just to be sure" :) */
}

/*
 *   Promizer_0.1_Packer.c   1997 (c) Asle / ReDoX
 *
 * Converts back to ptk Promizer 0.1 packed MODs
 *
 * ---updates : 2000, the 19th of april
 *  - Small bug correction (pointed out by Thoman Neumann)
 */
void pw_depack_PM01(pw_ctx *c)
{
    uint8_t  c1 = 0x00, c2 = 0x00, c3 = 0x00;
    uint8_t  Pat_Pos;
    uint8_t  poss[37][2];
    uint8_t *Whatever;
    uint8_t *PatternData;
    uint8_t  Smp_Fine_Table[31];
    uint8_t  Old_Smp_Nbr[4];
    uint8_t  fine;
    int32_t  i = 0, j = 0, k = 0, l = 0;
    int32_t  WholeSampleSize = 0;
    int32_t  Pattern_Address[128];
    int32_t  Where = c->start;
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    memset(Pattern_Address, 0, sizeof Pattern_Address);
    memset(Smp_Fine_Table, 0, sizeof Smp_Fine_Table);
    memset(Old_Smp_Nbr, 0, sizeof Old_Smp_Nbr);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* write title */
    Whatever = (uint8_t *)calloc(1024, 1);
    if (Whatever == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        pw_fclose(out);
        return;
    }
    /* title */
    pw_fwrite(out, Whatever, 20);

    /* read and write sample descriptions */
    for (i = 0; i < 31; i++)
    {
        /*sample name*/
        pw_fwrite(out, Whatever, 22);

        WholeSampleSize += (((IN(Where) * 256) + IN(Where + 1)) * 2);
        pw_fwrite_in(out, Where, 2);
        c1 = IN(Where + 2); /* finetune */
        Smp_Fine_Table[i] = c1;
        pw_fwrite(out, &c1, 1);

        pw_fwrite_in(out, Where + 3, 3);
        Whatever[32] = IN(Where + 7);
        if ((IN(Where + 6) == 0x00) && (Whatever[32] == 0x00))
            Whatever[32] = 0x01;
        pw_fwrite_in(out, Where + 6, 1);
        pw_fwrite(out, &Whatever[32], 1);
        Where += 8;
    }

    /* pattern table lenght */
    Pat_Pos = (uint8_t)(((IN(Where) * 256) + IN(Where + 1)) / 4);
    Where += 2;
    pw_fwrite(out, &Pat_Pos, 1);

    /* write NoiseTracker byte */
    Whatever[0] = 0x7F;
    pw_fwrite(out, Whatever, 1);

    /* read pattern address list */
    for (i = 0; i < 128; i++)
    {
        Pattern_Address[i] = (IN(Where) * 256 * 256 * 256) + (IN(Where + 1) * 256 * 256) +
                             (IN(Where + 2) * 256) + IN(Where + 3);
        Where += 4;
    }

    /* deduce pattern list and write it */
    for (i = 0; i < 128; i++)
        Whatever[i] = (uint8_t)(Pattern_Address[i] / 1024);
    pw_fwrite(out, Whatever, 128);

    /* write ptk's ID */
    Whatever[0] = 'M';
    Whatever[1] = '.';
    Whatever[2] = 'K';
    Whatever[3] = '.';
    pw_fwrite(out, Whatever, 4);

    /* get pattern data size */
    j = (IN(Where) * 256 * 256 * 256) + (IN(Where + 1) * 256 * 256) + (IN(Where + 2) * 256) + IN(Where + 3);
    Where += 4;

    /* read and XOR pattern data */
    free(Whatever);
    /* j fitted in the input (Rip_PM01 checked it); a negative one made the
     * original crash in memset() */
    if (j < 0)
    {
        pw_fclose(out);
        return;
    }
    Whatever = (uint8_t *)calloc((size_t)j + 1, 1);
    PatternData = (uint8_t *)calloc((size_t)j + 1, 1);
    if ((Whatever == NULL) || (PatternData == NULL))
    {
        free(Whatever);
        free(PatternData);
        c->fatal = PW_ERR_NOMEM;
        pw_fclose(out);
        return;
    }
    for (k = 0; k < j; k++)
    {
        if (k % 4 == 3)
        {
            PatternData[k] = (uint8_t)((240 - (IN(Where + k) & 0xf0)) + (IN(Where + k) & 0x0f));
            continue;
        }
        PatternData[k] = (uint8_t)(255 - IN(Where + k));
    }

    /* all right, now, let's take care of these 'finetuned' value ... pfff */
    Old_Smp_Nbr[0] = Old_Smp_Nbr[1] = Old_Smp_Nbr[2] = Old_Smp_Nbr[3] = 0x1f;
    for (i = 0; i < j / 4; i++)
    {
        c1 = PatternData[i * 4] & 0x0f;
        c2 = PatternData[i * 4 + 1];
        k = (c1 * 256) + c2;
        c3 = (PatternData[i * 4] & 0xf0) | ((PatternData[i * 4 + 2] >> 4) & 0x0f);
        if (c3 == 0)
            c3 = Old_Smp_Nbr[i % 4];
        else
            Old_Smp_Nbr[i % 4] = c3;
        /* Smp_Fine_Table[] has 31 entries; c3 can reach 255 on odd input */
        fine = ((c3 >= 1) && (c3 <= 31)) ? Smp_Fine_Table[c3 - 1] : 0;
        if ((k != 0) && (fine != 0x00))
        {
            for (l = 0; l < 36; l++)
            {
                /* the finetunes were checked by the test: fine < 16 */
                if (k == ((fine < 16) ? pw_tuning[fine][l] : 0))
                {
                    Whatever[i * 4] = poss[l + 1][0];
                    Whatever[i * 4 + 1] = poss[l + 1][1];
                }
            }
        }
        else
        {
            Whatever[i * 4] = PatternData[i * 4] & 0x0f;
            Whatever[i * 4 + 1] = PatternData[i * 4 + 1];
        }
        Whatever[i * 4] |= (PatternData[i * 4] & 0xf0);
        Whatever[i * 4 + 2] = PatternData[i * 4 + 2];
        Whatever[i * 4 + 3] = PatternData[i * 4 + 3];
    }
    pw_fwrite(out, Whatever, j);
    free(Whatever);
    free(PatternData);

    /* sample data */
    Where += j;
    pw_fwrite_in(out, Where, WholeSampleSize);

    /* crap */
    pw_crap("   Promizer 0.1   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
