/*
 * S3M.c - ScreamTracker III module, "SCRM" (upstream r/S3M.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_S3M(pw_ctx *c)
{
    /* test #1 */
    c->start = (int32_t)(c->i - 44);
    if ((c->start + 0x60) > c->in_size)
        return PW_BAD;
    /* must be 0x1a10 */
    if ((IN(c->start + 28) != 0x1A) || (IN(c->start + 29) != 0x10))
        return PW_BAD;
    /* must be 0x0100 or 0x0200 */
    if (((IN(c->start + 42) != 0x01) && (IN(c->start + 42) != 0x02)) || (IN(c->start + 43) != 0x00))
        return PW_BAD;

    /* get patternlist size */
    /* (an uneven size is accepted: unreal musics have that ?!?) */
    c->j = ((IN(c->start + 33) * 256) + IN(c->start + 32));

    /* get number of instruments */
    c->k = ((IN(c->start + 35) * 256) + IN(c->start + 34));
    /* get number of pattern stored */
    c->l = ((IN(c->start + 37) * 256) + IN(c->start + 36));

    /* c->j is the patternlist size */
    /* c->k is the number of instruments */
    /* c->l is the number of pattern stored */
    return PW_GOOD;
}

/*
 * S3M ripper
 * 20100207 - Sylvain "Asle" Chipaux
 *
 * pfiew - finaly completed. some "features" not documented :(
 */
void pw_rip_S3M(pw_ctx *c)
{
    int32_t currentptr;
    int32_t max = 0;
    int32_t whole_head_size = 0;

    /* read the pattern adresses and get sizes */
    currentptr = (int32_t)(c->start + 0x60 + c->j + (c->k * 2));
    max = 0;
    for (c->o = 0; c->o < c->l; c->o++)
    {
        int32_t tmp_addy = 0;
        tmp_addy = ((IN(currentptr + (c->o * 2) + 1) * 256) + IN(currentptr + (c->o * 2))) * 16;
        if (tmp_addy == 0)
            continue;
        if (tmp_addy > max)
        {
            max = tmp_addy;
            whole_head_size = max + ((IN(tmp_addy + 1 + c->start) * 256) + IN(tmp_addy + c->start));
        }
    }
    while (((whole_head_size / 16) * 16) != whole_head_size)
    {
        whole_head_size += 1;
    }

    /* read the instruments adresses and get sizes */
    /* they should be _before_ the pattern data, handled above */
    currentptr = (int32_t)(c->start + 0x60 + c->j);
    for (c->o = 0; c->o < c->k; c->o++)
    {
        int32_t tmp_addy = 0, tmp_addy2 = 0;
        tmp_addy = ((IN(currentptr + (c->o * 2) + 1) * 256) + IN(currentptr + (c->o * 2))) * 16;
        if (tmp_addy == 0)
            continue;
        if (IN(tmp_addy + c->start) == 1)
        {
            tmp_addy2 = ((IN(tmp_addy + 15 + c->start) * 256) + IN(tmp_addy + 14 + c->start)) * 16;
            if (tmp_addy2 > max)
            {
                int32_t t = ((IN(tmp_addy + 18 + c->start) * 256 * 256) + (IN(tmp_addy + 17 + c->start) * 256) +
                             IN(tmp_addy + 16 + c->start));
                /* upstream reads this flag relative to the file, not the module */
                if ((IN(tmp_addy + 31) & 0x04) == 0x04)
                    t *= 2;
                max = tmp_addy2;
                whole_head_size = max + t;
            }
        }
    }

    c->out_size = (uint32_t)whole_head_size;
    pw_printf(c, "\nwhole_head_size : %d\n"
                 "PW_j : %x\n"
                 "PW_k*2 : %x\n"
                 "PW_l*2 : %x\n",
              whole_head_size, c->j, (c->k * 2), (c->l * 2));

    c->convert = PW_BAD;
    pw_save_rip(c, "ScreamTracker III module", PWF_S3M);

    if (c->save_status == PW_GOOD)
        c->i += 1;
}
