/*
 * GPMO.c - GPMO (Crunch Player), "GPMO" at the Protracker ID position
 *          (upstream r/GPMO.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_GPMO(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1080)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1080);
    for (c->k = 0; c->k < 31; c->k++)
    {
        /* size */
        c->j = (((IN(c->start + 42 + c->k * 30) * 256) + IN(c->start + 43 + c->k * 30)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 46 + c->k * 30) * 256) + IN(c->start + 47 + c->k * 30)) * 2);
        /* loop size */
        c->n = (((IN(c->start + 48 + c->k * 30) * 256) + IN(c->start + 49 + c->k * 30)) * 2);

        if (pw_test_smps((int32_t)c->j, (int32_t)c->m, (int32_t)c->n,
                         (uint8_t)(IN(c->start + 45 + c->k * 30) / 2), IN(c->start + 44 + c->k * 30)) == PW_BAD)
            return PW_BAD;
    }

    /* test #4  pattern list size */
    c->l = IN(c->start + 950);
    if ((c->l > 127) || (c->l == 0))
        return PW_BAD;

    /* c->l holds the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 952 + c->j) > c->k)
            c->k = IN(c->start + 952 + c->j);
        if (IN(c->start + 952 + c->j) > 127)
            return PW_BAD;
    }

    /* c->k holds the highest pattern number */
    /* test last patterns of the pattern list = 0 ? */
    c->j += 2; /* found some obscure ptk :( */
    while (c->j < 128)
    {
        if (IN(c->start + 952 + c->j) > 0x7f)
            return PW_BAD;
        c->j += 1;
    }
    /* c->k is the number of pattern in the file (-1) */
    c->k += 1;

    /* test #5 pattern data ... */
    if (((c->k * 1024) + 1084 + c->start) > (uint32_t)c->in_size)
        return PW_BAD;
    for (c->j = 0; c->j < (c->k * 256); c->j++)
    {
        /* sample > 1f   or   pitch > 358 ? */
        if (IN(c->start + 1084 + c->j * 4) > 0x13)
            return PW_BAD;
        c->l = ((IN(c->start + 1084 + c->j * 4) & 0x0f) * 256) + IN(c->start + 1085 + c->j * 4);
        if ((c->l > 0) && (c->l < 0x1C))
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_GPMO(pw_ctx *c)
{
    /* c->k is still the nbr of pattern */

    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
        c->wss += (((IN(c->start + 42 + c->j * 30) * 256) + IN(c->start + 43 + c->j * 30)) * 2);

    c->out_size = c->wss + (c->k * 1024) + 1084;

    c->convert = PW_BAD;
    pw_save_rip(c, "GPMO (Crunch Player) module", PWF_GPMO);

    if (c->save_status == PW_GOOD)
        c->i += (c->out_size - 1081); /* 1080 should do but call it "just to be sure" :) */
}

/*
 *   gpmo.c   2003 (c) Asle / ReDoX
 *
 * Converts GPMO MODs back to PTK
 */
void pw_depack_GPMO(pw_ctx *c)
{
    static const uint8_t mk[4] = {'M', '.', 'K', '.'};
    uint8_t  max = 0x00;
    int32_t  whole_sample_size = 0;
    int32_t  i = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* get whole sample size and patch vols (/2) - in the input, as upstream */
    for (i = 0; i < 31; i++)
    {
        whole_sample_size += (((IN(where + 42 + i * 30) * 256) + IN(where + 43 + i * 30)) * 2);
        IN_SET(where + 45 + i * 30, IN(where + 45 + i * 30) / 2);
    }

    /* read and write whole header */
    pw_fwrite_in(out, where, 1080);

    where += 952;

    /* write ID */
    pw_fwrite(out, mk, 4);

    /* get number of pattern */
    max = 0x00;
    for (i = 0; i < 128; i++)
    {
        if (IN(where + i) > max)
            max = IN(where + i);
    }

    /* pattern data */
    where = c->start + 1084;
    pw_fwrite_in(out, where, (max + 1) * 1024);
    where += ((max + 1) * 1024);

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    pw_crap("       GPMO       ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
