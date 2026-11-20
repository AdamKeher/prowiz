/*
 * Mugician.c - Digital Mugician 1/2 module (upstream r/Mugician.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_MUGICIAN(pw_ctx *c)
{
    if (c->i < 2)
        return PW_BAD;
    c->start = (int32_t)(c->i - 2);

    if ((IN(c->start) != ' ') ||
        (IN(c->start + 1) != 'M') ||
        (IN(c->start + 6) != 'I') ||
        (IN(c->start + 7) != 'A') ||
        (IN(c->start + 8) != 'N') ||
        (IN(c->start + 28) != 0x00) ||
        (IN(c->start + 29) != 0x00) ||
        (IN(c->start + 32) != 0x00) ||
        (IN(c->start + 33) != 0x00) ||
        (IN(c->start + 36) != 0x00) ||
        (IN(c->start + 37) != 0x00) ||
        (IN(c->start + 40) != 0x00) ||
        (IN(c->start + 41) != 0x00))
        return PW_BAD;

    /* number of (real) samples */
    c->j = ((IN(c->start + 0x44) * 256 * 256 * 256) +
            (IN(c->start + 0x45) * 256 * 256) +
            (IN(c->start + 0x46) * 256) +
            IN(c->start + 0x47));
    /* sample data size */
    c->wss = ((IN(c->start + 0x48) * 256 * 256 * 256) +
              (IN(c->start + 0x49) * 256 * 256) +
              (IN(c->start + 0x4A) * 256) +
              IN(c->start + 0x4B));
    if (((c->j != 0) && (c->wss == 0)) || ((c->j == 0) && (c->wss != 0)) || (c->j > 255))
        return PW_BAD;

    /* number of instrument */
    c->k = ((IN(c->start + 0x3C) * 256 * 256 * 256) +
            (IN(c->start + 0x3D) * 256 * 256) +
            (IN(c->start + 0x3E) * 256) +
            IN(c->start + 0x3F));
    if ((c->k == 0) || (c->k > 255))
        return PW_BAD;

    /*
     * not many checks, here, but, to my point of view, it's unlikely
     * a music format comes anywhere near this one, so ... .
     */
    return PW_GOOD;
}

void pw_rip_MUGICIAN(pw_ctx *c)
{
    /*
     * c->wss : sample data size
     * c->j : number of sample
     * c->k : number of instrument
     */
    c->out_size = (c->j * 0x20) + (c->k * 0x10) + c->wss + 0x1CC;

    /* nbr of patterns */
    c->k = ((IN(c->start + 0x1a) * 256) + IN(c->start + 0x1b));
    c->out_size += (c->k * 0x100);
    /* nbr of waveforms */
    c->k = ((IN(c->start + 0x40) * 256 * 256 * 256) +
            (IN(c->start + 0x41) * 256 * 256) +
            (IN(c->start + 0x42) * 256) +
            IN(c->start + 0x43));
    c->out_size += (c->k * 0x80);
    /* nbr of sequences */
    c->l = 0;
    for (c->j = 0; c->j < 8; c->j++)
    {
        c->k = ((IN(c->start + 0x1c + (c->j * 4)) * 256 * 256 * 256) +
                (IN(c->start + 0x1d + (c->j * 4)) * 256 * 256) +
                (IN(c->start + 0x1e + (c->j * 4)) * 256) +
                IN(c->start + 0x1f + (c->j * 4)));
        c->l += c->k;
    }

    c->out_size += (c->l * 8);

    c->convert = PW_BAD;
    pw_save_rip(c, "Digital Mugician 1/2 module", PWF_dmu);

    if (c->save_status == PW_GOOD)
        c->i += 24; /* after the ID */
}
