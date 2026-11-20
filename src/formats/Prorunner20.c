/*
 * Prorunner20.c - ProRunner v2 (upstream r/Prorunner20.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "formats.h"

/* called with the scan position on the "SNT!" ID */
int16_t pw_test_PRUN2(pw_ctx *c)
{
    c->start = (int32_t)c->i;
    c->j = (IN(c->i + 4) * 256 * 256 * 256) + (IN(c->i + 5) * 256 * 256) + (IN(c->i + 6) * 256) + IN(c->i + 7);

    /* test sample_data address */
    if ((c->j + c->start) > (uint32_t)c->in_size)
        return PW_BAD;

    /* test volumes */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if (IN(c->start + 11 + c->k * 8) > 0x40)
            return PW_BAD;
    }

    /* test finetunes */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if (IN(c->start + 10 + c->k * 8) > 0x0F)
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_PRUN2(pw_ctx *c)
{
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
        c->wss += (((IN(c->start + 8 + c->k * 8) * 256) + IN(c->start + 9 + c->k * 8)) * 2);

    c->out_size = c->j + c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Prorunner 2 module", PWF_ProRunner_v2);

    if (c->save_status == PW_GOOD)
        c->i += 2; /* -1 should do but call it "just to be sure" :) */
}

/*
 *   ProRunner2.c   1996-1999 (c) Asle / ReDoX
 *
 * Converts ProRunner v2 packed MODs back to Protracker
 ********************************************************
 * 12 april 1999 : Update
 *   - no more open() of input file ... so no more fread() !.
 *     It speeds-up the process quite a bit :).
 *
 * 28 Nov 1999 : Update
 *   - optimized code for size and speed (again :)
 *     Heh, 1/5 shorter now !.
 *
 * 23 aug 2001 : debug
 *   - "repeat last note used bug" pointed out by Markus Jaegermeister !.
 *     thanks !.
 */
void pw_depack_PRUN2(pw_ctx *c)
{
    uint8_t  poss[37][2];
    uint8_t  Voices[4][4];
    uint8_t  Whatever[1024];
    uint8_t  note;
    int32_t  WholeSampleSize = 0;
    int32_t  i = 0, j = 0;
    int32_t  Where = c->start; /* main pointer to prevent fread() */
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    memset(Voices, 0, sizeof Voices);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(Whatever, 0, sizeof Whatever);
    /* title */
    pw_fwrite(out, Whatever, 20);

    Where += 8;

    for (i = 0; i < 31; i++)
    {
        /*sample name*/
        pw_fwrite(out, Whatever, 22);
        /* size */
        WholeSampleSize += (((IN(Where) * 256) + IN(Where + 1)) * 2);
        pw_fwrite_in(out, Where, 8);
        Where += 8;
    }

    pw_fwrite_in(out, Where, 1);
    Where += 1;

    /* noisetracker byte */
    pw_fwrite_in(out, Where, 1);
    Where += 1;

    Whatever[256] = 0x00;
    for (i = 0; i < 128; i++)
    {
        pw_fwrite_in(out, Where, 1);
        Whatever[256] = (IN(Where) > Whatever[256]) ? IN(Where) : Whatever[256];
        Where += 1;
    }

    Whatever[0] = 'M';
    Whatever[1] = '.';
    Whatever[2] = 'K';
    Whatever[3] = '.';
    pw_fwrite(out, Whatever, 4);

    /* pattern data stuff */
    Where = c->start + 770;
    for (i = 0; i <= Whatever[256]; i++)
    {
        for (j = 0; j < 256; j++)
        {
            Whatever[0] = IN(Where++);
            Whatever[100] = Whatever[101] = Whatever[102] = Whatever[103] = 0x00;
            if (Whatever[0] == 0x80)
            {
                pw_fwrite(out, &Whatever[100], 4);
            }
            else if (Whatever[0] == 0xC0)
            {
                pw_fwrite(out, Voices[j % 4], 4);
            }
            else if (Whatever[0] != 0xC0)
            {
                Whatever[1] = IN(Where++);
                Whatever[2] = IN(Where++);

                /* poss[] has 37 entries; the note can be 0..127 */
                note = (uint8_t)(Whatever[0] >> 1);
                Whatever[100] = (uint8_t)((Whatever[1] & 0x80) >> 3);
                Whatever[100] |= (note < 37) ? poss[note][0] : 0;
                Whatever[101] = (note < 37) ? poss[note][1] : 0;
                Whatever[102] = (uint8_t)((Whatever[1] & 0x70) << 1);
                Whatever[102] |= (uint8_t)((Whatever[0] & 0x01) << 4);
                Whatever[102] |= (Whatever[1] & 0x0f);
                Whatever[103] = Whatever[2];

                pw_fwrite(out, &Whatever[100], 4);

                /* rol previous values */
                Voices[j % 4][0] = Whatever[100];
                Voices[j % 4][1] = Whatever[101];
                Voices[j % 4][2] = Whatever[102];
                Voices[j % 4][3] = Whatever[103];
            }
        }
    }

    /* sample data */
    pw_fwrite_in(out, Where, WholeSampleSize);

    /* crap */
    pw_crap("   ProRunner v2   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
