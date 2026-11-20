/*
 * ModuleProtector.c - Module Protector, with or without the "TRK1" ID
 *                     (upstream r/ModuleProtector.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/*  (May 2002)
 *    added test_smps()
 */
int16_t pw_test_MP_noID(pw_ctx *c)
{
    /* TODO : test notes */

    /* test #1 */
    if ((c->i < 3) || ((c->i + 375) > (uint32_t)c->in_size))
        return PW_BAD;

    /* test #2 (c->wss is not reset: it adds up from whatever the previous
     * handler left there, as in the original) */
    c->start = (int32_t)(c->i - 3);
    c->l = 0;
    for (c->j = 0; c->j < 31; c->j++)
    {
        /* size */
        c->k = (((IN(c->start + 8 * c->j) * 256) + IN(c->start + 1 + 8 * c->j)) * 2);
        /* loop start */
        c->m = (((IN(c->start + 4 + 8 * c->j) * 256) + IN(c->start + 5 + 8 * c->j)) * 2);
        /* loop size */
        c->n = (((IN(c->start + 6 + 8 * c->j) * 256) + IN(c->start + 7 + 8 * c->j)) * 2);
        c->wss += c->k;

        if (pw_test_smps((int32_t)c->k, (int32_t)c->m, (int32_t)c->n,
                         IN(c->start + 3 + 8 * c->j), IN(c->start + 2 + 8 * c->j)) == PW_BAD)
            return PW_BAD;
    }
    if (c->wss <= 2)
        return PW_BAD;

    /* test #3 */
    c->l = IN(c->start + 248);
    if ((c->l > 0x7f) || (c->l == 0x00))
        return PW_BAD;

    /* test #4 */
    /* c->l contains the size of the pattern list */
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 250 + c->j) > c->k)
            c->k = IN(c->start + 250 + c->j);
        if (IN(c->start + 250 + c->j) > 0x7f)
            return PW_BAD;
        if ((c->j > c->l + 10) && (IN(c->start + 250 + c->j) != 0x00))
            return PW_BAD;
    }
    c->k += 1;

    /* test #5  ptk notes .. gosh ! (testing all patterns !) */
    /* c->k contains the number of pattern saved */
    /* c->wss is the whole sample size */
    /* (c->o is not reset either: only set to 1 below) */
    c->m = 0;
    if (c->start + 379 + ((c->k * 256) * 4) > (uint32_t)c->in_size)
        return PW_BAD;
    for (c->j = 0; c->j < ((256 * c->k) - 4); c->j++)
    {
        c->l = IN(c->start + 378 + c->j * 4 + 4);
        if ((c->l & 0xf0) > 0x10)
            return PW_BAD;
        if ((c->l & 0x0f) > 0x03)
            return PW_BAD;
        c->l = IN(c->start + 378 + c->j * 4) & 0x0f;
        c->l *= 256;
        c->l += IN(c->start + 379 + c->j * 4);
        c->n = IN(c->start + 380 + c->j * 4) >> 4;
        if (c->l != 0)
            c->m = 1;
        if (c->n != 0)
            c->o = 1;
        /*20100905 : more accurate test to lower fake finding */
        if ((c->l < 0x100) && (c->l != 0x00) && (c->l != 0x71) && (c->l != 0x78) && (c->l != 0x7f) &&
            (c->l != 0x87) && (c->l != 0x8f) && (c->l != 0x97) && (c->l != 0xa0) && (c->l != 0xaa) &&
            (c->l != 0xb4) && (c->l != 0xbe) && (c->l != 0xca) && (c->l != 0xd6) && (c->l != 0xe2) &&
            (c->l != 0xf0) && (c->l != 0xfe))
            return PW_BAD;
    }
    if ((c->m == 0) || (c->o == 0))
    {
        /* no note ... odd */
        return PW_BAD;
    }

    /* test #6  (loopStart+LoopSize > Sample ? ) */
    for (c->j = 0; c->j < 31; c->j++)
    {
        c->k = (((IN(c->start + c->j * 8) * 256) + IN(c->start + 1 + c->j * 8)) * 2);
        c->l = (((IN(c->start + 4 + c->j * 8) * 256) + IN(c->start + 5 + c->j * 8)) * 2)
             + (((IN(c->start + 6 + c->j * 8) * 256) + IN(c->start + 7 + c->j * 8)) * 2);
        if (c->l > (c->k + 2))
            return PW_BAD;
    }

    return PW_GOOD;
}

int16_t pw_test_MP_withID(pw_ctx *c)
{
    /* test #1 */
    c->start = (int32_t)c->i;
    for (c->j = 0; c->j < 31; c->j++)
    {
        if (IN(c->start + 6 + 8 * c->j) > 0x0f)
            return PW_BAD;
    }

    /* test #2 */
    c->l = IN(c->start + 252);
    if ((c->l > 0x7f) || (c->l == 0x00))
        return PW_BAD;

    /* test #4 */
    c->k = 0;
    for (c->j = 0; c->j < 128; c->j++)
    {
        if (IN(c->start + 254 + c->j) > c->k)
            c->k = IN(c->start + 254 + c->j);
        if (IN(c->start + 254 + c->j) > 0x7f)
            return PW_BAD;
    }
    c->k += 1;

    /* test #5  ptk notes .. gosh ! (testing all patterns !) */
    /* c->k contains the number of pattern saved */
    for (c->j = 0; c->j < (256 * c->k); c->j++)
    {
        c->l = IN(c->start + 382 + c->j * 4);
        if (c->l > 19) /* 0x13 */
            return PW_BAD;
    }

    return PW_GOOD;
}

void pw_rip_MP_noID(pw_ctx *c)
{
    /* c->wss is the whole sample size */

    c->j = IN(c->start + 248);
    c->l = 0;
    for (c->k = 0; c->k < c->j; c->k++)
        if (IN(c->start + 250 + c->k) > c->l)
            c->l = IN(c->start + 250 + c->k);

    c->k = (IN(c->start + 378) * 256 * 256 * 256) +
           (IN(c->start + 379) * 256 * 256) +
           (IN(c->start + 380) * 256) +
           IN(c->start + 381);

    c->l += 1;
    c->out_size = c->wss + (c->l * 1024) + 378;
    if (c->k == 0)
        c->out_size += 4;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Module Protector Packed music", PWF_Module_protector);

    if (c->save_status == PW_GOOD)
        c->i += 0x57E;
}

void pw_rip_MP_withID(pw_ctx *c)
{
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
        c->wss += (((IN(c->start + 4 + c->j * 8) * 256) + IN(c->start + 5 + c->j * 8)) * 2);
    c->j = IN(c->start + 252);
    c->l = 0;
    for (c->k = 0; c->k < c->j; c->k++)
        if (IN(c->start + 254 + c->k) > c->l)
            c->l = IN(c->start + 254 + c->k);

    c->k = (IN(c->start + 382) * 256 * 256 * 256) +
           (IN(c->start + 383) * 256 * 256) +
           (IN(c->start + 384) * 256) +
           IN(c->start + 385);

    c->l += 1;
    c->out_size = c->wss + (c->l * 1024) + 382;

    /* not sure for the following test because I've never found */
    /* any MP file with "TRK1" ID. I'm basing all this on Gryzor's */
    /* statement in his Hex-dump exemple ... */
    if (c->k == 0)
        c->out_size += 4;

    c->convert = PW_GOOD;
    pw_save_rip(c, "Module Protector Packed music", PWF_Module_protector);

    if (c->save_status == PW_GOOD)
        c->i += 3; /* 4 should do but call it "just to be sure" :) */
}

/*
 *   Module_Protector.c   1997 (c) Asle / ReDoX
 *
 * Converts MP packed MODs back to PTK MODs
 * thanks to Gryzor and his ProWizard tool ! ... without it, this prog
 * would not exist !!!
 *
 *  NOTE : It takes care of both MP packed files with or without ID !
 *
 * 20100119 : clean up - only one fwrite for header
 */
void pw_depack_MP(pw_ctx *c)
{
    uint8_t  whatever[1085];
    int32_t  i = 0, j = 0;
    int32_t  whole_sample_size = 0;
    int32_t  where = c->start;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(whatever, 0, sizeof whatever);

    /* empty title */

    if ((IN(where) == 'T') && (IN(where + 1) == 'R') && (IN(where + 2) == 'K') && (IN(where + 3) == '1'))
        where += 4;

    for (i = 0; i < 31; i++)
    {
        whole_sample_size += (((IN(where) * 256) + IN(where + 1)) * 2);
        whatever[i * 30 + 42] = IN(where);
        whatever[i * 30 + 43] = IN(where + 1);
        whatever[i * 30 + 44] = IN(where + 2);
        whatever[i * 30 + 45] = IN(where + 3);
        whatever[i * 30 + 46] = IN(where + 4);
        whatever[i * 30 + 47] = IN(where + 5);
        whatever[i * 30 + 48] = IN(where + 6);
        whatever[i * 30 + 49] = IN(where + 7);
        where += 8;
    }

    /* pattern table lenght & Ntk byte */
    whatever[950] = IN(where++);
    whatever[951] = IN(where++);

    j = 0;
    for (i = 0; i < 128; i++)
    {
        whatever[952 + i] = IN(where + i);
        if (IN(where + i) > j)
            j = IN(where + i);
    }
    where += 128;

    whatever[1080] = 'M';
    whatever[1081] = '.';
    whatever[1082] = 'K';
    whatever[1083] = '.';

    pw_fwrite(out, whatever, 1084);

    /* bypass 4 unknown empty bytes */
    if ((IN(where) == 0x00) && (IN(where + 1) == 0x00) && (IN(where + 2) == 0x00) && (IN(where + 3) == 0x00))
        where += 4;

    /* pattern data */
    i = (j + 1) * 1024;
    pw_fwrite_in(out, where, i);
    where += i;

    /* sample data */
    pw_fwrite_in(out, where, whole_sample_size);

    pw_crap(" Module Protector ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
