/*
 * TurboSqueezer61.c - TurboSqueezer 6.1 Exe-file (upstream r/TurboSqueezer61.c).
 *
 * The original had the hunk header test and the header rebuild commented
 * out: the header is always taken as present.
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

/* the input bytes at 'off' are 'sig' (the original compared them one by one) */
static int match(const pw_ctx *c, int64_t off, const uint8_t *sig, int n)
{
    int k;

    for (k = 0; k < n; k++)
        if (IN(off + k) != sig[k])
            return 0;
    return 1;
}

int16_t pw_test_TurboSqueezer61(pw_ctx *c)
{
    static const uint8_t code[18] = {
        0xFF, 0xF0, 0xD1, 0xC8, 0xD1, 0xC8, 0x22, 0x58, 0x28,
        0x48, 0xD3, 0xC9, 0xD3, 0xC9, 0x58, 0x89, 0x2A, 0x49};

    c->start = (int32_t)c->i;

    if (!match(c, c->start + 12, code, 18))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 484) * 256 * 256 * 256) +
            (IN(c->start + 485) * 256 * 256) +
            (IN(c->start + 486) * 256) +
            IN(c->start + 487));

    c->l += 572;
    c->amiga_exe_header = PW_GOOD;

    return PW_GOOD;
    /* c->l is the size of the pack */
}

void pw_rip_TurboSqueezer61(pw_ctx *c)
{
    /* c->l is still the whole size */
    c->out_size = c->l;

    c->convert = PW_BAD;

    c->start -= 40;
    pw_save_rip(c, "TurboSqueezer 6.1 Exe-file", PWF_TurboSqueezer61);

    if (c->save_status == PW_GOOD)
        c->i += 44;
}
