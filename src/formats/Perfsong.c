/*
 * Perfsong.c - Perfect Song Player by Seg/Darkness ("PERFSONG")
 *              (upstream r/Perfsong.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* (a*256*256*256)+(b*256*256)+(c*256)+d in int arithmetic: the original
 * relied on wrap-around, which uint32_t arithmetic gives without UB. */
static uint32_t perf_be32(pw_ctx *c, int64_t at)
{
    return ((uint32_t)IN(at) << 24) + ((uint32_t)IN(at + 1) << 16) +
           ((uint32_t)IN(at + 2) << 8) + (uint32_t)IN(at + 3);
}

int16_t pw_test_PERFSONG(pw_ctx *c)
{
    /* test 1 */
    c->start = (int32_t)c->i;
    if ((c->in_size - c->start) < 12)
        return PW_BAD;

    /* whole file size */
    c->j = perf_be32(c, c->start + 8) + 12;

    /* test 2 */
    /* too big a file ? or too small ? */
    if (((c->start + c->j) > (uint32_t)c->in_size) || (c->j < 802))
        return PW_BAD;
    /* c->j is the whole packed file size */

    return PW_GOOD;
}

void pw_rip_PERFSONG(pw_ctx *c)
{
    /* c->j is the whole file size */

    c->out_size = c->j;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Perfect Song module", PWF_PerfSong);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}

/*
 *   PERFSONG.c   20100120 (c) Asle / ReDoX
 *
 * Perfect Song Player to Protracker.
 * format/replayer by Seg/Darkness (~1997)
 */
void pw_depack_PERFSONG(pw_ctx *c)
{
    uint8_t  c1 = 0x00, c2 = 0x00;
    uint8_t  poss[37][2];
    uint8_t  max = 0x00;
    uint8_t  note, smp, fx, fx_val;
    uint8_t  whatever[1085];
    int32_t  i = 0, j = 0, k = 0, l;
    int32_t  where = c->start;
    int32_t  smp_addresses[31];
    int32_t  smp_sizes[31];
    int32_t  bnr_full_size = 0;
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(whatever, 0, sizeof whatever);

    /* retrieve full size to fetch sample text at the end */
    bnr_full_size = (int32_t)(perf_be32(c, where + 8) + 12);
    bnr_full_size = (int32_t)((uint32_t)bnr_full_size - 702); /* to reach the beginning of the text area */
    pw_printf(c, "BNRFullSize at : %d\n", bnr_full_size);

    /* get title */
    for (l = 0; l < 20; l++)
        whatever[l] = IN((int32_t)((uint32_t)where + (uint32_t)bnr_full_size + (uint32_t)l));
    bnr_full_size = (int32_t)((uint32_t)bnr_full_size + 0x14);

    /* 31 samples */
    where += 12; /* points on first psmp desc */
    for (i = 0; i < 31; i++)
    {
        /* read sample address */
        smp_addresses[i] = (int32_t)perf_be32(c, where);

        /* read loop start address */
        j = (int32_t)perf_be32(c, where + 4);
        /* get smptext */
        for (l = 0; l < 21; l++)
            whatever[i * 30 + 20 + l] =
                IN((int32_t)((uint32_t)c->start + (uint32_t)bnr_full_size + (uint32_t)l));
        bnr_full_size = (int32_t)((uint32_t)bnr_full_size + 0x16);
        /* read & write sample size */
        whatever[i * 30 + 42] = IN(where + 8);
        whatever[i * 30 + 43] = IN(where + 9);
        smp_sizes[i] = (((whatever[i * 30 + 42] * 256) + whatever[i * 30 + 43]) * 2);
        whatever[i * 30 + 44] = IN(where + 12); /* fine ? */
        whatever[i * 30 + 45] = IN(where + 13); /* vol */
        whatever[i * 30 + 48] = IN(where + 10); /* replen */
        whatever[i * 30 + 49] = IN(where + 11);

        /* calculate loop start value */
        k = ((int32_t)((uint32_t)j - (uint32_t)smp_addresses[i])) / 2;

        /* write loop start: bytes 2 and 3 of htonl(k) */
        whatever[i * 30 + 46] = (uint8_t)((uint32_t)k >> 8);
        whatever[i * 30 + 47] = (uint8_t)k;

        where += 16;
    }

    /* patternlist size */
    whatever[950] = (uint8_t)(IN(where + 1) + 1);
    whatever[951] = 0x7f;
    where += 2;

    /* pattern table */
    max = 0x00;
    for (c1 = 0; c1 < 128; c1++)
    {
        j = (int32_t)perf_be32(c, where);
        whatever[952 + c1] = (uint8_t)(((int32_t)((uint32_t)j - 0x3fe)) / 1024);
        if (whatever[952 + c1] > max)
            max = whatever[952 + c1];
        where += 4;
    }
    max += 1;

    /* write Protracker's ID */
    whatever[1080] = 'M';
    whatever[1081] = '.';
    whatever[1082] = 'K';
    whatever[1083] = '.';

    pw_fwrite(out, whatever, 1084);

    /* pattern data */
    /* where is already pointing on the 1st note of the 1st pattern */
    for (i = 0; i < max; i++)
    {
        memset(whatever, 0, sizeof whatever);
        for (j = 0; j < 256; j++)
        {
            smp = IN(where + j * 4 + 1);
            note = IN(where + j * 4);
            fx = IN(where + j * 4 + 2);
            fx_val = IN(where + j * 4 + 3);
            switch (fx)
            {
            case 0x00: /* no Fx */
                fx = 0x00;
                break;
            case 0x04: /* arpeggio */
                fx = 0x00;
                break;
            case 0x08: /* portamento up */
                fx = 0x01;
                break;
            case 0x0C: /* portamento down */
                fx = 0x02;
                break;
            case 0x10: /* tone portamento with no FxVal */
                fx = 0x03;
                break;
            case 0x14: /* tone portamento */
                fx = 0x03;
                break;
            case 0x18: /* vibrato with no FxVal */
                fx = 0x04;
                break;
            case 0x1C: /* vibrato */
                fx = 0x04;
                break;
            case 0x20: /* tone portamento + vol slide DOWN */
                fx = 0x05;
                break;
            case 0x24: /* vibrato + volume slide UP */
                fx = 0x06;
                c1 = (uint8_t)((fx_val << 4) & 0xf0);
                c2 = (uint8_t)((fx_val >> 4) & 0x0f);
                fx_val = c1 | c2;
                break;
            case 0x28: /* vibrato + volume slide DOWN */
                fx = 0x06;
                break;
            case 0x2c: /* vibrato + volume slide DOWN */
                fx = 0x06;
                break;
            case 0x30: /* tremolo */
                fx = 0x07;
                break;
            case 0x38: /* sample offset */
                fx = 0x09;
                break;
            case 0x3C: /* volume slide up */
                fx = 0x0A;
                c1 = (uint8_t)((fx_val << 4) & 0xf0);
                c2 = (uint8_t)((fx_val >> 4) & 0x0f);
                fx_val = c1 | c2;
                break;
            case 0x40: /* volume slide down */
                fx = 0x0A;
                break;
            case 0x44: /* position jump */
                fx = 0x0B;
                break;
            case 0x48: /* set volume */
                fx = 0x0C;
                break;
            case 0x4C: /* pattern break */
                fx = 0x0D;
                break;
            case 0x50: /* set speed */
                fx = 0x0F;
                break;
            case 0x58: /* set filter */
                fx = 0x0E;
                fx_val = 0x01;
                break;
            case 0x5C: /* fine slide up */
                fx = 0x0E;
                fx_val |= 0x10;
                break;
            case 0x60: /* fine slide down */
                fx = 0x0E;
                fx_val |= 0x20;
                break;
            case 0x74: /* fine volume slide up */
                fx = 0x0E;
                fx_val |= 0xa0;
                break;
            case 0x78: /* fine volume slide down */
                fx = 0x0E;
                fx_val |= 0xb0;
                break;
            case 0x7c: /* pattern delay */
                fx = 0x0E;
                fx_val |= 0xe0;
                break;
            default:
                /* ftell() returned a 32 bit long on the original build */
                pw_printf(c, "%x : at %d (out:%d)\n", fx, where + (j * 4), (int32_t)pw_ftell(out));
                fx = 0x00;
                break;
            }
            note /= 2;
            c1 = (smp >> 4) & 0x0f;
            c2 = (uint8_t)((smp << 4) & 0xf0);
            smp = c1 | c2;
            whatever[j * 4] = (smp & 0xf0);
            /* poss[] has 37 entries; the note index can reach 127 */
            whatever[j * 4] |= (note < 37) ? poss[note][0] : 0;
            whatever[j * 4 + 1] = (note < 37) ? poss[note][1] : 0;
            whatever[j * 4 + 2] = (uint8_t)((smp << 4) & 0xf0);
            whatever[j * 4 + 2] |= fx;
            whatever[j * 4 + 3] = fx_val;
        }
        where += 1024;
        pw_fwrite(out, whatever, 1024);
        pw_printf(c, "pattern %d written (Where : %d)\n", i, where);
    }

    /* sample data */
    for (i = 0; i < 31; i++)
    {
        if (smp_sizes[i] == 0)
            continue;
        pw_fwrite_in(out, (int32_t)((uint32_t)c->start + (uint32_t)smp_addresses[i]), smp_sizes[i]);
    }

    /* no crap as it destroys sample text */

    pw_fclose(out);

    pw_printf(c, "done\n");
}
