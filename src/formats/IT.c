/*
 * IT.c - Impulse Tracker module (upstream r/IT.c, first try 20110808).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_IT(pw_ctx *c)
{
    /* test #1 */
    c->start = (int32_t)c->i;
    if ((c->start + 0xc0) > c->in_size)
        return PW_BAD;
    /* must be < 10 */
    if ((IN(c->start + 41) >= 0x0f) || (IN(c->start + 43) >= 0x0F))
        return PW_BAD;

    c->k = 0xc0 + (IN(c->start + 33) * 256) + IN(c->start + 32);
    if (c->k > (uint32_t)c->in_size)
        return PW_BAD;
    c->j = c->k + ((IN(c->start + 35) * 256) + IN(c->start + 34)) * 4;
    if (c->j > (uint32_t)c->in_size)
        return PW_BAD;
    c->l = c->j + ((IN(c->start + 37) * 256) + IN(c->start + 36)) * 4;
    if (c->l > (uint32_t)c->in_size)
        return PW_BAD;

    return PW_GOOD;
    /* c->k => inst addresses */
    /* c->j => smp headers addresses */
    /* c->l => patterns addresses */
}

/*
 * IT ripper
 * 20110808 - Sylvain "Asle" Chipaux
 *
 * let's try it
 * OK, what's is the size of one compressed sample ?!?!?
 */
void pw_rip_IT(pw_ctx *c)
{
    /*
     * from the test part ->
     *  c->k => inst addresses
     *  c->j => smp headers addresses
     *  c->l => patterns addresses
     */
    int32_t ptr_inst = (int32_t)c->k;
    int32_t nbr_inst = (IN(c->start + 35) * 256) + IN(c->start + 34);
    int32_t ptr_smp_head = (int32_t)c->j;
    int32_t nbr_smp_head = (IN(c->start + 37) * 256) + IN(c->start + 36);
    int32_t ptr_patt = (int32_t)c->l;
    int32_t currentptr;
    int32_t max = 0;
    uint8_t compressed = 1;

    pw_printf(c, "\nPtrInst:%x\nPtrSmpHead:%x\nPtrPatt:%x\n",
              (unsigned)ptr_inst, (unsigned)ptr_smp_head, (unsigned)ptr_patt);

    /* let's get the highest address of an instrument (if any is set) */
    if (nbr_inst > 0)
    {
        currentptr = c->start + ptr_inst;
        for (c->k = 0; c->k < (uint32_t)nbr_inst; c->k++)
        {
            int32_t tmp_addy = ((IN(currentptr + (c->k * 4) + 3) * 256 * 256 * 256) +
                                (IN(currentptr + (c->k * 4) + 2) * 256 * 256) +
                                (IN(currentptr + (c->k * 4) + 1) * 256) +
                                IN(currentptr + (c->k * 4)));
            if (tmp_addy == 0)
                continue;
            if (tmp_addy > max)
                max = tmp_addy + 554;
            /* inst have constant size */
        }
    }

    /* let's get the highest address of a sample (if any is set) */
    if (nbr_smp_head > 0)
    {
        currentptr = c->start + ptr_smp_head;
        for (c->k = 0; c->k < (uint32_t)nbr_smp_head; c->k++)
        {
            /* address of smaple header */
            int32_t tmp_addy = ((IN(currentptr + (c->k * 4) + 3) * 256 * 256 * 256) +
                                (IN(currentptr + (c->k * 4) + 2) * 256 * 256) +
                                (IN(currentptr + (c->k * 4) + 1) * 256) +
                                IN(currentptr + (c->k * 4)));
            /* then address of its sample data */
            int32_t tmp_addy2 = (IN(c->start + tmp_addy + 0x4b) * 256 * 256 * 256) +
                                (IN(c->start + tmp_addy + 0x4a) * 256 * 256) +
                                (IN(c->start + tmp_addy + 0x49) * 256) +
                                IN(c->start + tmp_addy + 0x48);
            pw_printf(c, "smp addy[%2x]:%x (max:%x)(at:%x) ", c->k, (unsigned)tmp_addy,
                      (unsigned)max, (unsigned)tmp_addy2);
            if (tmp_addy == 0)
                continue;
            if (tmp_addy2 == 0)
                pw_printf(c, "---");
            if (tmp_addy > max)
                max = tmp_addy + 0x50;
            if (tmp_addy2 >= max)
            {
                int32_t tmp_size = (IN(c->start + tmp_addy + 0x33) * 256 * 256 * 256) +
                                   (IN(c->start + tmp_addy + 0x32) * 256 * 256) +
                                   (IN(c->start + tmp_addy + 0x31) * 256) +
                                   IN(c->start + tmp_addy + 0x30);
                if ((IN(c->start + tmp_addy + 0x12) & 0x02) == 0x02) /* 16 bits ? */
                    tmp_size *= 2;
                /* OK, what's the size of 'compressed' samples ?!? */
                if ((IN(c->start + tmp_addy + 0x12) & 0x08) == 0x08) /* compressed ? */
                {
                    pw_printf(c, "*");
                    compressed = 2;
                    tmp_size /= 2; /* just to be sure */
                }
                else
                    compressed = 1;
                pw_printf(c, "- size:%x)", (unsigned)tmp_size);
                max = tmp_addy2 + tmp_size;
            }
            pw_printf(c, "\n");

            /* here, max is now after the farthest sample data */
        }
    }

    /* PATTERNS could be _after_ the samples ... never seen but ...
       shall have to be handled, some day */

    c->out_size = (uint32_t)max;

    c->convert = PW_BAD;
    if (compressed == 2)
    {
        pw_printf(c, "found Impulse Tracker module at %d with last sample compressed - can't save\n", c->start);
        pw_report_unrippable(c, PWF_ImpulseTracker, "Impulse Tracker module (last sample compressed)",
                             (uint32_t)c->start);
        c->save_status = PW_GOOD;
    }
    else
    {
        pw_save_rip(c, "Impulse Tracker module", PWF_ImpulseTracker);
    }

    if (c->save_status == PW_GOOD)
        c->i += 1;
}
