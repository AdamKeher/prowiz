/*
 * okta.c - Oktalyzer module, "OKTASONGCMOD" (upstream r/okta.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_Okta(pw_ctx *c)
{
    /* test 1 */
    c->start = (int32_t)c->i;
    c->m = (uint32_t)c->start;
    c->m += 12; /* after OKTASONGCMOD */

    /* CMOD chunk size */
    c->j = (IN(c->m) * 256 * 256 * 256) +
           (IN(c->m + 1) * 256 * 256) +
           (IN(c->m + 2) * 256) +
           IN(c->m + 3);
    if ((c->j > 8) || (c->j < 4)) /* never seen it being different than 8 */
        return PW_BAD;

    c->m += c->j + 4;
    /* should now be on SAMP */
    if ((IN(c->m) != 'S') ||
        (IN(c->m + 1) != 'A') ||
        (IN(c->m + 2) != 'M') ||
        (IN(c->m + 3) != 'P'))
        return PW_BAD;

    c->m += 4;
    /* SAMP chunk size */
    c->j = (IN(c->m) * 256 * 256 * 256) +
           (IN(c->m + 1) * 256 * 256) +
           (IN(c->m + 2) * 256) +
           IN(c->m + 3);
    if (c->j != 0x480) /* size fixed as specified */
        return PW_BAD;
    c->m += 4;

    /* c->m is the first sample now */
    return PW_GOOD;
}

void pw_rip_Okta(pw_ctx *c)
{
    int32_t where = (int32_t)c->m;

    /* get the number of samples */
    c->l = 0;
    for (c->k = 0; c->k < 36; c->k++)
    {
        c->m = (IN(where + 20 + c->k * 32) * 256 * 256 * 256) +
               (IN(where + 21 + c->k * 32) * 256 * 256) +
               (IN(where + 22 + c->k * 32) * 256) +
               IN(where + 23 + c->k * 32);
        if (c->m != 0)
            c->l += 1; /* so c->l holds the nbr of samples (i.e. SBOD)*/
    }
    /* bypass SAMP */
    where += 0x480;

    /* bypass SPEE entire chunk, which total size should be 4 + 4 + 2 */
    where += +10;

    /* bypass SLEN word and its size which is always 2 */
    where += 8;

    /* get song length (i.e. number of PBOD) */
    c->m = (IN(where) * 256) + IN(where + 1);
    where += 2;

    /* bypass PLEN entire chunk which size is always 4 + 4 + 2 */
    where += 10;

    /* bypass PATT entire chunk which size is always 128 + 4 + 4 */
    where += 136;

    /* now, lets calculate the size of patterns (PBODs) */
    for (c->k = 0; c->k < c->m; c->k++)
    {
        where += 4; /* bypass 'PBOD' */
        c->j = (IN(where) * 256 * 256 * 256) +
               (IN(where + 1) * 256 * 256) +
               (IN(where + 2) * 256) +
               IN(where + 3);
        where += 4; /* bypass the dword just read */
        where += c->j;
        if (where > c->in_size)
            break;
    }
    /* where is now on the first SBOD */

    /* now, lets calculate the size of samples (SBODs) */
    for (c->k = 0; c->k < c->l; c->k++)
    {
        where += 4; /* bypass 'SBOD' */
        c->j = (IN(where) * 256 * 256 * 256) +
               (IN(where + 1) * 256 * 256) +
               (IN(where + 2) * 256) +
               IN(where + 3);
        where += 4; /* bypass the dword just read */
        where += c->j;
        if (where > c->in_size)
        {
            c->o = PW_BAD;
            break;
        }
    }
    /* We're is now at the end of the OKTA file */

    /* c->o is only set above: otherwise it is whatever an earlier handler left */
    if (c->o != PW_BAD)
    {
        c->out_size = (uint32_t)(where - c->start);

        c->convert = PW_BAD;
        pw_save_rip(c, "Oktalizer module", PWF_Oktalizer);

        if (c->save_status == PW_GOOD)
            c->i += 1;
    }
    else
    {
        pw_printf(c, "found a too much truncated Oktalizer file\n");
    }
}
