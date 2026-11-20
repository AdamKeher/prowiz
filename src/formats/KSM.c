/*
 * KSM.c - Kefrens Sound Machine (upstream r/KSM.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_KSM(pw_ctx *c)
{
    c->start = (int32_t)c->i;
    if ((c->start + 1536) > c->in_size)
        return PW_BAD;

    /* test "a" */
    if (IN(c->start + 15) != 'a')
        return PW_BAD;

    /* test volumes */
    for (c->k = 0; c->k < 15; c->k++)
        if (IN(c->start + 54 + c->k * 32) > 0x40)
            return PW_BAD;

    /* test tracks data */
    /* first, get the highest track number .. */
    c->j = 0;
    for (c->k = 0; c->k < 1024; c->k++)
    {
        if (IN(c->start + c->k + 512) == 0xFF)
            break;
        if (IN(c->start + c->k + 512) > c->j)
            c->j = IN(c->start + c->k + 512);
    }
    if (c->k == 1024)
        return PW_BAD;
    if (c->j == 0)
        return PW_BAD;
    /* so, now, c->j is the highest track number (first is 00h !!) */
    /* real test on tracks data starts now */
    /* first, test if we don't get out of the file */
    if ((c->start + 1536 + c->j * 192 + 64 * 3) > (uint32_t)c->in_size)
        return PW_BAD;
    /* now testing tracks */
    for (c->k = 0; c->k <= c->j; c->k++)
        for (c->l = 0; c->l < 64; c->l++)
            if (IN(c->start + 1536 + c->k * 192 + c->l * 3) > 0x24)
                return PW_BAD;

    /* c->j is still the highest track number */
    return PW_GOOD;
}

void pw_rip_KSM(pw_ctx *c)
{
    /* c->j is the highest track number */

    c->wss = 0;
    for (c->k = 0; c->k < 15; c->k++)
        c->wss += ((IN(c->start + 52 + c->k * 32) * 256) + IN(c->start + 53 + c->k * 32));

    c->out_size = ((c->j + 1) * 192) + c->wss + 1536;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Kefrens Sound Machine module", PWF_KSM);

    if (c->save_status == PW_GOOD)
        c->i += 2; /* -1 should do but call it "just to be sure" :) */
}

/*
 *   Kefrens_Sound_Machine.c   1997 (c) Sylvain "Asle" Chipaux
 *
 * Depacks musics in the Kefrens Sound Machine format and saves in ptk.
 *
 * Another Update : 05 may 2001
 *      - added transciption for sample names
 * update 30/08/10
 *      - patternlist with only one pattern fixed
 *      - conversion to STK instead of PTK
 */

#define ON  1
#define OFF 2

void pw_depack_KSM(pw_ctx *c)
{
    static const uint8_t transco[] = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        '-', ':', '!', '~', '1', '2', '3', '4', '5', '6',
        '7', '8', '9', '0', ' ', ';'};
    uint8_t  whatever[1024];
    uint8_t  c1 = 0x00, c2 = 0x00, c5;
    uint8_t  track_numbers[128][4];
    uint8_t  track_numbers_real[128][4];
    uint8_t  track_datas[4][192];
    uint8_t  max = 0x00;
    uint8_t  poss[37][2];
    uint8_t  pat_pos;
    uint8_t  status = ON;
    int32_t  where = c->start;
    int32_t  whole_sample_size = 0;
    uint32_t i = 0, j = 0, k = 0;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(track_numbers, 0, sizeof track_numbers);
    memset(track_numbers_real, 0, sizeof track_numbers_real);

    pw_fill_ptk_table(poss);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite_in(out, where + 2, 13);
    pw_fwrite(out, whatever, 7); /* fill-up there */

    /* read and write whole header */
    where += 32;
    for (i = 0; i < 15; i++)
    {
        /* write name (transco[] has 42 entries: past it the original read
         * neighbouring stack memory) */
        for (k = 0; k < 15; k++)
            whatever[230 + k] = (IN(where + k) < sizeof transco) ? transco[IN(where + k)] : 0;
        pw_fwrite(out, &whatever[230], 22);
        /* size */
        c1 = IN(where + 20);
        c2 = IN(where + 21);
        k = (IN(where + 20) * 256) + IN(where + 21);
        whole_sample_size += k;
        c2 /= 2;
        if ((c1 / 2) * 2 != c1)
        {
            if (c2 < 0x80)
                c2 += 0x80;
            else
            {
                c2 -= 0x80;
                c1 += 0x01;
            }
        }
        c1 /= 2;
        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c2, 1);
        /* finetune */
        pw_fwrite(out, whatever, 1);
        /* volume */
        pw_fwrite_in(out, where + 22, 1);
        /* loop start */
        c1 = IN(where + 24);
        c2 = IN(where + 25);
        j = k - ((c1 * 256) + c2);
        c2 /= 2;
        if ((c1 / 2) * 2 != c1)
        {
            if (c2 < 0x80)
                c2 += 0x80;
            else
            {
                c2 -= 0x80;
                c1 += 0x01;
            }
        }
        c1 /= 2;
        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c2, 1);

        if (j != k)
        {
            /* write loop size: bytes 2 and 3 of htonl(j) */
            j /= 2;
            c1 = (uint8_t)(j >> 8);
            c2 = (uint8_t)j;
            pw_fwrite(out, &c1, 1);
            pw_fwrite(out, &c2, 1);
        }
        else
        {
            c1 = 0x00;
            c2 = 0x01;
            pw_fwrite(out, &c1, 1);
            pw_fwrite(out, &c2, 1);
        }
        where += 32;
    }

    /* pattern list */
    where = c->start + 512;
    for (pat_pos = 0x00; pat_pos < 128; pat_pos++)
    {
        track_numbers[pat_pos][0] = IN(where + pat_pos * 4);
        track_numbers[pat_pos][1] = IN(where + pat_pos * 4 + 1);
        track_numbers[pat_pos][2] = IN(where + pat_pos * 4 + 2);
        track_numbers[pat_pos][3] = IN(where + pat_pos * 4 + 3);
        if (track_numbers[pat_pos][0] == 0xFF)
            break;
        if (track_numbers[pat_pos][0] > max)
            max = track_numbers[pat_pos][0];
        if (track_numbers[pat_pos][1] > max)
            max = track_numbers[pat_pos][1];
        if (track_numbers[pat_pos][2] > max)
            max = track_numbers[pat_pos][2];
        if (track_numbers[pat_pos][3] > max)
            max = track_numbers[pat_pos][3];
    }

    /* write patpos */
    pw_fwrite(out, &pat_pos, 1);

    /* ntk byte */
    c1 = 0x7f;
    pw_fwrite(out, &c1, 1);

    /* sort tracks numbers */
    c5 = 0x00;
    for (i = 0; i < pat_pos; i++)
    {
        if (i == 0)
        {
            whatever[0] = c5;
            c5 += 0x01;
            continue;
        }
        for (j = 0; j < i; j++)
        {
            status = ON;
            for (k = 0; k < 4; k++)
            {
                if (track_numbers[j][k] != track_numbers[i][k])
                {
                    status = OFF;
                    break;
                }
            }
            if (status == ON)
            {
                whatever[i] = whatever[j];
                break;
            }
        }
        if (status == OFF)
        {
            whatever[i] = c5;
            c5 += 0x01;
        }
        status = ON;
    }
    /* c5 is the Max pattern number */

    /* create a real list of tracks numbers for the really existing patterns */
    c1 = 0x00;
    for (i = 0; i < pat_pos; i++)
    {
        if (i == 0)
        {
            track_numbers_real[c1][0] = track_numbers[i][0];
            track_numbers_real[c1][1] = track_numbers[i][1];
            track_numbers_real[c1][2] = track_numbers[i][2];
            track_numbers_real[c1][3] = track_numbers[i][3];
            c1 += 0x01;
            continue;
        }
        for (j = 0; j < i; j++)
        {
            status = ON;
            if (whatever[i] == whatever[j])
            {
                status = OFF;
                break;
            }
        }
        if (status == OFF)
            continue;
        track_numbers_real[c1][0] = track_numbers[i][0];
        track_numbers_real[c1][1] = track_numbers[i][1];
        track_numbers_real[c1][2] = track_numbers[i][2];
        track_numbers_real[c1][3] = track_numbers[i][3];
        c1 += 0x01;
        status = ON;
    }

    /* write pattern list */
    pw_fwrite(out, whatever, 128);

    /* (no "M.K." ID: the result is a 15 sample Soundtracker module) */

    /* pattern data */
    for (i = 0; i < c5; i++)
    {
        memset(whatever, 0, sizeof whatever);
        memset(track_datas, 0, sizeof track_datas);
        for (k = 0; k < 4; k++)
            for (j = 0; j < 192; j++)
                track_datas[k][j] = IN(c->start + 1536 + 192 * track_numbers_real[i][k] + j);

        for (j = 0; j < 64; j++)
        {
            for (k = 0; k < 4; k++)
            {
                /* poss[] has 37 entries; tracks the test did not check can
                 * hold notes up to 255 */
                uint8_t note = track_datas[k][j * 3];

                whatever[j * 16 + k * 4] = (note < 37) ? poss[note][0] : 0;
                whatever[j * 16 + k * 4 + 1] = (note < 37) ? poss[note][1] : 0;
                if ((track_datas[k][j * 3 + 1] & 0x0f) == 0x0D)
                    track_datas[k][j * 3 + 1] -= 0x03;
                whatever[j * 16 + k * 4 + 2] = track_datas[k][j * 3 + 1];
                whatever[j * 16 + k * 4 + 3] = track_datas[k][j * 3 + 2];
            }
        }

        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    pw_fwrite_in(out, c->start + 1536 + (192 * (max + 1)), whole_sample_size);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
