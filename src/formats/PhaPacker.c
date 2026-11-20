/*
 * PhaPacker.c - PHA Packer (upstream r/PhaPacker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_PHA(pw_ctx *c)
{
    /* test #1 */
    if (c->i < 11)
        return PW_BAD;

    /* test #2 (volumes,sample addresses and whole sample size) */
    c->start = (int32_t)(c->i - 11);
    c->l = 0;
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
    {
        /* sample size */
        c->n = (((IN(c->start + c->j * 14) * 256) + IN(c->start + c->j * 14 + 1)) * 2);
        c->wss += c->n;
        /* loop start */
        c->m = (((IN(c->start + c->j * 14 + 4) * 256) + IN(c->start + c->j * 14 + 5)) * 2);

        if (IN(c->start + 3 + c->j * 14) > 0x40)
            return PW_BAD;
        if (c->m > c->wss)
            return PW_BAD;
        c->k = ((uint32_t)IN(c->start + 8 + c->j * 14) << 24) +
               ((uint32_t)IN(c->start + 9 + c->j * 14) << 16) +
               ((uint32_t)IN(c->start + 10 + c->j * 14) << 8) +
               IN(c->start + 11 + c->j * 14);
        /* c->k is the address of this sample data */
        if ((c->k < 0x3C0) || (c->k > (uint32_t)c->in_size))
            return PW_BAD;
    }
    if ((c->wss <= 2) || (c->wss > (31 * 65535)))
        return PW_BAD;

    /* test #3 (addresses of pattern in file ... possible ?) */
    /* c->wss is the WholeSampleSize */
    c->l = c->wss + 960;
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        c->m = ((uint32_t)IN(c->start + 448 + c->j * 4) << 24) +
               ((uint32_t)IN(c->start + 449 + c->j * 4) << 16) +
               ((uint32_t)IN(c->start + 450 + c->j * 4) << 8) +
               IN(c->start + 451 + c->j * 4);
        if (c->m > c->k)
            c->k = c->m;
        if ((c->m + 2) < c->l)
            return PW_BAD;
    }
    /* c->k is the highest pattern data address */

    return PW_GOOD;
}

void pw_rip_PHA(pw_ctx *c)
{
    /* c->k is still the highest pattern address ... so, 'all' we */
    /* have to do, here, is to depack the last pattern to get its */
    /* size ... that's all we need. */
    c->m = 0;

    for (c->j = 0; c->j < 256; c->j++)
    {
        /* 192 = 1100-0000 ($C0) */
        if (IN(c->start + c->k + c->m) < 192)
        {
            c->m += 4;
            continue;
        }
        else
        {
            c->l = 255 - IN(c->start + c->k + c->m + 1);
            c->m += 2;
            c->j += (c->l - 1);
        }
    }
    c->out_size = c->m + c->k;

    c->convert = PW_GOOD;
    pw_save_rip(c, "PHA Packed music", PWF_Pha_packer);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 12); /* 11 should do but call it "just to be sure" :) */
}

/*
 *   PhaPacker.c   1996-2003 (c) Asle / ReDoX
 *
 * Converts PHA packed MODs back to PTK MODs
 * nth revision :(.
 *
 * update (15 mar 2003)
 * - numerous bugs corrected ... seems to work now ... hum ?
 * update (8 dec 2003)
 * - removed fopen()
 */

/* The original read all pattern data into a heap copy and indexed it
 * without bounds; bytes past its end read as zero. */
static uint8_t pha_pat_byte(pw_ctx *c, int32_t where, int32_t size, int32_t i)
{
    return (i >= 0 && i < size) ? IN(where + i) : 0;
}

/* The original converted the patterns into a 65536 byte heap block and wrote
 * PatMax*1024 bytes of it (PatMax <= 255), reading past its end what it had
 * written there. Keep enough for that; later writes are never read back. */
#define PHA_PAT_BUF (256 * 1024)

void pw_depack_PHA(pw_ctx *c)
{
    uint8_t  c1 = 0x00, c2 = 0x00, c3 = 0x00;
    uint8_t  poss[37][2];
    uint8_t *pattern;
    uint8_t  whatever[64];
    uint8_t  old_note_value[4][4];
    uint8_t  note, smp, fx, fxval;
    uint8_t  pat_max = 0x00;
    int32_t  my_pat_list[128];
    int32_t  pats_address[128];
    int32_t  i = 0, j = 0, k = 0;
    int32_t  start_pat_address = 9999999l;
    int32_t  whole_pattern_data_size;
    int32_t  whole_sample_size = 0;
    int32_t  sample_data_address;
    int32_t  where = c->start;
    int32_t  pat_where;
    int32_t  n, v;
    int16_t  old_cpt[4];
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    pw_fill_ptk_table(poss);

    memset(pats_address, 0, sizeof pats_address);
    memset(old_note_value, 0, sizeof old_note_value);
    memset(old_cpt, 0, sizeof old_cpt);
    memset(my_pat_list, 0, sizeof my_pat_list);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    for (i = 0; i < 20; i++) /* title */
        pw_fwrite(out, &c1, 1);

    for (i = 0; i < 31; i++)
    {
        memset(whatever, 0, sizeof whatever);

        /* sample name */
        pw_fwrite(out, &whatever[32], 22);

        /* size */
        pw_fwrite_in(out, where, 2);
        whole_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);

        /* finetune */
        c1 = (uint8_t)(((IN(where + 12) * 256) + IN(where + 13)) / 0x48);
        pw_fwrite(out, &c1, 1);

        /* volume */
        pw_fwrite_in(out, where + 3, 1);

        /* loop start */
        pw_fwrite_in(out, where + 4, 2);

        /* loop size */
        pw_fwrite_in(out, where + 6, 2);
        where += 14;
    }

    /* bypass those unknown 14 bytes */
    where += 14;

    for (i = 0; i < 128; i++)
    {
        pats_address[i] = (int32_t)(((uint32_t)IN(where) << 24) + ((uint32_t)IN(where + 1) << 16) +
                                    ((uint32_t)IN(where + 2) << 8) + IN(where + 3));
        where += 4;
        if (pats_address[i] < start_pat_address)
            start_pat_address = pats_address[i];
    }

    sample_data_address = where;

    /* pattern datas */
    /* read ALL pattern data */
    whole_pattern_data_size = (int32_t)(c->out_size - (uint32_t)start_pat_address);
    pat_where = start_pat_address + c->start;
    pattern = (uint8_t *)calloc(PHA_PAT_BUF, 1);
    if (pattern == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        pw_fdiscard(out);
        return;
    }

#define WPD(x) pha_pat_byte(c, pat_where, whole_pattern_data_size, (x))
#define PAT_SET(x, v)                                  \
    do                                                 \
    {                                                  \
        int32_t px_ = (x);                             \
        if (px_ >= 0 && px_ < PHA_PAT_BUF)             \
            pattern[px_] = (uint8_t)(v);               \
    } while (0)
#define PAT_GET(x) (((x) >= 0 && (x) < PHA_PAT_BUF) ? pattern[(x)] : 0)

    j = 0;
    k = 0;
    c1 = 0x00;
    for (i = 0; i < whole_pattern_data_size; i++)
    {
        if ((k % 256) == 0)
        {
            /* MyPatList[] has 128 entries; c1 counts up to 255 */
            if (c1 < 128)
                my_pat_list[c1] = start_pat_address + i;
            c1 += 0x01;
        }
        if (WPD(i) == 0xff)
        {
            i += 1;
            /* for k == 0 the original wrote Old_cpt[-1] (stack); dropped */
            n = (k - 1) % 4;
            if ((n >= 0) && (n < 4))
                old_cpt[n] = (int16_t)(0xff - WPD(i));
            continue;
        }
        /* every index into the 4-entry voice arrays is checked on both ends
         * (k only wraps negative after 2^31 notes, but crafted data decides) */
        v = k % 4;
        if ((v >= 0) && (old_cpt[v] != 0))
        {
            smp = old_note_value[v][0];
            note = old_note_value[v][1];
            fx = old_note_value[v][2];
            fxval = old_note_value[v][3];
            old_cpt[v] -= 1;

            /* poss[] has 37 entries; note / 2 can reach 127 */
            PAT_SET(j, smp & 0xf0);
            PAT_SET(j, PAT_GET(j) | ((note / 2 < 37) ? poss[note / 2][0] : 0));
            PAT_SET(j + 1, (note / 2 < 37) ? poss[note / 2][1] : 0);
            PAT_SET(j + 2, (smp << 4) & 0xf0);
            PAT_SET(j + 2, PAT_GET(j + 2) | fx);
            PAT_SET(j + 3, fxval);
            k += 1;
            j += 4;
            i -= 1;
            continue;
        }
        smp = WPD(i);
        note = WPD(i + 1);
        fx = WPD(i + 2);
        fxval = WPD(i + 3);
        if (v >= 0)
        {
            old_note_value[v][0] = smp;
            old_note_value[v][1] = note;
            old_note_value[v][2] = fx;
            old_note_value[v][3] = fxval;
        }
        i += 3;
        PAT_SET(j, smp & 0xf0);
        PAT_SET(j, PAT_GET(j) | ((note / 2 < 37) ? poss[note / 2][0] : 0));
        PAT_SET(j + 1, (note / 2 < 37) ? poss[note / 2][1] : 0);
        PAT_SET(j + 2, (smp << 4) & 0xf0);
        PAT_SET(j + 2, PAT_GET(j + 2) | fx);
        PAT_SET(j + 3, fxval);
        k += 1;
        j += 4;
    }
    pat_max = c1;

    /* try to get the number of pattern in pattern list */
    /* The original starts at Pats_Address[128], one past the array. In the
     * x86-64 gcc reference build that slot is MyPatList[0] (the next local
     * on the stack), which normally holds the lowest pattern address. Real
     * modules only convert correctly with that value, so it is kept. */
    for (c1 = 128; c1 > 0x00; c1 -= 0x01)
        if (((c1 < 128) ? pats_address[c1] : my_pat_list[0]) != pats_address[127])
            break;

    /* write this value */
    c1 += 1;
    pw_fwrite(out, &c1, 1);

    /* ntk restart byte */
    c2 = 0x7f;
    pw_fwrite(out, &c2, 1);

    /* write pattern list */
    for (i = 0; i < 128; i++)
    {
        /* MyPatList[] has 128 entries but c1 runs to 255 and wraps: the
         * original read past the array and could loop forever. Entries past
         * the end read as 0 and the search stops after one full turn. */
        for (n = 0, c1 = 0x00; n < 256; n++, c1 += 0x01)
            if (pats_address[i] == ((c1 < 128) ? my_pat_list[c1] : 0))
                break;
        pw_fwrite(out, &c1, 1);
    }

    /* ID string */
    c1 = 'M';
    c2 = '.';
    c3 = 'K';
    pw_fwrite(out, &c1, 1);
    pw_fwrite(out, &c2, 1);
    pw_fwrite(out, &c3, 1);
    pw_fwrite(out, &c2, 1);

    pw_fwrite(out, pattern, pat_max * 1024);
    free(pattern);

#undef WPD
#undef PAT_SET
#undef PAT_GET

    /* Sample data */
    pw_fwrite_in(out, sample_data_address, whole_sample_size);

    pw_crap("    PhaPacker     ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
