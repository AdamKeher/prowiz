/*
 * Dragpack252.c - DragPack 2.52 executables (upstream r/Dragpack252.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_Dragpack252(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 16) != 0x01) ||
        (IN(c->start + 17) != 0x50) ||
        (IN(c->start + 18) != 0x41) ||
        (IN(c->start + 19) != 0xFA) ||
        (IN(c->start + 22) != 0x24) ||
        (IN(c->start + 23) != 0x48) ||
        (IN(c->start + 24) != 0x22) ||
        (IN(c->start + 25) != 0x48) ||
        (IN(c->start + 26) != 0x4B) ||
        (IN(c->start + 27) != 0xE8) ||
        (IN(c->start + 30) != 0xD3) ||
        (IN(c->start + 31) != 0xFC))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 38) * 256 * 256 * 256) +
            (IN(c->start + 39) * 256 * 256) +
            (IN(c->start + 40) * 256) +
            IN(c->start + 41));

    c->l += 0x170;
    if ((c->l / 4) * 4 != c->l)
        c->l += 2; /* aligned on dwords it seems */

    if (c->i >= 32)
    {
        if ((IN(c->start - 32) != 0x00) ||
            (IN(c->start - 31) != 0x00) ||
            (IN(c->start - 30) != 0x03) ||
            (IN(c->start - 29) != 0xF3) ||
            (IN(c->start - 28) != 0x00) ||
            (IN(c->start - 27) != 0x00) ||
            (IN(c->start - 26) != 0x00) ||
            (IN(c->start - 25) != 0x00) ||
            (IN(c->start - 24) != 0x00) ||
            (IN(c->start - 23) != 0x00) ||
            (IN(c->start - 22) != 0x00) ||
            (IN(c->start - 21) != 0x01) ||
            (IN(c->start - 20) != 0x00) ||
            (IN(c->start - 19) != 0x00) ||
            (IN(c->start - 18) != 0x00) ||
            (IN(c->start - 17) != 0x00))
        {
            c->amiga_exe_header = PW_BAD;
        }
        else
            c->amiga_exe_header = PW_GOOD;
    }
    else
        c->amiga_exe_header = PW_BAD;

    return PW_GOOD;
    /* c->l is the size of the pack */
}

void pw_rip_Dragpack252(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t hdr[32];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 32;
        memset(hdr, 0, sizeof hdr);
        hdr[2] = hdr[26] = 0x03;
        hdr[3] = 0xF3;
        hdr[11] = 0x01;
        hdr[27] = 0xE9;

        /* the original copied the bytes of PW_j in reverse (x86) order */
        c->j = c->l - 36;
        c->j /= 4;
        hdr[20] = hdr[28] = (uint8_t)(c->j >> 24);
        hdr[21] = hdr[29] = (uint8_t)(c->j >> 16);
        hdr[22] = hdr[30] = (uint8_t)(c->j >> 8);
        hdr[23] = hdr[31] = (uint8_t)c->j;
        pw_save_rip_special(c, "DragPack 2.52 Exe-file", PWF_DragPack252, hdr, 32);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "DragPack 2.52 Exe-file", PWF_DragPack252);
    }

    if (c->save_status == PW_GOOD)
    {
        /* Guard (not in the original): when OutputSize - 36 wraps (a size
         * below 36, or a huge one) and brings the scan back to or before this
         * position, the original found the same file again forever. Stay
         * here instead. A step that wraps past the end is kept. */
        if ((uint32_t)(c->i + (c->out_size - 36) + 1) > c->i)
            c->i += (c->out_size - 36); /* 32 should do but call it "just to be sure" :) */
    }
}
