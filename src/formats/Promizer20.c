/*
 * Promizer20.c - Promizer 2.0 (upstream r/Promizer20.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "formats.h"

int16_t pw_test_PM2(pw_ctx *c)
{
    c->start = (int32_t)c->i;
    /* test 1 */
    if ((c->start + 5714) > c->in_size)
        return PW_BAD;

    /* test 2 (frequency table at 5094) was disabled in the original */

    /* test 3 */
    if (IN(c->start + 5461) > 0x40)
        /* testing a volume */
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_PM20(pw_ctx *c)
{
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
        c->wss += (((IN(c->start + 5458 + c->k * 8) * 256) + IN(c->start + 5459 + c->k * 8)) * 2);
    c->j = (IN(c->start + 5706) * 256 * 256 * 256) + (IN(c->start + 5707) * 256 * 256) +
           (IN(c->start + 5708) * 256) + IN(c->start + 5709);
    c->out_size = c->wss + 5198 + c->j;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Promizer 2.0 module", PWF_Promizer_20);

    if (c->save_status == PW_GOOD)
        c->i += 2; /* 1 should do but call it "just to be sure" :) */
}

/*
 *   Promizer_20.c   1997 (c) Asle / ReDoX
 *
 * Converts PM20 packed MODs back to PTK MODs
 *
 * update 20 mar 2003 (it's war time again .. brrrr)
 * - removed all open() funcs.
 * - optimized more than quite a bit (src 4kb shorter !)
 *
 * update 20091113
 * - fixed patternlist generation and cleaned a bit
 *
 * update 20100212-20100216
 * - fixed endless loop bug (used PW_i instead of i ...)
 * - correct conversion when there's unused patterns stored .. they are kept ;)
 */

#define ON  0
#define OFF 1
#define AFTER_REPLAY_CODE   5198
#define SAMPLE_DESC         5458
#define ADDRESS_SAMPLE_DATA 5706
#define ADDRESS_REF_TABLE   5710
#define PATTERN_DATA        5714

/* The pattern data size is not checked. Data further than this past the end
 * of the input is only "read" by corrupt files (the original then ran off its
 * input buffer): give up on those instead of looping over gigabytes. */
#define PM20_OVERREAD_LIMIT (2 * 1024 * 1024)

/* The original converted into a 64 KiB heap buffer but wrote c->l*1024
 * bytes of it, c->l being the number of patterns met; crafted pattern data
 * (Bxx/Dxx skips) runs far past 64 KiB. The buffer here grows as needed up to
 * this size; past it the original read (and wrote) foreign heap memory. */
#define PM20_PATTERN_MAX (16 * 1024 * 1024)

static int pm20_pattern_room(uint8_t **buf, int32_t *cap, int32_t need)
{
    int32_t  ncap;
    uint8_t *p;

    if (need <= *cap)
        return 1;
    if (need > PM20_PATTERN_MAX)
        return 0;
    ncap = *cap;
    while (ncap < need)
        ncap *= 2;
    if (ncap > PM20_PATTERN_MAX)
        ncap = PM20_PATTERN_MAX;
    p = (uint8_t *)realloc(*buf, (size_t)ncap);
    if (p == NULL)
        return 0;
    memset(p + *cap, 0, (size_t)(ncap - *cap));
    *buf = p;
    *cap = ncap;
    return 1;
}

void pw_depack_PM20(pw_ctx *c)
{
    int16_t  Ref_Max = 0;
    int32_t  Pats_Address[128], Pats_Address_infile[128];
    uint8_t  NOP = 0x00; /* number of pattern */
    uint8_t *ReferenceTable;
    uint8_t *Pattern;
    int32_t  PatCap = 65536;
    int32_t  i = 0, j;
    int32_t  Total_Sample_Size = 0;
    int32_t  PatDataSize = 0;
    int32_t  RefSize, PatWhere, lmax;
    uint8_t  FLAG = OFF;
    uint8_t  poss[37][2];
    uint8_t  Note, Smp;
    uint8_t  p0, p1, p2, p3;
    uint8_t *Whatever;
    int32_t  Where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    pw_fill_ptk_table(poss);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(Pats_Address, 0, sizeof Pats_Address);
    memset(Pats_Address_infile, 0, sizeof Pats_Address_infile);

    Whatever = (uint8_t *)calloc(1085, 1);
    if (Whatever == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        pw_fclose(out);
        return;
    }

    /* bypass replaycode routine */
    Where += SAMPLE_DESC;

    for (i = 0; i < 31; i++)
    {
        Whatever[i * 30 + 42] = IN(Where);
        Whatever[i * 30 + 43] = IN(Where + 1);
        Total_Sample_Size += (((Whatever[i * 30 + 42] * 256) + Whatever[i * 30 + 43]) * 2);
        Whatever[i * 30 + 44] = IN(Where + 2) / 2;
        Whatever[i * 30 + 45] = IN(Where + 3);
        Whatever[i * 30 + 46] = IN(Where + 4);
        Whatever[i * 30 + 47] = IN(Where + 5);
        Whatever[i * 30 + 48] = IN(Where + 6);
        Whatever[i * 30 + 49] = IN(Where + 7);
        if ((Whatever[i * 30 + 48] == 0x00) && (Whatever[i * 30 + 49] == 0x00))
            Whatever[i * 30 + 49] = 0x01;

        Where += 8;
    }

    /* read "used" size of pattern table */
    Where = c->start + AFTER_REPLAY_CODE + 2;
    NOP = (uint8_t)(((IN(Where) * 256) + IN(Where + 1)) / 2);
    Where += 2;

    /* write size of pattern list */
    Whatever[950] = NOP;

    /* NoiseTracker restart byte */
    Whatever[951] = 0x7f;

    /* read pattern addys */
    for (i = 0; i < NOP; i++)
    {
        /* Pats_Address[] has 128 entries; NOP can reach 255 */
        if (i < 128)
            Pats_Address[i] = (IN(Where) * 256) + IN(Where + 1);
        Where += 2;
    }

    /* write tag */
    Whatever[1080] = 'M';
    Whatever[1081] = '.';
    Whatever[1082] = 'K';
    Whatever[1083] = '.';

    /* a little pre-calc code ... no other way to deal with these unknown pattern data sizes ! :( */
    /* so, first, we get the pattern data size .. */
    Where = c->start + ADDRESS_REF_TABLE;
    c->j = (IN(Where) * 256 * 256 * 256) + (IN(Where + 1) * 256 * 256) + (IN(Where + 2) * 256) + IN(Where + 3);
    PatDataSize = (AFTER_REPLAY_CODE + c->j) - PATTERN_DATA;

    /* go back to pattern data starting address */
    Where = c->start + PATTERN_DATA;

    /* The original copied the pattern data to a buffer of PatDataSize+1
     * bytes first; it is read straight from the input here. A negative size
     * made it crash (memset(), or the unsigned loop below running off the
     * buffer), a huge one ran off the input. */
    if ((PatDataSize < 0) ||
        ((int64_t)Where + PatDataSize > (int64_t)c->in_size + PM20_OVERREAD_LIMIT))
    {
        free(Whatever);
        pw_fclose(out);
        return;
    }
    PatWhere = Where;

    /* now, reading all pattern data to get the max value of note */
    for (c->j = 0; c->j < (uint32_t)PatDataSize; c->j += 2)
    {
        if (((IN(PatWhere + c->j) * 256) + IN(PatWhere + c->j + 1)) > Ref_Max)
            Ref_Max = (int16_t)((IN(PatWhere + c->j) * 256) + IN(PatWhere + c->j + 1));
    }

    /* read "reference Table" */
    Where = c->start + ADDRESS_REF_TABLE;
    c->j = (IN(Where) * 256 * 256 * 256) + (IN(Where + 1) * 256 * 256) + (IN(Where + 2) * 256) + IN(Where + 3);
    Where = c->start + AFTER_REPLAY_CODE + c->j;

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
    RefSize = i;
    ReferenceTable = (uint8_t *)calloc((size_t)RefSize + 1, 1);
    Pattern = (uint8_t *)calloc((size_t)PatCap, 1);
    if ((ReferenceTable == NULL) || (Pattern == NULL))
    {
        free(ReferenceTable);
        free(Pattern);
        free(Whatever);
        c->fatal = PW_ERR_NOMEM;
        pw_fclose(out);
        return;
    }
    for (c->j = 0; c->j < (uint32_t)i; c->j++)
        ReferenceTable[c->j] = IN(Where + c->j);

    /* go back to pattern data starting address */
    Where = c->start + PATTERN_DATA;

    c->k = 0; /* current note number */
    i = 0;
    c->l = 1; /* nbr of patterns stored */
    Pats_Address_infile[0] = 0;
    for (c->j = 0; c->j < (uint32_t)PatDataSize; c->j += 2)
    {
        if (((i % 1024) == 0) && (c->j > 0))
        {
            /* Pats_Address_infile[] has 128 entries */
            if (c->l < 128)
                Pats_Address_infile[c->l] = (int32_t)c->j;
            c->l++;
        }
        c->m = ((IN(PatWhere + c->j) * 256) + IN(PatWhere + c->j + 1)) * 4;

        /* references past the table read 0 (the zeroed heap of the reference) */
        Smp = (c->m < (uint32_t)RefSize) ? ReferenceTable[c->m] : 0;
        Smp = Smp >> 2;
        Note = (c->m + 1 < (uint32_t)RefSize) ? ReferenceTable[c->m + 1] : 0;

        /* poss[] has 37 entries; Note/2 can reach 127 */
        p0 = (Smp & 0xf0);
        p0 |= ((Note / 2) < 37) ? poss[(Note / 2)][0] : 0;
        p1 = ((Note / 2) < 37) ? poss[(Note / 2)][1] : 0;
        p2 = (c->m + 2 < (uint32_t)RefSize) ? ReferenceTable[c->m + 2] : 0;
        p2 |= ((Smp << 4) & 0xf0);
        p3 = (c->m + 3 < (uint32_t)RefSize) ? ReferenceTable[c->m + 3] : 0;
        if ((i >= 0) && (i <= PM20_PATTERN_MAX - 4) && pm20_pattern_room(&Pattern, &PatCap, i + 4))
        {
            Pattern[i] = p0;
            Pattern[i + 1] = p1;
            Pattern[i + 2] = p2;
            Pattern[i + 3] = p3;
        }

        if (((p2 & 0x0f) == 0x0d) || ((p2 & 0x0f) == 0x0b))
            FLAG = ON;
        if ((FLAG == ON) && ((c->k % 4) == 3))
        {
            FLAG = OFF;
            while ((i % 1024) != 0)
                i++;
            i -= 4;
        }

        c->k += 1;
        i += 4;
    }

    /* let's update now the pattern list */
    /* (entries 128 and up of Pats_Address_infile[] read as 0 and can never
     * match: a 0 address already matched entry 0) */
    lmax = (c->l < 128) ? (int32_t)c->l : 128;
    for (i = 0; i < NOP; i++)
    {
        for (j = 0; j < lmax; j++)
        {
            if (Pats_Address_infile[j] == ((i < 128) ? Pats_Address[i] : 0))
            {
                /* Whatever[] is 1085 bytes; i can reach 254 */
                if (952 + i < 1085)
                    Whatever[952 + i] = (uint8_t)j;
                break;
            }
        }
    }
    pw_fwrite(out, Whatever, 1084);

    free(ReferenceTable);
    free(Whatever);

    /* write pattern data */
    /* c->l is still the number of patterns stored */
    if (((uint64_t)c->l * 1024 > PM20_PATTERN_MAX) ||
        !pm20_pattern_room(&Pattern, &PatCap, (int32_t)(c->l * 1024)))
    {
        /* the original wrote heap memory far past its pattern buffer */
        free(Pattern);
        pw_fclose(out);
        return;
    }
    pw_fwrite(out, Pattern, (int64_t)c->l * 1024);
    free(Pattern);

    /* get address of sample data .. and go there */
    Where = c->start + ADDRESS_SAMPLE_DATA;
    i = (IN(Where) * 256 * 256 * 256) + (IN(Where + 1) * 256 * 256) + (IN(Where + 2) * 256) + IN(Where + 3);
    Where = c->start + AFTER_REPLAY_CODE + i;

    /* read and save sample data */
    pw_fwrite_in(out, Where, Total_Sample_Size);

    pw_crap("   Promizer 2.0   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
