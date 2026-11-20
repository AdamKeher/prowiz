/*
 * Prorunner10.c - ProRunner v1 (upstream r/Prorunner10.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "formats.h"

/* called with the scan position on the "SNT." ID */
int16_t pw_test_PRUN1(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1080);
    if ((IN(c->start + 951) != 0x7f) && (IN(c->start + 951) != 0x00))
        return PW_BAD;

    /* test 3 */
    if (IN(c->start + 950) > 0x7f)
        return PW_BAD;

    /* test 4 */
    /* (c->wss is not reset: it adds to what the previous handler left) */
    for (c->k = 0; c->k < 31; c->k++)
    {
        /* size */
        c->j = (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2);
        /* loop size */
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);

        if (pw_test_smps((int32_t)(c->j * 2), (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 45 + 30 * c->k), IN(c->start + 44 + 30 * c->k)) == PW_BAD)
            return PW_BAD;

        c->wss += c->j;
    }

    return PW_GOOD;
}

void pw_rip_PRUN1(pw_ctx *c)
{
    c->l = 0;
    for (c->k = 0; c->k < 128; c->k++)
        if (IN(c->start + 952 + c->k) > c->l)
            c->l = IN(c->start + 952 + c->k);
    c->l += 1;
    c->out_size = (c->l * 1024) + 1084 + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Prorunner 1 module", PWF_ProRunner_v1);

    if (c->save_status == PW_GOOD)
        c->i += 1; /* 1080 could be enough */
}

/*
 *   ProRunner1.c   1996 (c) Asle / ReDoX
 *
 * Converts MODs converted with Prorunner packer v1.0
 *
 * update:28/11/99
 *   - removed fopen() and all similar functions
 *   - Speed and Size (1/4) optimizings
 */
void pw_depack_PRUN1(pw_ctx *c)
{
    uint8_t  Whatever[4];
    uint8_t  poss[37][2];
    uint8_t  Max = 0x00;
    uint8_t  note;
    int32_t  WholeSampleSize = 0;
    int32_t  i = 0, j = 0;
    int32_t  Where = c->start;
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* read and write whole header */
    pw_fwrite_in(out, Where, 950);

    /* get whole sample size */
    for (i = 0; i < 31; i++)
        WholeSampleSize += (((IN(Where + 42 + i * 30) * 256) + IN(Where + 43 + i * 30)) * 2);

    /* read and write size of pattern list */
    /* read and write ntk byte and pattern list */
    pw_fwrite_in(out, Where + 950, 130);
    Where += 952;

    /* write ID */
    Whatever[0] = 'M';
    Whatever[1] = '.';
    Whatever[2] = 'K';
    Whatever[3] = '.';
    pw_fwrite(out, Whatever, 4);

    /* get number of pattern */
    Max = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(Where + i) > Max)
            Max = IN(Where + i);
    }

    /* pattern data */
    Where = c->start + 1084;
    for (i = 0; i <= Max; i++)
    {
        for (j = 0; j < 256; j++)
        {
            Whatever[0] = IN(Where) & 0xf0;
            Whatever[2] = (uint8_t)((IN(Where) & 0x0f) << 4);
            Whatever[2] |= IN(Where + 2);
            Whatever[3] = IN(Where + 3);
            /* poss[] has 37 entries; the note byte can be 0..255 */
            note = IN(Where + 1);
            Whatever[0] |= (note < 37) ? poss[note][0] : 0;
            Whatever[1] = (note < 37) ? poss[note][1] : 0;
            pw_fwrite(out, Whatever, 4);
            Where += 4;
        }
    }

    /* sample data */
    pw_fwrite_in(out, Where, WholeSampleSize);

    /* crap */
    pw_crap("   ProRunner v1   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
