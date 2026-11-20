/*
 * Promizer18a.c - Promizer 1.8a (upstream r/Promizer18a.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "formats.h"

int16_t pw_test_PMZ(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* test 1 */
    if ((c->start + 4456) > c->in_size)
        return PW_BAD;

    /* test 2 (byte 21 == 0xd2) was disabled in the original */

    /* test 3 */
    c->j = (IN(c->start + 4456) * 256 * 256 * 256) + (IN(c->start + 4457) * 256 * 256) +
           (IN(c->start + 4458) * 256) + IN(c->start + 4459);
    if ((c->start + c->j + 4456) > (uint32_t)c->in_size)
        return PW_BAD;

    /* test 4 */
    c->k = (IN(c->start + 4712) * 256) + IN(c->start + 4713);
    c->l = c->k / 4;
    c->l *= 4;
    if (c->l != c->k)
        return PW_BAD;

    /* test 5 */
    if (IN(c->start + 36) != 0x11)
        return PW_BAD;

    /* test 6 */
    if (IN(c->start + 37) != 0x00)
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_PM18a(pw_ctx *c)
{
    /* we NEED this 'c->j' value found while testing !,so we keep it :) */
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
        c->wss += (((IN(c->start + 4464 + 8 * c->k) * 256) + IN(c->start + 4465 + 8 * c->k)) * 2);
    c->out_size = 4460 + c->j + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Promizer 1.8a module", PWF_Promizer_18a);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 2); /* 1 should do but call it "just to be sure" :) */
}

/*
 *   Promizer_18a.c   1997 (c) Asle / ReDoX
 *
 * Converts PM18a packed MODs back to PTK MODs
 * thanks to Gryzor and his ProWizard tool ! ... without it, this prog
 * would not exist !!!
 *
 * update 20 mar 2003 (it's war time again .. brrrr)
 * - removed all open() funcs.
 * - optimized more than quite a bit (src is 5kb shorter !)
 *
 * update 07 jan 2010
 * - bug fix in patternlist generation
 */

#define ON  0
#define OFF 1

/* The original converted into a 64 KiB heap buffer and wrote c1*1024 bytes
 * of it back (c1 is a byte: at most 255 patterns). Crafted pattern data runs
 * past 64 KiB; the buffer here covers everything that can be written back. */
#define PM18A_PATTERN_BUF (256 * 1024)

/* The pattern data size is not checked. Data further than this past the end
 * of the input is only "read" by corrupt files (the original then failed to
 * allocate its copy, or ran off its input buffer): give up on those. */
#define PM18A_OVERREAD_LIMIT (2 * 1024 * 1024)

void pw_depack_PM18a(pw_ctx *c)
{
    uint8_t  c1 = 0x00, c2 = 0x00;
    int16_t  Ref_Max = 0;
    int32_t  Pats_Address[128];
    int32_t  Read_Pats_Address[128];
    uint8_t  NOP = 0x00; /* number of pattern */
    uint8_t *ReferenceTable;
    uint8_t *Pattern;
    int32_t  i = 0, j = 0, k = 0, l = 0, m = 0;
    int32_t  Total_Sample_Size = 0;
    int32_t  PatDataSize = 0;
    int32_t  SDAV = 0;
    int32_t  RefSize;
    uint8_t  FLAG = OFF;
    uint8_t  Smp_Fine_Table[31];
    uint8_t  poss[37][2];
    uint8_t  OldSmpValue[4];
    uint8_t  p0, p1, p2, p3, fine, smp;
    int16_t  Period;
    uint8_t *Whatever;
    int32_t  Where = c->start;
    int32_t  PatWhere;
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(Smp_Fine_Table, 0, sizeof Smp_Fine_Table);
    memset(OldSmpValue, 0, sizeof OldSmpValue);
    memset(Pats_Address, 0, sizeof Pats_Address);
    memset(Read_Pats_Address, 0, sizeof Read_Pats_Address);

    Whatever = (uint8_t *)calloc(1085, 1);
    if (Whatever == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        pw_fclose(out);
        return;
    }

    /* bypass replaycode routine */
    Where = c->start + 4464;

    for (i = 0; i < 31; i++)
    {
        Whatever[42 + 30 * i] = IN(Where);
        Whatever[43 + 30 * i] = IN(Where + 1);
        Whatever[44 + 30 * i] = IN(Where + 2);
        Whatever[45 + 30 * i] = IN(Where + 3);
        Whatever[46 + 30 * i] = IN(Where + 4);
        Whatever[47 + 30 * i] = IN(Where + 5);
        Whatever[48 + 30 * i] = IN(Where + 6);
        Whatever[49 + 30 * i] = IN(Where + 7);

        Total_Sample_Size += (((IN(Where) * 256) + IN(Where + 1)) * 2);
        Smp_Fine_Table[i] = IN(Where + 2);
        Where += 8;
    }

    /* pattern table lenght */
    NOP = (uint8_t)(((IN(Where) * 256) + IN(Where + 1)) / 4);
    Whatever[950] = NOP;
    Where += 2;

    /* NoiseTracker restart byte */
    Whatever[951] = 0x7f;

    for (i = 0; i < 128; i++)
    {
        Pats_Address[i] = (IN(Where) * 256 * 256 * 256) + (IN(Where + 1) * 256 * 256) +
                          (IN(Where + 2) * 256) + IN(Where + 3);
        Where += 4;
    }

    /* a little pre-calc code ... no other way to deal with these unknown pattern data sizes ! :( */
    Where = c->start + 4460;
    PatDataSize = (IN(Where) * 256 * 256 * 256) + (IN(Where + 1) * 256 * 256) +
                  (IN(Where + 2) * 256) + IN(Where + 3);

    /* go back to pattern data starting address */
    Where = c->start + 5226;
    /* The original copied the pattern data to a buffer of PatDataSize+1
     * bytes first and returned (leaving the file empty) when malloc()
     * failed, as it did for a negative size. The data is read straight from
     * the input here; sizes far past the input end are treated as such a
     * failure too. */
    if ((PatDataSize + 1 < 0) ||
        ((int64_t)Where + PatDataSize > (int64_t)c->in_size + PM18A_OVERREAD_LIMIT))
    {
        free(Whatever);
        pw_fclose(out);
        return;
    }
    PatWhere = Where;

    for (j = 0; j < PatDataSize; j += 2)
    {
        if (((IN(PatWhere + j) * 256) + IN(PatWhere + j + 1)) > Ref_Max)
            Ref_Max = (int16_t)((IN(PatWhere + j) * 256) + IN(PatWhere + j + 1));
    }
    Where += PatDataSize;

    /* read "reference Table" */
    Ref_Max += 1; /* coz 1st value is 0 ! */
    i = Ref_Max * 4; /* coz each block is 4 bytes long */
    /* Ref_Max is a signed short: values from 0x7fff up make it negative and
     * the original crashed in memset() */
    if (i < 0)
    {
        free(Whatever);
        pw_fclose(out);
        return;
    }
    RefSize = i + 1;
    ReferenceTable = (uint8_t *)calloc((size_t)RefSize, 1);
    Pattern = (uint8_t *)calloc(PM18A_PATTERN_BUF, 1);
    if ((ReferenceTable == NULL) || (Pattern == NULL))
    {
        free(ReferenceTable);
        free(Pattern);
        free(Whatever);
        c->fatal = PW_ERR_NOMEM;
        pw_fclose(out);
        return;
    }
    for (j = 0; j < i; j++)
        ReferenceTable[j] = IN(Where + j);

    c1 = 0; /* will count patterns */
    k = 0;  /* current note number */
    i = 0;
    for (j = 0; j < PatDataSize; j += 2)
    {
        if (((i % 1024) == 0) || (i == 0))
        {
            /* Read_Pats_Address[] has 128 entries; c1 counts up to 255 */
            if (c1 < 128)
                Read_Pats_Address[c1] = j;
            c1 += 0x01;
        }

        /* references past the table read 0 (the zeroed heap of the reference) */
        m = ((IN(PatWhere + j) * 256) + IN(PatWhere + j + 1)) * 4;
        p0 = (m < RefSize) ? ReferenceTable[m] : 0;
        p1 = (m + 1 < RefSize) ? ReferenceTable[m + 1] : 0;
        p2 = (m + 2 < RefSize) ? ReferenceTable[m + 2] : 0;
        p3 = (m + 3 < RefSize) ? ReferenceTable[m + 3] : 0;

        c2 = ((p2 >> 4) & 0x0f) | (p0 & 0xf0);
        if (c2 != 0x00)
            OldSmpValue[k % 4] = c2;
        Period = (int16_t)(((p0 & 0x0f) * 256) + p1);
        /* Smp_Fine_Table[] has 31 entries; the sample number can be 0..255 */
        smp = OldSmpValue[k % 4];
        fine = ((smp >= 1) && (smp <= 31)) ? Smp_Fine_Table[smp - 1] : 0;
        if ((Period != 0) && (fine != 0x00))
        {
            for (l = 0; l < 36; l++)
            {
                /* the finetune is not checked: only 16 period tables */
                if (((fine < 16) ? pw_tuning[fine][l] : 0) == Period)
                {
                    p0 &= 0xf0;
                    p0 |= poss[l + 1][0];
                    p1 = poss[l + 1][1];
                    break;
                }
            }
        }
        if ((i >= 0) && (i < PM18A_PATTERN_BUF - 3))
        {
            Pattern[i] = p0;
            Pattern[i + 1] = p1;
            Pattern[i + 2] = p2;
            Pattern[i + 3] = p3;
        }

        if (((p2 & 0x0f) == 0x0d) || ((p2 & 0x0f) == 0x0b))
            FLAG = ON;
        if ((FLAG == ON) && ((k % 4) == 3))
        {
            FLAG = OFF;
            while ((i % 1024) != 0)
                i++;
            i -= 4;
        }
        k += 1;
        i += 4;
    }
    free(ReferenceTable);

    /* write pattern table */
    for (c2 = 0; c2 < 128; c2 += 0x01)
        for (i = 0; i < c1; i++)
            if (Pats_Address[c2] == ((i < 128) ? Read_Pats_Address[i] : 0))
                Whatever[952 + c2] = (uint8_t)i;
    while (NOP < 128)
    {
        Whatever[952 + NOP] = 0x00;
        NOP++;
    }

    /* write tag */
    Whatever[1080] = 'M';
    Whatever[1081] = '.';
    Whatever[1082] = 'K';
    Whatever[1083] = '.';

    pw_fwrite(out, Whatever, 1084);

    free(Whatever);

    /* write pattern data */
    /* c1 is still the number of patterns stored */
    pw_fwrite(out, Pattern, c1 * 1024);

    free(Pattern);

    Where = c->start + 4456;
    SDAV = (IN(Where) * 256 * 256 * 256) + (IN(Where + 1) * 256 * 256) + (IN(Where + 2) * 256) + IN(Where + 3);
    Where = c->start + 4460 + SDAV;

    /* smp data */
    pw_fwrite_in(out, Where, Total_Sample_Size);

    pw_crap("  Promizer v1.8a  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
