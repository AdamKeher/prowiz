/*
 * PolkaPacker.c - Polka Packer ("PWR." / "PSUX" ID) (upstream r/PolkaPacker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_Polka(pw_ctx *c)
{
    /* test #1 */
    if ((c->i < 0x438) || ((c->i + 0x830) > (uint32_t)c->in_size))
        return PW_BAD;

    /* test #2 */
    c->start = (int32_t)(c->i - 0x438);
    c->l = 0;
    /* (the original never resets c->wss here: it adds to what is left) */
    for (c->j = 0; c->j < 31; c->j++)
    {
        /* size */
        c->k = (((IN(c->start + 42 + 30 * c->j) * 256) + IN(c->start + 43 + 30 * c->j)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 46 + 30 * c->j) * 256) + IN(c->start + 47 + 30 * c->j)) * 2);
        /* loop size */
        c->n = (((IN(c->start + 48 + 30 * c->j) * 256) + IN(c->start + 49 + 30 * c->j)) * 2);
        c->wss += c->k;

        if (pw_test_smps((int32_t)c->k, (int32_t)(c->m / 2), (int32_t)c->n,
                         IN(c->start + 45 + 30 * c->j), IN(c->start + 44 + 30 * c->j)) == PW_BAD)
            return PW_BAD;
    }
    if (c->wss <= 2)
        return PW_BAD;

    /* test #3 */
    c->l = IN(c->start + 0x3b6);
    if ((c->l > 0x7f) || (c->l == 0x00))
        return PW_BAD;

    /* test #4 */
    /* c->l contains the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 0x3b8 + c->j) > c->k)
            c->k = IN(c->start + 0x3b8 + c->j);
        if (IN(c->start + 0x3b8 + c->j) > 0x7f)
            return PW_BAD;
    }
    c->k += 1;

    /* test #5  notes .. gosh ! (testing all patterns !) */
    /* c->k contains the number of pattern saved */
    /* c->wss contains the whole sample size :) */
    for (c->j = 0; c->j < (256 * c->k); c->j++)
    {
        c->l = IN(c->start + 0x43c + c->j * 4);
        if ((c->l > 72) || (((c->l / 2) * 2) != c->l))
            return PW_BAD;
        c->l = IN(c->start + 0x43e + c->j * 4) & 0xf0;
        c->m = IN(c->start + 0x43d + c->j * 4);
        if ((c->l != 0) || (c->m > 0x1f))
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_Polka(pw_ctx *c)
{
    /* c->wss is the whole sample size */
    /* c->k is the highest pattern number */
    c->out_size = c->wss + (c->k * 1024) + 0x43c;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Polka Packed music", PWF_PolkaPacker);

    if (c->save_status == PW_GOOD)
        c->i += 0x43C; /* put back pointer after header */
}

/*
 *   Polka.c   2003 (c) Asle
 */
void pw_depack_Polka(pw_ctx *c)
{
    uint8_t  poss[37][2];
    uint8_t  c1 = 0x00, c2 = 0x00;
    uint8_t  max = 0x00;
    uint8_t  note;
    int32_t  whole_sample_size = 0;
    int32_t  i = 0, j;
    int32_t  where = c->start;
    pw_file *out;
    uint8_t  whatever[4];

    if (c->save_status == PW_BAD)
        return;

    pw_fill_ptk_table(poss);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* takes care of header */
    pw_fwrite_in(out, where, 20);
    for (i = 0; i < 31; i++)
    {
        pw_fwrite_in(out, where + 20 + i * 30, 18);
        c1 = 0x00;
        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c1, 1);
        pw_fwrite(out, &c1, 1);
        pw_fwrite_in(out, where + 42 + i * 30, 8);
        whole_sample_size += (((IN(where + 42 + i * 30) * 256) + IN(where + 43 + i * 30)) * 2);
    }

    /* read and write size of pattern list+ntk byte + pattern list */
    pw_fwrite_in(out, where + 0x3b6, 130);

    /* write ID */
    c1 = 'M';
    c2 = '.';
    pw_fwrite(out, &c1, 1);
    pw_fwrite(out, &c2, 1);
    c1 = 'K';
    pw_fwrite(out, &c1, 1);
    pw_fwrite(out, &c2, 1);

    /* get number of pattern */
    max = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(where + i + 0x3b8) > max)
            max = IN(where + i + 0x3b8);
    }
    max += 1;

    /* pattern data */
    where = c->start + 0x43c;
    for (i = 0; i < max; i++)
    {
        for (j = 0; j < 256; j++)
        {
            whatever[0] = IN(where + 1) & 0xf0;
            whatever[2] = (uint8_t)((IN(where + 1) & 0x0f) << 4);
            whatever[2] |= IN(where + 2);
            whatever[3] = IN(where + 3);
            /* poss[] has 37 entries; the note byte / 2 can reach 127 */
            note = (uint8_t)(IN(where) / 2);
            whatever[0] |= (note < 37) ? poss[note][0] : 0;
            whatever[1] = (note < 37) ? poss[note][1] : 0;
            pw_fwrite(out, whatever, 4);
            where += 4;
        }
    }

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap("   Polka Packer   ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
