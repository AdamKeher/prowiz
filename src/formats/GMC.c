/*
 * GMC.c - Game Music Creator (upstream r/GMC.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/*
 * 20100902 - some test enhancements - less fake
 */
int16_t pw_test_GMC(pw_ctx *c)
{
    /* test #1 (c->start still holds the value of the previous test, as in
     * the original) */
    if ((c->i < 7) || ((c->start + 444) > c->in_size))
        return PW_BAD;
    c->start = (int32_t)(c->i - 7);

    /* samples descriptions */
    c->wss = 0;
    c->j = 0;
    for (c->k = 0; c->k < 15; c->k++)
    {
        c->o = (IN(c->start + 16 * c->k + 4) * 256) + IN(c->start + 16 * c->k + 5);
        c->n = (IN(c->start + 16 * c->k + 12) * 256) + IN(c->start + 16 * c->k + 13);
        c->o *= 2;
        /* volumes */
        if (IN(c->start + 7 + (16 * c->k)) > 0x40)
            return PW_BAD;
        /* size */
        if (c->o > 0xFFFF)
            return PW_BAD;
        if (c->n > c->o)
            return PW_BAD;
        c->wss += c->o;
        if (c->o != 0)
            c->j = c->k + 1;
    }
    if (c->wss <= 4)
        return PW_BAD;
    /* c->j is the highest not null sample */

    /* pattern table size */
    if ((IN(c->start + 243) > 0x64) ||
        (IN(c->start + 243) == 0x00))
        return PW_BAD;

    /* pattern order table */
    c->l = 0;
    for (c->n = 0; c->n < 100; c->n++)
    {
        c->k = ((IN(c->start + 244 + c->n * 2) * 256) +
                IN(c->start + 245 + c->n * 2));
        if (((c->k / 1024) * 1024) != c->k)
            return PW_BAD;
        c->l = ((c->k / 1024) > c->l) ? c->k / 1024 : c->l;
    }
    c->l += 1;
    /* c->l is the number of pattern */
    /* 20100822 - was wrong when there was only one pattern */
    if (c->l > 0x64)
        return PW_BAD;

    /* test pattern data */
    c->o = IN(c->start + 243);
    c->m = 0;
    for (c->k = 0; c->k < c->l; c->k++)
    {
        for (c->n = 0; c->n < 256; c->n++)
        {
            if (IN(c->start + 444 + (c->k * 1024) + (c->n * 4)) > 0x03)
                return PW_BAD;
            /* 20010902:new test if note pitch too small */
            if ((IN(c->start + 444 + (c->k * 1024) + (c->n * 4)) == 0x00) &&
                (IN(c->start + 444 + (c->k * 1024) + (c->n * 4) + 1) < 0x71) &&
                (IN(c->start + 444 + (c->k * 1024) + (c->n * 4) + 1) > 0x00))
                return PW_BAD;
            /* 20100902 - wrong test with 0x90 instead of 0x09 - less fake found now */
            if (((IN(c->start + 444 + (c->k * 1024) + (c->n * 4) + 2) & 0x0f) >= 0x09) &&
                ((IN(c->start + 444 + (c->k * 1024) + (c->n * 4) + 2) & 0x0f) != 0x0f))
                return PW_BAD;
            /* 20100822 - the test of the sample number against the highest
             * sample was removed: there seem to exist GMC with sample number
             * higher than the actual number of sample saved ... seen in
             * Jumping Jack Son game */
            /* test volume effect if value is > 64 */
            if (((IN(c->start + 444 + c->k * 1024 + c->n * 4 + 2) & 0x0f) == 3) &&
                (IN(c->start + 444 + c->k * 1024 + c->n * 4 + 3) > 0x64))
                return PW_BAD;
            if (((IN(c->start + 444 + c->k * 1024 + c->n * 4 + 2) & 0x0f) == 4) &&
                (IN(c->start + 444 + c->k * 1024 + c->n * 4 + 3) > 0x63))
                return PW_BAD;
            if (((IN(c->start + 444 + c->k * 1024 + c->n * 4 + 2) & 0x0f) == 5) &&
                (IN(c->start + 444 + c->k * 1024 + c->n * 4 + 3) > c->o + 1))
                return PW_BAD;
            if (((IN(c->start + 444 + c->k * 1024 + c->n * 4 + 2) & 0x0f) == 6) &&
                (IN(c->start + 444 + c->k * 1024 + c->n * 4 + 3) >= 0x02))
                return PW_BAD;
            if (((IN(c->start + 444 + c->k * 1024 + c->n * 4 + 2) & 0x0f) == 7) &&
                (IN(c->start + 444 + c->k * 1024 + c->n * 4 + 3) >= 0x02))
                return PW_BAD;
            if (((IN(c->start + 444 + c->k * 1024 + c->n * 4) & 0x0f) > 0x00) ||
                (IN(c->start + 445 + c->k * 1024 + c->n * 4) > 0x00))
                c->m = 1;
        }
    }
    if (c->m == 0)
    {
        /* only empty notes */
        return PW_BAD;
    }
    /* c->wss is the whole sample size */

    return PW_GOOD;
}

void pw_rip_GMC(pw_ctx *c)
{
    /* c->l is still the number of pattern to play */
    /* c->wss is already the whole sample size */

    c->out_size = c->wss + (c->l * 1024) + 444;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Game Music Creator module", PWF_GMC);

    if (c->save_status == PW_GOOD)
        c->i += 444; /* after header */
}

/*
 *   Game_Music_Creator.c   1997 (c) Sylvain "Asle" Chipaux
 *
 * Depacks musics in the Game Music Creator format and saves in ptk.
 *
 * update: 23/08/10
 *   - clean up
 */
void pw_depack_GMC(pw_ctx *c)
{
    uint8_t  whatever[1084];
    uint8_t  max = 0x00;
    int32_t  whole_sample_size = 0;
    int32_t  i = 0, j = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* title */
    memset(whatever, 0, sizeof whatever);

    /* read and write whole header */
    for (i = 0; i < 15; i++)
    {
        whatever[(i * 30) + 42] = IN(where + 4);
        whatever[(i * 30) + 43] = IN(where + 5);
        whole_sample_size += (((IN(where + 4) * 256) + IN(where + 5)) * 2);
        whatever[(i * 30) + 44] = IN(where + 6);
        whatever[(i * 30) + 45] = IN(where + 7);
        whatever[(i * 30) + 48] = IN(where + 12);
        whatever[(i * 30) + 49] = IN(where + 13);

        if ((whatever[(i * 30) + 48] == 0x00) && (whatever[(i * 30) + 49] == 0x02))
            whatever[(i * 30) + 49] = 0x01;

        /* loop start stuff - must check if there's a loop */
        if (((IN(where + 12) * 256) + IN(where + 13)) > 2)
        {
            /* ok, there's a loop size. use addresses to get the loop start */
            /* I know, that could be done some other way ... */
            unsigned char cc = 0x00, d;
            if (IN(where + 3) > IN(where + 11))
            {
                d = (unsigned char)(IN(where + 11) + 256 - IN(where + 3));
                cc += 1;
            }
            else
                d = (unsigned char)(IN(where + 11) - IN(where + 3));
            if (IN(where + 2) > IN(where + 10))
                cc += (IN(where + 10) + 256 - IN(where + 2));
            else
                cc += IN(where + 10) - IN(where + 2);
            /* other bytes _must_ be 0 */
            whatever[(i * 30) + 46] = (uint8_t)(cc / 2);
            whatever[(i * 30) + 47] = (uint8_t)(d / 2);
        }

        where += 16;
    }

    /* pattern list size */
    where = c->start + 0xF3;
    whatever[950] = IN(where++);
    whatever[951] = 0x7F;

    /* read and write size of pattern list */
    max = 0x00;
    for (i = 0; i < 100; i++)
    {
        whatever[952 + i] = (uint8_t)(((IN(where) * 256) + IN(where + 1)) / 1024);
        where += 2;
        if (whatever[952 + i] > max)
            max = whatever[952 + i];
    }

    /* write ID */
    whatever[1080] = 'M';
    whatever[1081] = '.';
    whatever[1082] = 'K';
    whatever[1083] = '.';
    pw_fwrite(out, whatever, 1084);

    /* pattern data */
    where = c->start + 444;
    for (i = 0; i <= max; i++)
    {
        memset(whatever, 0, 1024);
        for (j = 0; j < 1024; j++)
            whatever[j] = IN(where++);
        for (j = 0; j < 256; j++)
        {
            switch (whatever[(j * 4) + 2] & 0x0f)
            {
            case 0: /* no fx -> empty fxval */
                whatever[(j * 4) + 3] = 0x00;
                break;
            case 3: /* replace by C */
                whatever[(j * 4) + 2] += 0x09;
                break;
            case 4: /* replace by D */
                whatever[(j * 4) + 2] += 0x09;
                break;
            case 5: /* replace by B */
                whatever[(j * 4) + 2] += 0x06;
                break;
            case 6: /* replace by E0 */
                whatever[(j * 4) + 2] += 0x08;
                break;
            case 7: /* replace by E0 */
                whatever[(j * 4) + 2] += 0x07;
                break;
            case 8: /* replace by F */
                whatever[(j * 4) + 2] += 0x07;
                break;
            default:
                break;
            }
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap("Game Music Creator", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
