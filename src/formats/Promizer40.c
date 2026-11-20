/*
 * Promizer40.c - Promizer 4.0 (upstream r/Promizer40.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "formats.h"

int16_t pw_test_PM40(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* size of the pattern list */
    c->j = IN(c->start + 7);
    if (c->j > 0x7f)
        return PW_BAD;
    /* c->j is the size of the pattern list */

    /* finetune */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if (IN(c->start + c->k * 8 + 266) > 0x0f)
            return PW_BAD;
    }

    /* volume */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if (IN(c->start + c->k * 8 + 267) > 0x40)
            return PW_BAD;
    }

    /* sample data address */
    c->l = ((IN(c->start + 512) * 256 * 256 * 256) + (IN(c->start + 513) * 256 * 256) +
            (IN(c->start + 514) * 256) + IN(c->start + 515));
    if ((c->l <= 520) || (c->l > 2500000l))
        return PW_BAD;

    /* c->l is the sample data address */
    return PW_GOOD;
}

void pw_rip_PM40(pw_ctx *c)
{
    /* c->l is the sample data address */

    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
        c->wss += (((IN(c->start + 264 + c->k * 8) * 256) + IN(c->start + 265 + c->k * 8)) * 2);

    c->out_size = c->wss + c->l + 4;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Promizer 4.0 module", PWF_PM40);

    if (c->save_status == PW_GOOD)
        c->i += 2; /* 1 should do but call it "just to be sure" :) */
}

/*
 *   Promizer_40.c   1997 (c) Asle / ReDoX
 *
 * Converts PM40 packed MODs back to PTK MODs
 *
 * 20100112 - complete rewrite
 */

#define ON  0
#define OFF 1
#define SAMPLE_DESC         264
#define ADDRESS_SAMPLE_DATA 512
#define ADDRESS_REF_TABLE   516
#define PATTERN_DATA        520

/* The original converted into a 64 KiB heap buffer and wrote (c1-1)*1024
 * bytes of it back (c1 is a byte). Crafted pattern data runs past 64 KiB;
 * the buffer here covers everything that can be written back. */
#define PM40_PATTERN_BUF (256 * 1024)

/* The pattern data size is not checked. Data further than this past the end
 * of the input is only "read" by corrupt files (the original then ran off its
 * input buffer): give up on those instead of looping over gigabytes. */
#define PM40_OVERREAD_LIMIT (2 * 1024 * 1024)

void pw_depack_PM40(pw_ctx *c)
{
    int32_t  Pats_Address[128];
    int32_t  Read_Pats_Address[128];
    uint8_t *Whatever;
    uint8_t  c1 = 0x00, c2 = 0x00;
    int32_t  Where;
    uint8_t *Pattern;
    int32_t  i = 0, j = 0, k = 0, l = 0, m = 0;
    int32_t  Total_Sample_Size = 0;
    int32_t  PatDataSize = 0;
    int32_t  SmpAddy;
    uint8_t  FLAG = OFF;
    uint8_t  poss[37][2];
    uint8_t  note, p0, p1, p2, p3;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    pw_fill_ptk_table(poss);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    Pattern = (uint8_t *)calloc(PM40_PATTERN_BUF, 1);
    memset(Pats_Address, 0, sizeof Pats_Address);
    memset(Read_Pats_Address, 0, sizeof Read_Pats_Address);

    /* title (empty) */
    Whatever = (uint8_t *)calloc(1085, 1);
    if ((Pattern == NULL) || (Whatever == NULL))
    {
        free(Pattern);
        free(Whatever);
        c->fatal = PW_ERR_NOMEM;
        pw_fclose(out);
        return;
    }

    /* read and write sample headers */
    Where = c->start + SAMPLE_DESC;
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
        Total_Sample_Size += (((Whatever[42 + 30 * i] * 256) + Whatever[43 + 30 * i]) * 2);
        Where += 8;
    }

    /* size of the pattern list */
    Whatever[950] = IN(c->start + 7);

    /* NoiseTracker restart byte */
    Whatever[951] = 0x7f;

    /* pattern addresses (the test limited the list to 127 entries) */
    Where = c->start + 8;
    for (i = 0; (i < Whatever[950]) && (i < 128); i++)
        Pats_Address[i] = (IN(Where + i * 2) * 256) + IN(Where + i * 2 + 1);

    Where = c->start + ADDRESS_SAMPLE_DATA;
    SmpAddy = ((IN(Where) * 256 * 256 * 256) + (IN(Where + 1) * 256 * 256) + (IN(Where + 2) * 256) + IN(Where + 3)) + 4;

    Where += 4;
    l = ((IN(Where) * 256 * 256 * 256) + (IN(Where + 1) * 256 * 256) + (IN(Where + 2) * 256) + IN(Where + 3)) + 8;

    Where += 4;
    PatDataSize = l - PATTERN_DATA;

    /* a pattern data size far past the input end: the original ran off its
     * input buffer */
    if ((int64_t)c->start + PATTERN_DATA + PatDataSize > (int64_t)c->in_size + PM40_OVERREAD_LIMIT)
    {
        free(Pattern);
        free(Whatever);
        pw_fclose(out);
        return;
    }

    /* convert pattern data here */
    i = 0;
    k = 0;
    c1 = 0x00;
    for (j = 0; j < PatDataSize; j += 2)
    {
        if (((i % 1024) == 0) || (i == 0))
        {
            /* Read_Pats_Address[] has 128 entries; c1 counts up to 255 */
            if (c1 < 128)
                Read_Pats_Address[c1] = j;
            c1 += 0x01;
        }

        m = (IN(c->start + j + PATTERN_DATA) * 256) + IN(c->start + j + PATTERN_DATA + 1);
        if (m == 0) /* no note */
        {
        }
        else
        {
            m -= 1;
            m *= 4;
            m += c->start;
            /* poss[] has 37 entries; the note byte can be 0..255 */
            note = IN(l + m + 1);
            p0 = (IN(l + m) & 0xf0) | ((note < 37) ? poss[note][0] : 0);
            p1 = (note < 37) ? poss[note][1] : 0;
            p2 = IN(l + m + 2) | ((IN(l + m) << 4) & 0xf0);
            p3 = IN(l + m + 3);
            if ((i >= 0) && (i < PM40_PATTERN_BUF - 3))
            {
                Pattern[i] = p0;
                Pattern[i + 1] = p1;
                Pattern[i + 2] = p2;
                Pattern[i + 3] = p3;
            }

            if (((p2 & 0x0f) == 0x0d) || ((p2 & 0x0f) == 0x0b))
                FLAG = ON;
        }
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

    /* pattern table */
    for (c2 = 0; c2 < 128; c2 += 0x01)
        for (i = 0; i < c1; i++)
            if (Pats_Address[c2] == ((i < 128) ? Read_Pats_Address[i] : 0))
                Whatever[952 + c2] = (uint8_t)i;
    c2 = Whatever[950];
    while (c2 < 128)
    {
        Whatever[952 + c2] = 0x00;
        c2 += 0x01;
    }

    Whatever[1080] = 'M';
    Whatever[1081] = '.';
    Whatever[1082] = 'K';
    Whatever[1083] = '.';

    pw_fwrite(out, Whatever, 1084);
    free(Whatever);

    /* c1 (+1) is still the number of patterns stored */
    if ((c1 - 1) * 1024 < 0)
    {
        /* c1 == 0 (no pattern data, or 256 patterns): fwrite() got the size
         * (size_t)-1024. The C library of the Windows build copied what fit
         * in its 4 KiB stdio buffer (which held the 1084 header bytes),
         * flushed it, and failed the rest of the write. */
        pw_fwrite(out, Pattern, 4096 - 1084);
    }
    else
        pw_fwrite(out, Pattern, (c1 - 1) * 1024);

    free(Pattern);

    Where = c->start + SmpAddy;
    pw_fwrite_in(out, Where, Total_Sample_Size);

    pw_crap("   Promizer 4.0   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
