/*
 * new_formats.c - dispatch to the handlers of the formats Pro-Wizard 1.70a
 *                 never had (see new_formats.h), and their shared helpers.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "new_formats.h"

#define B(k) pw_in8(c, (int64_t)(uint32_t)(c->i + (k)))

int pw_match(const pw_ctx *c, int64_t off, const char *s, size_t n)
{
    size_t k;
    for (k = 0; k < n; k++)
        if (pw_in8(c, off + (int64_t)k) != (uint8_t)s[k])
            return 0;
    return 1;
}

uint8_t *pw_copy_in(pw_ctx *c, int64_t off, uint32_t size)
{
    uint8_t *p = (uint8_t *)malloc(size ? size : 1);
    uint32_t k;

    if (p == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        return NULL;
    }
    for (k = 0; k < size; k++)
        p[k] = pw_in8(c, off + k);
    return p;
}

/* the first bytes pw_scan_new() dispatches on (keep in step with it) */
const uint8_t pw_new_first_byte[256] = {
    ['.'] = 1, ['A'] = 1, ['D'] = 1, ['L'] = 1, ['M'] = 1, ['P'] = 1, ['S'] = 1,
    ['T'] = 1, ['t'] = 1, [0x60] = 1, [0x08] = 1, [0x47] = 1, [0x48] = 1,
};

/* Called after the original's checks at the scan positions whose byte is in
 * pw_new_first_byte. All of these are music formats. A handler that saves
 * something moves c->i past it. */
void pw_scan_new(pw_ctx *c)
{
    if ((c->modes & PW_MODE_MUSIC) == 0)
        return;

    switch (B(0))
    {
    case '.':
        pw_new_DeltaMusic2(c);
        break;
    case 'A':
        if (!pw_new_ArtOfNoise(c))
            pw_new_StartrekkerNT(c);
        break;
    case 'D':
        pw_new_DigiBoosterPro(c);
        break;
    case 'L':
        pw_new_LSP(c);
        break;
    case 'M':
        pw_new_Musicline(c);
        break;
    case 'P':
        pw_new_PreTracker(c);
        break;
    case 'S':
        if (!pw_new_SidMon2(c) && !pw_new_Symphonie(c))
            pw_new_StartrekkerNT(c);
        break;
    case 'T':
    case 't':
        pw_new_TFMX(c);
        break;
    case 0x60:
        if (!pw_new_RobHubbard(c))
            pw_new_BenDaglish(c);
        break;
    case 0x08:
    case 0x47:
    case 0x48:
        pw_new_DavidWhittaker(c);
        break;
    default:
        break;
    }
}
