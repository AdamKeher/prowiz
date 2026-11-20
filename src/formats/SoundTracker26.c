/*
 * SoundTracker26.c - Soundtracker 2.6 & IceTracker 1.0 (upstream r/SoundTracker26.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* Soundtracker 2.6 & IceTracker 1.0 */
int16_t pw_test_STK26(pw_ctx *c)
{
    /* test 1 */
    if (c->i < 1464)
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 1464);
    c->wss = 0;
    for (c->j = 0; c->j < 31; c->j++)
    {
        if (IN(c->start + 45 + 30 * c->j) > 0x40)
            return PW_BAD;
        if (IN(c->start + 44 + 30 * c->j) > 0x0F)
            return PW_BAD;
        c->wss += (((IN(c->start + 42 + c->j * 30) * 256) + IN(c->start + 43 + c->j * 30)) * 2);
    }

    /* c->wss is the whole sample size :) */
    return PW_GOOD;
}

void pw_rip_STK26(pw_ctx *c)
{
    /* c->wss is the whole sample siz */
    c->out_size = (IN(c->start + 951) * 256) + 1468 + c->wss;

    c->convert = PW_GOOD;
    if (IN(c->start + 1464) == 'M')
        pw_save_rip(c, "Sountracker 2.6 module", PWF_STK26);
    else
        pw_save_rip(c, "IceTracker 1.0 modul", PWF_IceTracker);

    if (c->save_status == PW_GOOD)
    {
        uint32_t next = c->i + (c->out_size - 1470); /* 1464 should do but call it "just to be sure" :) */
        /* Hang guard (not in the original): a module of 1468 bytes (no
         * samples, byte 951 = 0) gives a step of -2, so upstream moves the
         * scan position back, reaches this module again and rips it forever.
         * Keep the position instead; the scan goes on. */
        if (next >= c->i)
            c->i = next;
    }
}

/*
 *   STK26.c   1999 (c) Asle / ReDoX
 *
 * handles Soundtracker 2.6 and IceTracker 1.0 file
 * convert them back to PTK
 *
 * Update: 19/04/00
 *  - forced 7F value to NTK byte
 * Update: 29/05/02
 *  - above update was bugged ... great.
 */
void pw_depack_STK26(pw_ctx *c)
{
    uint8_t  whatever[1536];
    int32_t  whole_sample_size = 0;
    int32_t  where = c->start;
    int32_t  i = 0, j, k;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* read and write whole header */
    pw_fwrite_in(out, where, 952);

    /* get whole sample size */
    for (i = 0; i < 31; i++)
        whole_sample_size += (((IN(where + 42 + i * 30) * 256) + IN(where + 43 + i * 30)) * 2);

    /* generate patlist */
    memset(whatever, 0, sizeof whatever);
    whatever[1024] = IN(where + 950);
    for (i = 0; i < whatever[1024]; i++, whatever[256] += 0x01)
        whatever[i] = whatever[256];
    pw_fwrite(out, whatever, 128);

    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    whatever[3] = '.';
    pw_fwrite(out, whatever, 4);

    /* NTK byte */
    whatever[0] = 0x7f;
    pw_fseek(out, 951, 0);
    pw_fwrite(out, &whatever[0], 1);
    pw_fseek(out, 0, 2);

    /* I dont care highest track number and go on blindly to convert shit ! */
    where += 952;
    for (i = 0; i < whatever[1024]; i++)
    {
        for (j = 0; j < 4; j++)
        {
            for (k = 0; k < 64; k++)
            {
                whatever[k * 16 + j * 4]     = IN(where + (IN(where + i * 4 + j)) * 256 + k * 4 + 516);
                whatever[k * 16 + j * 4 + 1] = IN(where + (IN(where + i * 4 + j)) * 256 + k * 4 + 517);
                whatever[k * 16 + j * 4 + 2] = IN(where + (IN(where + i * 4 + j)) * 256 + k * 4 + 518);
                whatever[k * 16 + j * 4 + 3] = IN(where + (IN(where + i * 4 + j)) * 256 + k * 4 + 519);
            }
        }
        pw_fwrite(out, whatever, 1024);
    }

    /* sample data (upstream reads offset 951 of the file, not of the module) */
    where = 1468 + IN(951) * 256;
    pw_fwrite_in(out, where, whole_sample_size);

    /* crap */
    if (IN(c->start + 1464) == 'M')
        pw_crap(" SoundTracker 2.6 ", PW_BAD, PW_BAD, out);
    else
        pw_crap("  IceTracker 1.0  ", PW_BAD, PW_BAD, out);

    pw_fclose(out);
}
