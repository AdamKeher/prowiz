/*
 * TrackerPacker1.c - Tracker Packer v1 (upstream r/TrackerPacker1.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_TP1(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    /* size of the module */
    c->wss = ((IN(c->start + 4) * 256 * 256 * 256) +
              (IN(c->start + 5) * 256 * 256) +
              (IN(c->start + 6) * 256) +
              IN(c->start + 7));
    if ((c->wss < 794) || (c->wss > 2129178l))
        return PW_BAD;

    /* test finetunes */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if (IN(c->start + 32 + c->k * 8) > 0x0f)
            return PW_BAD;
    }

    /* test volumes */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if (IN(c->start + 33 + c->k * 8) > 0x40)
            return PW_BAD;
    }

    /* sample data address */
    c->l = ((IN(c->start + 28) * 256 * 256 * 256) +
            (IN(c->start + 29) * 256 * 256) +
            (IN(c->start + 30) * 256) +
            IN(c->start + 31));
    if ((c->l == 0) || (c->l > c->wss))
        return PW_BAD;

    /* test sample sizes */
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->j = (IN(c->start + c->k * 8 + 34) * 256) + IN(c->start + c->k * 8 + 35);
        c->m = (IN(c->start + c->k * 8 + 36) * 256) + IN(c->start + c->k * 8 + 37);
        c->n = (IN(c->start + c->k * 8 + 38) * 256) + IN(c->start + c->k * 8 + 39);
        c->j *= 2;
        c->m *= 2;
        c->n *= 2;
        if ((c->j > 0xFFFF) || (c->m > 0xFFFF) || (c->n > 0xFFFF))
            return PW_BAD;
        if ((c->m + c->n) > (c->j + 2))
            return PW_BAD;
        if ((c->m != 0) && (c->n <= 2))
            return PW_BAD;
    }

    /* pattern list size */
    c->l = IN(c->start + 281);
    if ((c->l == 0) || (c->l > 128))
        return PW_BAD;

    /* c->wss is the size of the module :) */
    return PW_GOOD;
}

void pw_rip_TP1(pw_ctx *c)
{
    /* c->wss is the size of the module :) */
    c->out_size = c->wss;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Tracker Packer v1 module", PWF_TP1);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1); /* 0 could be enough */
}

/*
 *   TrackerPacker_v1.c   1998 (c) Asle / ReDoX
 * Converts TP1 packed MODs back to PTK MODs
 * thanks to Gryzor and his ProWizard tool ! ... without it, this prog
 * would not exist !!!
 * Update : 1 may 2003
 * - changed way to locate pattern datas. Correct pattern list saved now.
 */
void pw_depack_TP1(pw_ctx *c)
{
    uint8_t  c1, c2, c3;
    uint8_t  poss[37][2];
    uint8_t *whatever;
    size_t   whatever_size;
    int64_t  span;
    uint8_t  note, smp, fx, fxval;
    uint8_t  patternlist[128];
    uint8_t  pat_pos;
    /* pat_pos reaches 129; the original's table had 128 entries and entry
     * 128 landed in unused stack space */
    int32_t  pats_address[129];
    int32_t  i = 0, j = 0, k;
    int32_t  pats_address_read[128];
    int32_t  start_pat_address;
    int32_t  whole_sample_size = 0;
    int32_t  sample_data_address;
    int32_t  where = c->start;
    pw_file *out;

    pw_fill_ptk_table(poss);

    if (c->save_status == PW_BAD)
        return;

    memset(pats_address, 0, sizeof pats_address);
    memset(pats_address_read, 0, sizeof pats_address_read);

    /* sample data address (read ahead: it sizes the pattern buffer) */
    sample_data_address = (IN(where + 28) * 256 * 256 * 256) +
                          (IN(where + 29) * 256 * 256) +
                          (IN(where + 30) * 256) +
                          IN(where + 31);

    /* The original used a 64 KiB buffer and wrote 4 bytes into it for each
     * byte of pattern data, running past its end on large inputs. Make it
     * big enough for every write (at most 4 per byte of the data scanned). */
    span = (int64_t)sample_data_address - 792;
    whatever_size = 65536;
    if (span > 0 && (size_t)(span * 4 + 4) > whatever_size)
        whatever_size = (size_t)(span * 4 + 4);
    whatever = (uint8_t *)calloc(whatever_size, 1);
    if (whatever == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        return;
    }

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* title */
    pw_fwrite_in(out, where + 8, 20);

    /* setting the first pattern address as the whole file size */
    start_pat_address = 0xFFFFFF;

    where += 28;
    /* sample data address (read above) */
    where += 4;

    for (i = 0; i < 31; i++)
    {
        /* sample name */
        pw_fwrite(out, whatever, 22);

        /* size */
        whole_sample_size += (((IN(where + 2) * 256) + IN(where + 3)) * 2);
        pw_fwrite_in(out, where + 2, 2);

        /* write finetune,vol */
        pw_fwrite_in(out, where, 2);

        /* loops */
        pw_fwrite_in(out, where + 4, 4);

        where += 8;
    }

    /* read size of pattern table */
    where = c->start + 281;
    pat_pos = (uint8_t)(IN(where) + 0x01);
    pw_fwrite(out, &pat_pos, 1);
    where += 1;

    /* ntk byte */
    c1 = 0x7f;
    pw_fwrite(out, &c1, 1);

    /* the test limits IN(start + 281) to 1..128, so pat_pos <= 129 */
    for (i = 0; i < pat_pos; i++)
    {
        pats_address[i] = (IN(where) * 256 * 256 * 256) + (IN(where + 1) * 256 * 256) + (IN(where + 2) * 256) + IN(where + 3);
        where += 4;
        if (start_pat_address > pats_address[i])
            start_pat_address = pats_address[i];
    }

    /* setting real addresses */
    for (i = 0; i < pat_pos; i++)
        pats_address[i] -= (start_pat_address - 794);

    /* pattern datas */
    j = 0;
    k = 0;
    for (i = (c->start + 794); i <= (sample_data_address + c->start + 1); i += 1, j += 4)
    {
        if ((j % 1024) == 0)
        {
            /* more than 128 patterns of data ran past the original's table */
            if (k >= 0 && k < 128)
                pats_address_read[k] = i - c->start;
            k++;
        }
        c1 = IN(i);
        if (c1 == 0xC0)
            continue;

        if ((c1 & 0xC0) == 0x80)
        {
            c2 = IN(i + 1);
            fx = (uint8_t)((c1 >> 2) & 0x0f);
            fxval = c2;
            if ((uint32_t)j + 3 < whatever_size)
            {
                whatever[j + 2] = fx;
                whatever[j + 3] = fxval;
            }
            i += 1;
            continue;
        }
        c2 = IN(i + 1);
        c3 = IN(i + 2);

        smp = (uint8_t)(((c2 >> 4) & 0x0f) | ((c1 << 4) & 0x10));
        note = (uint8_t)(c1 & 0xFE);
        fx = (uint8_t)(c2 & 0x0F);
        fxval = c3;

        if ((uint32_t)j + 3 < whatever_size)
        {
            whatever[j] = (uint8_t)(smp & 0xf0);
            /* poss[] has 37 entries; note/2 can reach 127 on odd input */
            whatever[j] |= ((note / 2) < 37) ? poss[(note / 2)][0] : 0;
            whatever[j + 1] = ((note / 2) < 37) ? poss[(note / 2)][1] : 0;
            whatever[j + 2] = (uint8_t)((smp << 4) & 0xf0);
            whatever[j + 2] |= fx;
            whatever[j + 3] = fxval;
        }
        i += 2;
    }
    k -= 1;
    /* k is -1 when the loop did not run (the original wrote before its table) */
    if (k >= 0 && k < 128)
        pats_address_read[k] = 0;
    memset(patternlist, 0, sizeof patternlist);
    for (i = 0; i < pat_pos; i++)
        for (j = 0; j < k; j++)
            if (pats_address[i] == ((j < 128) ? pats_address_read[j] : 0))
            {
                /* pat_pos can be 129: the original wrote one past patternlist */
                if (i < 128)
                    patternlist[i] = (uint8_t)j;
            }

    /* write pattern list */
    pw_fwrite(out, patternlist, 128);

    /* ID string */
    patternlist[0] = 'M';
    patternlist[1] = '.';
    patternlist[2] = 'K';
    patternlist[3] = '.';
    pw_fwrite(out, patternlist, 4);

    /* pattern data */
    if (k >= 0)
        pw_fwrite(out, whatever, (int64_t)1024 * k);
    else
    {
        /* k == -1 (the loop did not run): the original passed -1024 to
         * fwrite as a huge size_t. The reference C library filled the rest
         * of its 4 KiB stream buffer from Whatever (all zero here), then
         * the write failed and the file carried on from there. */
        pw_fwrite(out, whatever, 4096 - (pw_ftell(out) % 4096));
    }
    free(whatever);

    /* Sample data */
    where = c->start + sample_data_address;
    pw_fwrite_in(out, where, whole_sample_size);

    pw_crap(" Tracker Packer 1 ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
