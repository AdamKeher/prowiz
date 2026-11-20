/*
 * FuchsTracker.c - Fuchs Tracker module (upstream r/FuchsTracker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_FuchsTracker(pw_ctx *c)
{
    /* test #1 */
    if (c->i < 192)
        return PW_BAD;
    c->start = (int32_t)(c->i - 192);

    /* all sample size */
    c->j = ((IN(c->start + 10) * 256 * 256 * 256) +
            (IN(c->start + 11) * 256 * 256) +
            (IN(c->start + 12) * 256) +
            IN(c->start + 13));
    if ((c->j <= 2) || (c->j >= (65535 * 16)))
        return PW_BAD;

    /* samples descriptions */
    c->m = 0;
    for (c->k = 0; c->k < 16; c->k++)
    {
        /* size */
        c->o = (IN(c->start + c->k * 2 + 14) * 256) + IN(c->start + c->k * 2 + 15);
        /* loop start */
        c->n = (IN(c->start + c->k * 2 + 78) * 256) + IN(c->start + c->k * 2 + 79);

        /* volumes */
        if (IN(c->start + 46 + c->k * 2) > 0x40)
            return PW_BAD;
        /* size < loop start ? */
        if (c->o < c->n)
            return PW_BAD;
        c->m += c->o;
    }

    /* c->m is the size of all samples (in descriptions) */
    /* c->j is the sample data sizes (header) */
    /* size<2  or  size > header sample size ? */
    if ((c->m <= 2) || (c->m > c->j))
        return PW_BAD;

    /* get highest pattern number in pattern list */
    c->k = 0;
    for (c->j = 0; c->j < 40; c->j++)
    {
        c->n = IN(c->start + c->j * 2 + 113);
        if (c->n > 40)
            return PW_BAD;
        if (c->n > c->k)
            c->k = c->n;
    }

    /* c->m is the size of all samples (in descriptions) */
    /* c->k is the highest pattern data -1 */
    /* input file not long enough ? */
    c->k += 1;
    c->k *= 1024;
    if ((c->k + 200) > (uint32_t)c->in_size)
        return PW_BAD;

    /* c->m is the size of all samples (in descriptions) */
    /* c->k is the pattern data size */

    return PW_GOOD;
}

void pw_rip_FuchsTracker(pw_ctx *c)
{
    /* c->m is the size of all samples (in descriptions) */
    /* c->k is the pattern data size */

    /* 204 = 200 (header) + 4 ("INST" id) */
    c->out_size = c->m + c->k + 204;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Fuchs Tracker module", PWF_FuchsTracker);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 195); /* 192 should do but call it "just to be sure" :) */
}

/*
 *   FuchsTracker.c   1999 (c) Sylvain "Asle" Chipaux
 *
 * Depacks Fucks Tracker modules
 *
 * Last update: 30/11/99
 *   - removed open() (and other fread()s and the like)
 *   - general Speed & Size Optmizings
 *   - small bug correction with loops (thx to Thomas Neumann
 *     for pointing this out !)
 * Another update : 23 nov 2003
 *   - used htonl() so that use of addy is now portable on 68k archs
 */
void pw_depack_FuchsTracker(pw_ctx *c)
{
    uint8_t  whatever[1080];
    uint8_t *pat;
    uint8_t  c1 = 0x00;
    int32_t  sample_sizes[16];
    int32_t  loop_start[16];
    uint32_t i = 0, j = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(sample_sizes, 0, sizeof sample_sizes);
    memset(loop_start, 0, sizeof loop_start);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* write empty ptk header */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 1080);

    /* write title */
    pw_fseek(out, 0, 0);
    pw_fwrite_in(out, where, 10);
    where += 10;

    /* bypass all sample data size */
    where += 4;

    /* read/write sample sizes */
    /* have to halve these :( */
    for (i = 0; i < 16; i++)
    {
        pw_fseek(out, 42 + i * 30, 0);
        whatever[0] = IN(where);
        whatever[1] = IN(where + 1);
        sample_sizes[i] = (whatever[0] * 256) + whatever[1];
        whatever[1] /= 2;
        if ((whatever[0] / 2) * 2 != whatever[0])
        {
            if (whatever[1] < 0x80)
                whatever[1] += 0x80;
            else
            {
                whatever[1] -= 0x80;
                whatever[0] += 0x01;
            }
        }
        whatever[0] /= 2;
        pw_fwrite(out, whatever, 2);
        where += 2;
    }

    /* read/write volumes */
    for (i = 0; i < 16; i++)
    {
        pw_fseek(out, 45 + i * 30, 0);
        where += 1;
        pw_fwrite_in(out, where++, 1);
    }

    /* read/write loop start */
    /* have to halve these :( */
    for (i = 0; i < 16; i++)
    {
        pw_fseek(out, 46 + i * 30, 0);
        whatever[0] = IN(where);
        whatever[1] = IN(where + 1);
        loop_start[i] = (whatever[0] * 256) + whatever[1];
        whatever[1] /= 2;
        if ((whatever[0] / 2) * 2 != whatever[0])
        {
            if (whatever[1] < 0x80)
                whatever[1] += 0x80;
            else
            {
                whatever[1] -= 0x80;
                whatever[0] += 0x01;
            }
        }
        whatever[0] /= 2;
        pw_fwrite(out, whatever, 2);
        where += 2;
    }

    /* write replen */
    /* have to halve these :( */
    whatever[128] = 0x01;
    for (i = 0; i < 16; i++)
    {
        pw_fseek(out, 48 + i * 30, 0);
        j = (uint32_t)(sample_sizes[i] - loop_start[i]);
        if ((j == 0) || (loop_start[i] == 0))
        {
            pw_fwrite(out, &whatever[127], 2);
            continue;
        }

        j /= 2;
        /* upstream: bytes 2 and 3 of htonl(j) */
        whatever[0] = (uint8_t)(j >> 8);
        whatever[1] = (uint8_t)j;
        pw_fwrite(out, whatever, 2);
    }

    /* fill replens up to 31st sample wiz $0001 */
    whatever[49] = 0x01;
    for (i = 16; i < 31; i++)
    {
        pw_fseek(out, 48 + i * 30, 0);
        pw_fwrite(out, &whatever[48], 2);
    }

    /* that's it for the samples ! */
    /* now, the pattern list */

    /* read number of pattern to play */
    pw_fseek(out, 950, 0);
    /* bypass empty byte (saved wiz a WORD ..) */
    where += 1;
    pw_fwrite_in(out, where++, 1);

    /* write ntk byte */
    whatever[0] = 0x7f;
    pw_fwrite(out, whatever, 1);

    /* read/write pattern list */
    for (i = 0; i < 40; i++)
    {
        where += 1;
        pw_fwrite_in(out, where++, 1);
    }

    /* write ptk's ID */
    pw_fseek(out, 0, 2);
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* now, the pattern data */

    /* bypass the "SONG" ID */
    where += 4;

    /* read pattern data size */
    j = ((IN(where) * 256 * 256 * 256) +
         (IN(where + 1) * 256 * 256) +
         (IN(where + 2) * 256) +
         IN(where + 3));
    where += 4;

    if ((uint64_t)pw_ftell(out) + j > PW_MAX_OUTPUT_FILE)
    {
        /* j comes straight from the file. Such a write is dropped by the
         * output size cap anyway, so skip the conversion and the huge buffer
         * (the original malloc()ed j bytes and read far past its input). */
        pw_fzero(out, j);
        where = (int32_t)((uint32_t)where + ((j + 3) & ~3u));
    }
    else
    {
        /* +4: the loop below writes up to 3 bytes past j when j%4 != 0 */
        pat = (uint8_t *)calloc((size_t)j + 4, 1);
        if (pat == NULL)
        {
            c->fatal = PW_ERR_NOMEM;
            pw_fdiscard(out);
            return;
        }

        /* convert shits */
        for (i = 0; i < j; i += 4)
        {
            pat[i]     = IN(where++);
            pat[i + 1] = IN(where++);
            pat[i + 2] = IN(where++);
            pat[i + 3] = IN(where++);
            /* convert fx C arg back to hex value */
            if ((pat[i + 2] & 0x0f) == 0x0c)
            {
                c1 = pat[i + 3];
                if (c1 <= 9) { pat[i + 3] = c1; continue; }
                if ((c1 >= 16) && (c1 <= 25)) { pat[i + 3] = (uint8_t)(c1 - 6); continue; }
                if ((c1 >= 32) && (c1 <= 41)) { pat[i + 3] = (uint8_t)(c1 - 12); continue; }
                if ((c1 >= 48) && (c1 <= 57)) { pat[i + 3] = (uint8_t)(c1 - 18); continue; }
                if ((c1 >= 64) && (c1 <= 73)) { pat[i + 3] = (uint8_t)(c1 - 24); continue; }
                if ((c1 >= 80) && (c1 <= 89)) { pat[i + 3] = (uint8_t)(c1 - 30); continue; }
                if ((c1 >= 96) && (c1 <= 100)) { pat[i + 3] = (uint8_t)(c1 - 36); continue; }
            }
        }

        /* write pattern data */
        pw_fwrite(out, pat, j);
        free(pat);
    }

    /* read/write sample data */
    where += 4;
    for (i = 0; i < 16; i++)
    {
        if (sample_sizes[i] != 0)
        {
            pw_fwrite_in(out, where, sample_sizes[i]);
            where += sample_sizes[i];
        }
    }

    /* crap */
    pw_crap("  Fuchs Tracker   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
