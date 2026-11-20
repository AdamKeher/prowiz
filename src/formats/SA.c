/*
 * SA.c - Sonic Arranger module, with ("SOARV1.0") and without hunks
 *        (upstream r/SA.c).
 *
 * Sonic Arranger ("shot in the dark" try)
 * 20160313- Asle
 * 20160314: no hunk version
 *
 * The tests print a lot of progress text; it is part of the output of the
 * original and is kept as is.
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* the 4 byte tag at c->start + c->n */
static int sa_tag(pw_ctx *c, const char *tag)
{
    return (IN(c->start + c->n) == (uint8_t)tag[0]) &&
           (IN(c->start + c->n + 1) == (uint8_t)tag[1]) &&
           (IN(c->start + c->n + 2) == (uint8_t)tag[2]) &&
           (IN(c->start + c->n + 3) == (uint8_t)tag[3]);
}

int16_t pw_test_SAhunk(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 8) != 'S') ||
        (IN(c->start + 9) != 'T') ||
        (IN(c->start + 10) != 'B') ||
        (IN(c->start + 11) != 'L'))
    {
        pw_printf(c, "#1 Start:%d - expecting STBL hunk\n", c->start);
        return PW_BAD;
    }

    /* file size */
    if ((c->in_size - c->start) < 36)
        return PW_BAD;

    /************* OVTB ****************/
    if ((IN(c->start + 28) != 'O') ||
        (IN(c->start + 29) != 'V') ||
        (IN(c->start + 30) != 'T') ||
        (IN(c->start + 31) != 'B'))
    {
        pw_printf(c, "#1,2 Start:%d - expecting OVTB hunk\n", c->start);
        return PW_BAD;
    }

    /* nbr of Overtable */
    c->n = ((IN(c->start + 34) * 256) + IN(c->start + 35)) * 16;

    /* file size */
    if ((uint32_t)(c->in_size - c->start - 32) < (c->n + 8))
        return PW_BAD;

    c->n += 36; /* should now be on NTBL hunk */

    /************* NTBL ****************/
    if (!sa_tag(c, "NTBL"))
    {
        pw_printf(c, "#2 Start:%d - expecting NTBL hunk\n", c->start);
        return PW_BAD;
    }

    /* nbr of notes */
    c->m = ((IN(c->start + c->n + 6) * 256) + IN(c->start + c->n + 7)) * 4;

    /* file size */
    if ((c->in_size - c->start - c->n) < (c->m + 16))
    {
        pw_printf(c, "#2,1 (start:%d) (size:%d)\n", c->start, (int)(c->in_size - c->start - c->n));
        return PW_BAD;
    }

    c->n += (c->m + 8); /* should now be on INST hunk */

    /************* INST ****************/
    if (!sa_tag(c, "INST"))
    {
        pw_printf(c, "#3 Start:%d - expecting INST hunk\n", c->start);
        return PW_BAD;
    }

    /* nbr of instruments */
    c->m = ((IN(c->start + c->n + 6) * 256) + IN(c->start + c->n + 7)) * 152;

    /* file size */
    if ((c->in_size - c->start - c->n) < (c->m + 16))
    {
        pw_printf(c, "#3,1 (start:%d) (size:%d)\n", c->start, (int)(c->in_size - c->start - c->n));
        return PW_BAD;
    }

    c->n += (c->m + 8); /* should now be on SD8B hunk */

    /************* SD8B ****************/
    if (!sa_tag(c, "SD8B"))
    {
        pw_printf(c, "#4 Start:%d - expecting SD8B hunk\n", c->start);
        return PW_BAD;
    }

    /* nbr 8b samples */
    c->m = ((IN(c->start + c->n + 6) * 256) + IN(c->start + c->n + 7));

    /* sample data size, if any */
    if (c->m > 0)
    {
        c->k = 0;
        c->l = (c->m * 4) + (c->m * 4) + (c->m * 30);
        for (c->o = 0; c->o < c->m; c->o++)
        {
            c->j = (IN(c->start + c->n + c->l + (c->o * 4) + 10) * 256 +
                    IN(c->start + c->n + c->l + (c->o * 4) + 11));
            c->k += c->j;
        }
        c->l += (c->m * 4);
        c->m = c->l + c->k;
    }

    /* file size */
    if ((c->in_size - c->start - c->n) < (c->m + 16))
    {
        pw_printf(c, "#4,1 (start:%d) (size:%d)\n", c->start, (int)(c->in_size - c->start - c->n));
        return PW_BAD;
    }

    c->n += (c->m + 8); /* should now be on SYWT hunk */

    /************* SYWT ****************/
    if (!sa_tag(c, "SYWT"))
    {
        /* c->l is only set above when there are 8 bit samples */
        pw_printf(c, "#5 Start:%d (at %d) (PW_l:%d) - expecting SYWT hunk\n", c->start,
                  (int)(c->start + c->n), (int)c->l);
        return PW_BAD;
    }

    /* nbr synth wave tables */
    c->m = ((IN(c->start + c->n + 6) * 256) + IN(c->start + c->n + 7)) * 128;

    /* file size */
    if ((c->in_size - c->start - c->n) < (c->m + 16))
    {
        pw_printf(c, "#5,1 (start:%d) (size:%d)\n", c->start, (int)(c->in_size - c->start - c->n));
        return PW_BAD;
    }

    c->n += (c->m + 8); /* should now be on SYAR hunk */

    /************* SYAR ****************/
    if (!sa_tag(c, "SYAR"))
    {
        pw_printf(c, "#6 Start:%d (at %d) - expecting SYAR hunk\n", c->start, (int)(c->start + c->n));
        return PW_BAD;
    }

    /* nbr synth wave tables */
    c->m = ((IN(c->start + c->n + 6) * 256) + IN(c->start + c->n + 7)) * 128;

    /* file size */
    if ((c->in_size - c->start - c->n) < (c->m + 16))
    {
        pw_printf(c, "#6,1 (start:%d) (size:%d)\n", c->start, (int)(c->in_size - c->start - c->n));
        return PW_BAD;
    }

    c->n += (c->m + 8); /* should now be on SYAF hunk */

    /************* SYAF ****************/
    if (!sa_tag(c, "SYAF"))
    {
        pw_printf(c, "#7 Start:%d (at %d) - expecting SYAF hunk\n", c->start, (int)(c->start + c->n));
        return PW_BAD;
    }

    /* nbr synth wave tables */
    c->m = ((IN(c->start + c->n + 6) * 256) + IN(c->start + c->n + 7)); /* no known case, so size of block is unknown */
    if (c->m > 0)
        pw_printf(c, "testSA() - case of SYAF that is not 0 - send this file to Asle\n");

    /* file size */
    if ((c->in_size - c->start - c->n) < (c->m + 16))
    {
        pw_printf(c, "#7,1 (start:%d) (size:%d)\n", c->start, (int)(c->in_size - c->start - c->n));
        return PW_BAD;
    }

    c->n += (c->m + 8); /* should now be on EDATV1.1 hunk */

    /************* EDATV1.1 ****************/
    if (!sa_tag(c, "EDAT"))
    {
        pw_printf(c, "#8 Start:%d - expecting EDATV1.1 hunk\n", c->start);
        return PW_BAD;
    }

    /* nbr synth wave tables */
    c->m = 16; /* always 16 bytes, it seems */

    c->n += (c->m + 8); /* should now be at the end */

    return PW_GOOD;
}

/*
 * Sonic Arranger - no hunk
 */
int16_t pw_test_SA(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    c->n = 3;
    if (IN(c->start + c->n) > 0x7f)
        return PW_BAD;

    /* file size */
    if ((c->start + IN(c->start + c->n)) > c->in_size)
        return PW_BAD;

    /************* point after replay ****************/
    c->n = ((IN(c->start + 22) * 256) + IN(c->start + 23));
    c->n += 22;

    /* file size */
    if ((c->start + c->n) > (uint32_t)c->in_size)
    {
        pw_printf(c, "#1,3 (start:%d) (jump:%d)\n", c->start, (int)(c->start + c->n));
        return PW_BAD;
    }
    pw_printf(c, "testSA(%x) - jump to %x\n", (unsigned)c->start, c->n);

    /************* try to detect the next 'Nu' ni the next 100h bytes ****************/
    for (c->j = 0; c->j < 0x100; c->j++)
    {
        if ((IN(c->start + c->n + c->j) == 'N') &&
            (IN(c->start + c->n + c->j + 1) == 'u'))
            break;
    }
    if (c->j == 0x100)
    {
        pw_printf(c, "#2 Start:%d - no 'Nu' found\n", c->start);
        return PW_BAD;
    }
    pw_printf(c, "testSA(%x) - 'Nu' at %x\n", (unsigned)c->start, c->j);

    /************* try to detect the next usable dword ****************/
    c->n += (c->j + 2);
    if ((IN(c->start + c->n + 3) == 0x00) &&
        (IN(c->start + c->n + 5) == 0x00))
    {
        pw_printf(c, "#3 Start:%d (at %x) - consecutive 0x00 found\n", c->start, c->n + c->start);
        return PW_BAD;
    }
    if (IN(c->start + c->n + 5) != 0x00)
        c->n += 2;
    pw_printf(c, "testSA(%x) - first dword at %x\n", (unsigned)c->start, c->n);

    /************* real music data start here ****************/

    /* file size */
    if ((c->start + c->n + 32) > (uint32_t)c->in_size)
    {
        pw_printf(c, "#4 (start:%d) (size:%d)\n", c->start, (int)(c->start + c->n + 32));
        return PW_BAD;
    }

    /*************  pseudo SD8B hunk ****************/

    c->j = ((IN(c->start + c->n + 30) * 256) + IN(c->start + c->n + 31));
    c->n += c->j; /* this includes the 32 bytes of addresses */
    pw_printf(c, "testSA(%x) - S8BD hunk at %x\n", (unsigned)c->start, c->n);

    /* nbr 8b samples */
    c->m = ((IN(c->start + c->n + 2) * 256) + IN(c->start + c->n + 3));
    pw_printf(c, "testSA(%x) - nbr 8b data %x\n", (unsigned)c->start, c->m);
    c->n += 4;

    /* sample data size, if any (c->k is not reset when there is none) */
    if (c->m > 0)
    {
        c->k = 0;
        for (c->o = 0; c->o < c->m; c->o++)
        {
            c->j = (IN(c->start + c->n + (c->o * 4) + 2) * 256 + IN(c->start + c->n + (c->o * 4) + 3));
            pw_printf(c, "testSA(%x) - 8b data sizes %x\n", (unsigned)c->start, c->j);
            c->k += c->j;
        }
    }

    c->n += (c->k + (c->m * 4)); /* should now be on 'deadbeef' tag */

    /* OK, detect 0xF500 after the 'deadbeef' hunk size */
    for (c->j = 12; c->j < 0x100; c->j++)
    {
        if ((IN(c->start + c->n + c->j) == 0xF5) &&
            (IN(c->start + c->n + c->j + 1) == 0x00))
        {
            c->n += (c->j + 2);
            pw_printf(c, "0xF500 found at %x (%x)\n", c->j, c->n);
            break;
        }
        if ((IN(c->start + c->n + c->j)) > c->in_size)
        {
            pw_printf(c, "no 0xF500 found. size will be at PW_n (%x)\n", c->n);
            break;
        }
    }

    return PW_GOOD;
}

void pw_rip_SA(pw_ctx *c)
{
    c->out_size = c->n;

    c->convert = PW_BAD;
    pw_save_rip(c, "Sonic Arranger module", PWF_SA);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}
