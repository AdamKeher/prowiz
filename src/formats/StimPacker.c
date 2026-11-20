/*
 * StimPacker.c - STIM (Slamtilt) packer (upstream r/StimPacker.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 *
 * update on the 3rd of april 2000
 * bug pointed out by Thomas Neumann ... thx
 */
#include "formats.h"

int16_t pw_test_STIM(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    c->j = (((uint32_t)IN(c->start + 4) << 24) +
            ((uint32_t)IN(c->start + 5) << 16) +
            ((uint32_t)IN(c->start + 6) << 8) +
            IN(c->start + 7));
    if (c->j < 406)
        return PW_BAD;

    /* size of the pattern list */
    c->k = ((IN(c->start + 18) * 256) + IN(c->start + 19));
    if (c->k > 128)
        return PW_BAD;

    /* nbr of pattern saved */
    c->k = ((IN(c->start + 20) * 256) + IN(c->start + 21));
    if ((c->k > 64) || (c->k == 0))
        return PW_BAD;

    /* pattern list */
    for (c->l = 0; c->l < 128; c->l++)
    {
        if (IN(c->start + 22 + c->l) > c->k)
            return PW_BAD;
    }

    /* test sample sizes */
    c->wss = 0;
    for (c->l = 0; c->l < 31; c->l++)
    {
        /* addresse de la table */
        c->o = c->start + c->j + c->l * 4;

        /* address du sample */
        c->k = (((uint32_t)IN(c->o) << 24) +
                ((uint32_t)IN(c->o + 1) << 16) +
                ((uint32_t)IN(c->o + 2) << 8) +
                IN(c->o + 3));

        /* taille du smp */
        c->m = ((IN(c->o + c->k - c->l * 4) * 256) +
                IN(c->o + c->k + 1 - c->l * 4)) * 2;

        c->wss += c->m;
    }

    if (c->wss <= 4)
        return PW_BAD;

    /* c->wss is the size of the sample data */
    /* c->j is the address of the sample desc */
    return PW_GOOD;
}

void pw_rip_STIM(pw_ctx *c)
{
    c->out_size = c->wss + c->j + 31 * 4 + 31 * 8;

    c->convert = PW_GOOD;
    pw_save_rip(c, "STIM (Slamtilt) module", PWF_STIM);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1); /* 0 should do but call it "just to be sure" :) */
}

/*
 *   STIM_Packer.c   1998 (c) Sylvain "Asle" Chipaux
 *
 * STIM Packer to Protracker.
 ********************************************************
 * 13 april 1999 : Update
 *   - no more open() of input file ... so no more fread() !.
 *     It speeds-up the process quite a bit :).
 * 28 Nov 1999 : Update
 *   - Speed & Size optimizings
 */
void pw_depack_STIM(pw_ctx *c)
{
    uint8_t  whatever[1024];
    uint8_t  c1 = 0x00, c2 = 0x00, c3 = 0x00, c4 = 0x00;
    uint8_t  poss[37][2];
    uint8_t  max = 0x00;
    uint8_t  note, smp, fx, fxval;
    int16_t  tracks_add[4];
    int32_t  i = 0, j = 0, k = 0;
    int32_t  smp_desc_add = 0;
    int32_t  pat_adds[64];
    int32_t  smp_data_adds[31];
    int32_t  smp_sizes[31];
    int32_t  where = c->start; /* main pointer to prevent fread() */
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    /* (the original declared poss[36][2] and filled 37 entries) */
    pw_fill_ptk_table(poss);

    memset(pat_adds, 0, sizeof pat_adds);
    memset(smp_data_adds, 0, sizeof smp_data_adds);
    memset(smp_sizes, 0, sizeof smp_sizes);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* write title */
    memset(whatever, 0, sizeof whatever);
    pw_fwrite(out, whatever, 20);

    /* bypass ID */
    where += 4;

    /* read $ of sample description */
    smp_desc_add = (int32_t)(((uint32_t)IN(where) << 24) +
                             ((uint32_t)IN(where + 1) << 16) +
                             ((uint32_t)IN(where + 2) << 8) +
                             IN(where + 3));
    /* "where" isn't "+=4" coz it's assigned below */

    /* convert and write header */
    for (i = 0; i < 31; i++)
    {
        where = c->start + smp_desc_add + i * 4;
        smp_data_adds[i] = (int32_t)(((uint32_t)IN(where) << 24) +
                                     ((uint32_t)IN(where + 1) << 16) +
                                     ((uint32_t)IN(where + 2) << 8) +
                                     IN(where + 3));
        smp_data_adds[i] = (int32_t)((uint32_t)smp_data_adds[i] + (uint32_t)smp_desc_add);
        where = c->start + smp_data_adds[i];
        smp_data_adds[i] = (int32_t)((uint32_t)smp_data_adds[i] + 8u);

        /* write sample name */
        pw_fwrite(out, whatever, 22);

        /* sample size */
        smp_sizes[i] = (((IN(where) * 256) + IN(where + 1)) * 2);
        /* size,fine,vol,loops */
        pw_fwrite_in(out, where, 8);

        /* no "where += 8" coz it's reassigned inside and after loop */
    }

    /* size of the pattern list */
    where = c->start + 19;
    pw_fwrite_in(out, where++, 1);
    whatever[0] = 0x7f;
    pw_fwrite(out, whatever, 1);

    /* pattern table */
    where += 1;
    max = IN(where++);
    pw_fwrite_in(out, where, 128);
    where += 128;

    /* write Protracker's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* read pattern addresses */
    for (i = 0; i < 64; i++)
    {
        pat_adds[i] = (int32_t)(((uint32_t)IN(where) << 24) +
                                ((uint32_t)IN(where + 1) << 16) +
                                ((uint32_t)IN(where + 2) << 8) +
                                IN(where + 3));
        pat_adds[i] = (int32_t)((uint32_t)pat_adds[i] + 0x0cu);
        where += 4;
    }

    /* pattern data */
    for (i = 0; i < max; i++)
    {
        /* PatAdds[] has 64 entries; max is a byte from the file */
        int32_t pat_add = (i < 64) ? pat_adds[i] : 0;

        where = c->start + pat_add;
        for (k = 0; k < 4; k++)
        {
            tracks_add[k] = (int16_t)((IN(where) * 256) + IN(where + 1));
            where += 2;
        }

        memset(whatever, 0, sizeof whatever);
        for (k = 0; k < 4; k++)
        {
            where = c->start + pat_add + tracks_add[k];
            for (j = 0; j < 64; j++)
            {
                c1 = IN(where++);
                if ((c1 & 0x80) == 0x80)
                {
                    j += (c1 & 0x7F);
                    continue;
                }
                c2 = IN(where++);
                c3 = IN(where++);

                smp = c1 & 0x1F;
                note = c2 & 0x3F;
                fx = ((c1 >> 5) & 0x03);
                c4 = ((c2 >> 4) & 0x0C);
                fx |= c4;
                fxval = c3;

                whatever[j * 16 + k * 4] = (smp & 0xf0);

                if (note != 0)
                {
                    /* poss[] had 36 entries in the original; note-1 can
                     * reach 62 */
                    whatever[j * 16 + k * 4] |= (note - 1 < 36) ? poss[note - 1][0] : 0;
                    whatever[j * 16 + k * 4 + 1] = (note - 1 < 36) ? poss[note - 1][1] : 0;
                }

                whatever[j * 16 + k * 4 + 2] = (uint8_t)((smp << 4) & 0xf0);
                whatever[j * 16 + k * 4 + 2] |= fx;
                whatever[j * 16 + k * 4 + 3] = fxval;
            }
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data */
    for (i = 0; i < 31; i++)
    {
        where = c->start + smp_data_adds[i];
        pw_fwrite_in(out, where, smp_sizes[i]);
    }

    pw_crap(" STIM (Slamtilt)  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
