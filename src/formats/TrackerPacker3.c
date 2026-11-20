/*
 * TrackerPacker3.c - Tracker Packer v3 (upstream r/TrackerPacker3.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_TP3(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* number of sample */
    c->l = ((IN(c->start + 28) * 256) +
            IN(c->start + 29));
    if ((((c->l / 8) * 8) != c->l) || (c->l == 0))
        return PW_BAD;
    c->l /= 8;
    /* c->l is the number of sample */

    /* test finetunes */
    for (c->k = 0; c->k < c->l; c->k++)
    {
        if (IN(c->start + 30 + c->k * 8) > 0x0f)
            return PW_BAD;
    }

    /* test volumes */
    for (c->k = 0; c->k < c->l; c->k++)
    {
        if (IN(c->start + 31 + c->k * 8) > 0x40)
            return PW_BAD;
    }

    /* test sample sizes */
    c->wss = 0;
    for (c->k = 0; c->k < c->l; c->k++)
    {
        /* size */
        c->j = (IN(c->start + c->k * 8 + 32) * 256) + IN(c->start + c->k * 8 + 33);
        /* loop start */
        c->m = (IN(c->start + c->k * 8 + 34) * 256) + IN(c->start + c->k * 8 + 35);
        /* loop size */
        c->n = (IN(c->start + c->k * 8 + 36) * 256) + IN(c->start + c->k * 8 + 37);
        c->j *= 2;
        c->m *= 2;
        c->n *= 2;
        if ((c->j > 0xFFFF) || (c->m > 0xFFFF) || (c->n > 0xFFFF))
            return PW_BAD;
        if ((c->m + c->n) > (c->j + 2))
            return PW_BAD;
        if ((c->m != 0) && (c->n == 0))
            return PW_BAD;
        c->wss += c->j;
    }
    if (c->wss <= 4)
        return PW_BAD;

    /* pattern list size */
    c->j = IN(c->start + c->l * 8 + 31);
    if ((c->j == 0) || (c->j > 128))
        return PW_BAD;

    /* c->j is the size of the pattern list */
    /* c->l is the number of sample */
    /* c->wss is the sample data size */
    return PW_GOOD;
}

void pw_rip_TP3(pw_ctx *c)
{
    /* c->j is the size of the pattern list */
    /* c->l is the number of sample */
    /* c->wss is the sample data size */

    c->m = 0;
    for (c->k = 0; c->k < c->j; c->k++)
    {
        c->o = (IN(c->start + c->l * 8 + 32 + c->k * 2) * 256) + IN(c->start + c->l * 8 + 33 + c->k * 2);
        if (c->o > c->m)
            c->m = c->o;
    }
    /* c->m is the highest pattern number */
    c->m += 8;
    /* c->m is now the size of the track table list */
    c->n = 0;
    for (c->k = 0; c->k < (c->m / 2); c->k++)
    {
        c->o = (IN(c->start + c->l * 8 + 32 + c->j * 2 + c->k * 2) * 256) + IN(c->start + c->l * 8 + 33 + c->j * 2 + c->k * 2);
        if (c->o > c->n)
            c->n = c->o;
    }
    c->n += (34 + 8 * c->l + 2 * c->j + c->m);
    c->out_size = c->n;

    /* all vars are availlable now, save c->wss */

    /* now counting size of the last pattern ... pfiew .. */
    c->l = 0;
    for (c->j = 0; c->j < 64; c->j++)
    {
        if ((IN(c->start + c->n + c->l) & 0xC0) == 0xC0)
        {
            c->j += (0x100 - IN(c->start + c->n + c->l));
            c->j -= 1;
            c->l += 1;
            continue;
        }
        if ((IN(c->start + c->n + c->l) & 0xC0) == 0x80)
        {
            c->l += 2;
            continue;
        }
        c->l += 1;
        if ((IN(c->start + c->n + c->l) & 0x0F) == 0x00)
        {
            c->l += 1;
            continue;
        }
        c->l += 2;
    }

    c->out_size += c->l + 2; /* +2 for $0000 at the end .. */
    if (((c->out_size / 2) * 2) != c->out_size)
        c->out_size += 1;
    c->out_size += c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Tracker Packer v3 module", PWF_TP3);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1); /* 0 could be enough */
}

/*
 *   TrackerPacker_v3.c   1998 (c) Asle / ReDoX
 * Converts TP3 packed MODs back to PTK MODs
 */
void pw_depack_TP3(pw_ctx *c)
{
    uint8_t  c1, c2, c3;
    uint8_t  poss[37][2];
    uint8_t  whatever[1024];
    uint8_t  note, smp, fx, fxval;
    uint8_t  pat_max = 0x00;
    /* pat_max is a byte: at most 255 (the original's table had 128 rows) */
    int32_t  track_address[256][4];
    int32_t  i = 0, j = 0, k;
    int32_t  start_pat_address;
    int32_t  whole_sample_size = 0;
    int32_t  max_track_address = 0;
    int32_t  where = c->start; /* main pointer to prevent fread() */
    uint32_t fill;
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    memset(track_address, 0, sizeof track_address);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* title */
    where += 8;
    pw_fwrite_in(out, where, 20);
    where += 20;

    /* number of sample */
    j = (IN(where) * 256) + IN(where + 1);
    where += 2;
    j /= 8;

    memset(whatever, 0, sizeof whatever);
    for (i = 0; i < j; i++)
    {
        /* sample name */
        pw_fwrite(out, whatever, 22);

        /* size */
        whole_sample_size += (((IN(where + 2) * 256) + IN(where + 3)) * 2);
        pw_fwrite_in(out, where + 2, 2);

        /* write finetune & Volume */
        pw_fwrite_in(out, where, 2);

        /* loop start & Loop size */
        pw_fwrite_in(out, where + 4, 4);

        where += 8;
    }
    whatever[29] = 0x01;
    /* guard: with more than 31 samples the original counted i up through
     * the 32-bit wrap-around, writing 30 bytes 2^32 times. Output files stop
     * growing at PW_MAX_OUTPUT_FILE, so stop once that much was written. */
    fill = 0;
    while (i != 31 && fill <= PW_MAX_OUTPUT_FILE / 30)
    {
        pw_fwrite(out, whatever, 30);
        i++;
        fill++;
    }

    /* read size of pattern table (the test limits it to 1..128) */
    where += 1;
    whatever[256] = IN(where++); /* PatPos*/
    pw_fwrite(out, &whatever[256], 1);

    /* ntk byte */
    whatever[0] = 0x7f;
    pw_fwrite(out, &whatever[0], 1);

    for (i = 0; i < whatever[256]; i++)
    {
        whatever[i] = (uint8_t)(((IN(where) * 256) + IN(where + 1)) / 8);
        where += 2;
        if (whatever[i] > pat_max)
            pat_max = whatever[i];
    }

    /* read tracks addresses */
    /* bypass 4 bytes or not ?!? */
    /* Here, I choose not :) */
    for (i = 0; i <= pat_max; i++)
    {
        for (j = 0; j < 4; j++)
        {
            track_address[i][j] = (IN(where) * 256) + IN(where + 1);
            where += 2;
            if (track_address[i][j] > max_track_address)
                max_track_address = track_address[i][j];
        }
    }

    /* write pattern list */
    pw_fwrite(out, whatever, 128);

    /* ID string */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    start_pat_address = where + 2;

    /* pattern datas */
    for (i = 0; i <= pat_max; i++)
    {
        memset(whatever, 0, sizeof whatever);
        for (j = 0; j < 4; j++)
        {
            where = track_address[i][j] + start_pat_address;
            for (k = 0; k < 64; k++)
            {
                c1 = IN(where++);
                if ((c1 & 0xC0) == 0xC0)
                {
                    k += (0x100 - c1);
                    k -= 1;
                    continue;
                }
                if ((c1 & 0xC0) == 0x80)
                {
                    c2 = IN(where++);
                    fx = (uint8_t)((c1 >> 1) & 0x0f);
                    fxval = c2;
                    if ((fx == 0x05) || (fx == 0x06) || (fx == 0x0A))
                    {
                        if (fxval > 0x80)
                            fxval = (uint8_t)(0x100 - fxval);
                        else if (fxval <= 0x80)
                            fxval = (uint8_t)((fxval << 4) & 0xf0);
                    }
                    if (fx == 0x08)
                        fx = 0x00;
                    whatever[k * 16 + j * 4 + 2] = fx;
                    whatever[k * 16 + j * 4 + 3] = fxval;
                    continue;
                }

                c2 = IN(where++);
                smp = (uint8_t)(((c2 >> 4) & 0x0f) | ((c1 >> 2) & 0x10));
                if ((c1 & 0x40) == 0x40)
                    note = (uint8_t)(0x7f - c1);
                else
                    note = (uint8_t)(c1 & 0x3F);
                fx = (uint8_t)(c2 & 0x0F);
                if (fx == 0x00)
                {
                    whatever[k * 16 + j * 4] = (uint8_t)(smp & 0xf0);
                    /* poss[] has 37 entries; note can reach 63 on odd input */
                    whatever[k * 16 + j * 4] |= (note < 37) ? poss[note][0] : 0;
                    whatever[k * 16 + j * 4 + 1] = (note < 37) ? poss[note][1] : 0;
                    whatever[k * 16 + j * 4 + 2] = (uint8_t)((smp << 4) & 0xf0);
                    whatever[k * 16 + j * 4 + 2] |= fx;
                    continue;
                }
                c3 = IN(where++);
                if (fx == 0x08)
                    fx = 0x00;
                fxval = c3;
                if ((fx == 0x05) || (fx == 0x06) || (fx == 0x0A))
                {
                    if (fxval > 0x80)
                        fxval = (uint8_t)(0x100 - fxval);
                    else if (fxval <= 0x80)
                        fxval = (uint8_t)((fxval << 4) & 0xf0);
                }

                whatever[k * 16 + j * 4] = (uint8_t)(smp & 0xf0);
                whatever[k * 16 + j * 4] |= (note < 37) ? poss[note][0] : 0;
                whatever[k * 16 + j * 4 + 1] = (note < 37) ? poss[note][1] : 0;
                whatever[k * 16 + j * 4 + 2] = (uint8_t)((smp << 4) & 0xf0);
                whatever[k * 16 + j * 4 + 2] |= fx;
                whatever[k * 16 + j * 4 + 3] = fxval;
            }
            if (where > max_track_address)
                max_track_address = where;
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* Sample data */
    if (((max_track_address / 2) * 2) != max_track_address)
        max_track_address += 1;
    pw_fwrite_in(out, max_track_address, whole_sample_size);

    pw_crap(" Tracker Packer 3 ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
