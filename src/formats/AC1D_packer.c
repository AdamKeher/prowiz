/*
 * AC1D_packer.c - AC1D Packer (upstream r/AC1D_packer.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_AC1D(pw_ctx *c)
{
    /* test #1 */
    if (pw_test_1_start(c, 2) == PW_BAD)
        return PW_BAD;

    /* test #2
     * 20121104 - testing also if the pattern list isn't empty */
    c->start = (int32_t)(c->i - 2);
    if ((IN(c->start) > 0x7f) ||
        (IN(c->start) == 0x00) ||
        ((c->start + 896) > c->in_size))
        return PW_BAD;

    /* test #4 */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if (IN(c->start + 10 + (8 * c->k)) > 0x0f)
            return PW_BAD;
    }

    /* test #5 */
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 768 + c->j) > 0x7f)
            return PW_BAD;
    }
    return PW_GOOD;
}

void pw_rip_AC1D(pw_ctx *c)
{
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
        c->wss += (((IN(c->start + 8 + c->j * 8) * 256) + IN(c->start + 9 + c->j * 8)) * 2);
    c->k = (IN(c->start + 4) * 256 * 256 * 256) +
           (IN(c->start + 5) * 256 * 256) +
           (IN(c->start + 6) * 256) +
           IN(c->start + 7);

    c->out_size = c->wss + c->k;

    c->convert = PW_GOOD;
    pw_save_rip(c, "AC1D Packed module", PWF_AC1D_packer);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 4); /* 3 should do but call it "just to be sure" :) */
}

/*
 *   ac1d.c   1996-1997 (c) Asle / ReDoX
 *
 * Converts AC1D packed MODs back to PTK MODs
 * thanks to Gryzor and his ProWizard tool ! ... without it, this prog
 * would not exist !!!
 */
void pw_depack_AC1D(pw_ctx *c)
{
    const uint8_t NO_NOTE = 0xff;
    uint8_t  c1, c2, c3, c4;
    uint8_t  whatever[1024];
    uint8_t  nbr_pat;
    uint8_t  poss[37][2];
    uint8_t  note, smp, fx, fxval;
    int32_t  sample_data_address;
    int32_t  whole_sample_size = 0;
    int32_t  pattern_addresses[128];
    int32_t  pattern_sizes[128];
    int32_t  siztrack1, siztrack2, siztrack3;
    int32_t  i, j, k;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(pattern_addresses, 0, sizeof pattern_addresses);
    memset(pattern_sizes, 0, sizeof pattern_sizes);

    pw_fill_ptk_table(poss);

    /* bypass ID */
    where += 4;

    sample_data_address = (IN(where) * 256 * 256 * 256) +
                          (IN(where + 1) * 256 * 256) +
                          (IN(where + 2) * 256) +
                          IN(where + 3);
    where += 4;

    /* write title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 20);

    for (i = 0; i < 31; i++)
    {
        pw_fwrite(out, whatever, 22);
        whole_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);
        pw_fwrite_in(out, where, 8);
        where += 8;
    }

    /* pattern addresses */
    for (nbr_pat = 0; nbr_pat < 128; nbr_pat++)
    {
        pattern_addresses[nbr_pat] = (IN(where) * 256 * 256 * 256) +
                                     (IN(where + 1) * 256 * 256) +
                                     (IN(where + 2) * 256) +
                                     IN(where + 3);
        where += 4;
        if (pattern_addresses[nbr_pat] == 0)
            break;
    }
    nbr_pat -= 1;

    /* nbr_pat is 255 when the first address is 0: the original then read and
     * wrote past both arrays (pattern_sizes is never used afterwards) */
    for (i = 0; i < (nbr_pat - 1); i++)
    {
        if (i < 127)
            pattern_sizes[i] = pattern_addresses[i + 1] - pattern_addresses[i];
    }
    (void)pattern_sizes;

    /* write number of pattern pos */
    /* write "noisetracker" byte */
    pw_fwrite_in(out, c->start, 2);

    /* go to pattern table .. */
    where = c->start + 0x300;

    /* pattern table */
    pw_fwrite_in(out, where, 128);
    where += 128;

    /* write ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* pattern data */
    for (i = 0; i < nbr_pat; i++)
    {
        /* past the 128 entries the original read neighbouring stack memory */
        where = c->start + ((i < 128) ? pattern_addresses[i] : 0);
        siztrack1 = (IN(where) * 256 * 256 * 256) +
                    (IN(where + 1) * 256 * 256) +
                    (IN(where + 2) * 256) +
                    IN(where + 3);
        where += 4;
        siztrack2 = (IN(where) * 256 * 256 * 256) +
                    (IN(where + 1) * 256 * 256) +
                    (IN(where + 2) * 256) +
                    IN(where + 3);
        where += 4;
        siztrack3 = (IN(where) * 256 * 256 * 256) +
                    (IN(where + 1) * 256 * 256) +
                    (IN(where + 2) * 256) +
                    IN(where + 3);
        where += 4;
        (void)siztrack1;
        (void)siztrack2;
        (void)siztrack3;
        memset(whatever, 0, sizeof whatever);
        for (k = 0; k < 4; k++)
        {
            for (j = 0; j < 64; j++)
            {
                note = smp = fx = fxval = 0x00;
                c1 = IN(where++);
                if ((c1 & 0x80) == 0x80)
                {
                    c4 = (uint8_t)(c1 & 0x7f);
                    j += (c4 - 1);
                    continue;
                }
                c2 = IN(where++);
                smp = (uint8_t)((c1 & 0xc0) >> 2);
                smp |= ((c2 >> 4) & 0x0f);
                note = (uint8_t)(c1 & 0x3f);
                if (note == 0x3f)
                    note = NO_NOTE;
                else if (note != 0x00)
                    note -= 0x0b;
                if (note == 0x00)
                    note += 0x01;
                whatever[j * 16 + k * 4] = (uint8_t)(smp & 0xf0);
                if (note != NO_NOTE)
                {
                    /* poss[] has 37 entries; corrupt notes go up to 0xfe */
                    whatever[j * 16 + k * 4] |= (note < 37) ? poss[note][0] : 0;
                    whatever[j * 16 + k * 4 + 1] = (note < 37) ? poss[note][1] : 0;
                }
                if ((c2 & 0x0f) == 0x07)
                {
                    fx = 0x00;
                    fxval = 0x00;
                    whatever[j * 16 + k * 4 + 2] = (uint8_t)((smp << 4) & 0xf0);
                    continue;
                }
                c3 = IN(where++);
                fx = (uint8_t)(c2 & 0x0f);
                fxval = c3;
                whatever[j * 16 + k * 4 + 2] = (uint8_t)((smp << 4) & 0xf0);
                whatever[j * 16 + k * 4 + 2] |= fx;
                whatever[j * 16 + k * 4 + 3] = fxval;
            }
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    where = c->start + sample_data_address;
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap ... */
    pw_crap("   AC1D Packer    ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
