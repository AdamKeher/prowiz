/*
 * Bytekiller.c - ByteKiller 1.3/2.0/3.0, ByteKillerPro 1.0, ByteKiller Clone
 *                FLT and unknown ByteKiller Clone 5 executables
 *                (upstream r/Bytekiller.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_ByteKiller_13(pw_ctx *c)
{
    if (pw_test_1_start(c, 135) == PW_BAD)
        return PW_BAD;

    c->start = (int32_t)(c->i - 135);

    if ((IN(c->start)      != 0x41) ||
        (IN(c->start + 1)  != 0xFA) ||
        (IN(c->start + 2)  != 0x00) ||
        (IN(c->start + 3)  != 0xE6) ||
        (IN(c->start + 4)  != 0x43) ||
        (IN(c->start + 5)  != 0xF9) ||
        (IN(c->start + 10) != 0x20) ||
        (IN(c->start + 11) != 0x18) ||
        (IN(c->start + 12) != 0x22) ||
        (IN(c->start + 13) != 0x18) ||
        (IN(c->start + 14) != 0x2A) ||
        (IN(c->start + 15) != 0x18) ||
        (IN(c->start + 16) != 0x24) ||
        (IN(c->start + 17) != 0x49) ||
        (IN(c->start + 18) != 0xD1) ||
        (IN(c->start + 19) != 0xC0) ||
        (IN(c->start + 20) != 0xD5) ||
        (IN(c->start + 21) != 0xC1) ||
        (IN(c->start + 22) != 0x20) ||
        (IN(c->start + 23) != 0x20) ||
        (IN(c->start + 24) != 0xB1) ||
        (IN(c->start + 25) != 0x85))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 232) * 256 * 256 * 256) +
            (IN(c->start + 233) * 256 * 256) +
            (IN(c->start + 234) * 256) +
            IN(c->start + 235));

    c->l += 304;

    if (c->i >= 171)
    {
        if ((IN(c->start - 36) != 0x00) ||
            (IN(c->start - 35) != 0x00) ||
            (IN(c->start - 34) != 0x03) ||
            (IN(c->start - 33) != 0xF3) ||
            (IN(c->start - 32) != 0x00) ||
            (IN(c->start - 31) != 0x00) ||
            (IN(c->start - 30) != 0x00) ||
            (IN(c->start - 29) != 0x00) ||
            (IN(c->start - 28) != 0x00) ||
            (IN(c->start - 27) != 0x00) ||
            (IN(c->start - 26) != 0x00) ||
            (IN(c->start - 25) != 0x02) ||
            (IN(c->start - 24) != 0x00) ||
            (IN(c->start - 23) != 0x00) ||
            (IN(c->start - 22) != 0x00) ||
            (IN(c->start - 21) != 0x00))
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

int16_t pw_test_ByteKiller_20(pw_ctx *c)
{
    if (pw_test_1_start(c, 127) == PW_BAD)
        return PW_BAD;
    c->start = (int32_t)(c->i - 127);

    if ((IN(c->start)      != 0x48) ||
        (IN(c->start + 1)  != 0xE7) ||
        (IN(c->start + 2)  != 0xFF) ||
        (IN(c->start + 3)  != 0xFE) ||
        (IN(c->start + 4)  != 0x4D) ||
        (IN(c->start + 5)  != 0xF9) ||
        (IN(c->start + 6)  != 0x00) ||
        (IN(c->start + 10) != 0x41) ||
        (IN(c->start + 11) != 0xFA) ||
        (IN(c->start + 12) != 0x00) ||
        (IN(c->start + 13) != 0xBA) ||
        (IN(c->start + 14) != 0x43) ||
        (IN(c->start + 15) != 0xF9) ||
        (IN(c->start + 20) != 0x20) ||
        (IN(c->start + 21) != 0x18) ||
        (IN(c->start + 22) != 0x22) ||
        (IN(c->start + 23) != 0x18) ||
        (IN(c->start + 24) != 0x2A) ||
        (IN(c->start + 25) != 0x18))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 198) * 256 * 256 * 256) +
            (IN(c->start + 199) * 256 * 256) +
            (IN(c->start + 200) * 256) +
            IN(c->start + 201));

    c->l += 272;

    if (c->i >= 163)
    {
        if ((IN(c->start - 36) != 0x00) ||
            (IN(c->start - 35) != 0x00) ||
            (IN(c->start - 34) != 0x03) ||
            (IN(c->start - 33) != 0xF3) ||
            (IN(c->start - 32) != 0x00) ||
            (IN(c->start - 31) != 0x00) ||
            (IN(c->start - 30) != 0x00) ||
            (IN(c->start - 29) != 0x00) ||
            (IN(c->start - 28) != 0x00) ||
            (IN(c->start - 27) != 0x00) ||
            (IN(c->start - 26) != 0x00) ||
            (IN(c->start - 25) != 0x02) ||
            (IN(c->start - 24) != 0x00) ||
            (IN(c->start - 23) != 0x00) ||
            (IN(c->start - 22) != 0x00) ||
            (IN(c->start - 21) != 0x00))
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

int16_t pw_test_ByteKiller30(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 16) != 0x00) ||
        (IN(c->start + 17) != 0xDF) ||
        (IN(c->start + 18) != 0xF1) ||
        (IN(c->start + 19) != 0x80) ||
        (IN(c->start + 20) != 0x20) ||
        (IN(c->start + 21) != 0x18) ||
        (IN(c->start + 22) != 0x22) ||
        (IN(c->start + 23) != 0x18) ||
        (IN(c->start + 24) != 0xD1) ||
        (IN(c->start + 25) != 0xC0) ||
        (IN(c->start + 26) != 0x20) ||
        (IN(c->start + 27) != 0x10) ||
        (IN(c->start + 28) != 0x24) ||
        (IN(c->start + 29) != 0x49) ||
        (IN(c->start + 30) != 0xD5) ||
        (IN(c->start + 31) != 0xC1) ||
        (IN(c->start + 32) != 0x7A) ||
        (IN(c->start + 33) != 0x03))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 188) * 256 * 256 * 256) +
            (IN(c->start + 189) * 256 * 256) +
            (IN(c->start + 190) * 256) +
            IN(c->start + 191));

    c->l += 236;

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

int16_t pw_test_bytekillerpro10(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((IN(c->start + 22) != 0x00) ||
        (IN(c->start + 23) != 0x04) ||
        (IN(c->start + 24) != 0x2A) ||
        (IN(c->start + 25) != 0x28) ||
        (IN(c->start + 26) != 0x00) ||
        (IN(c->start + 27) != 0x08) ||
        (IN(c->start + 28) != 0x41) ||
        (IN(c->start + 29) != 0xE8) ||
        (IN(c->start + 30) != 0x00) ||
        (IN(c->start + 31) != 0x0C) ||
        (IN(c->start + 32) != 0x24) ||
        (IN(c->start + 33) != 0x49) ||
        (IN(c->start + 34) != 0xD1) ||
        (IN(c->start + 35) != 0xC0) ||
        (IN(c->start + 36) != 0xD5) ||
        (IN(c->start + 37) != 0xC1))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 222) * 256 * 256 * 256) +
            (IN(c->start + 223) * 256 * 256) +
            (IN(c->start + 224) * 256) +
            IN(c->start + 225));

    c->l += 308;

    if (c->i >= 50)
    {
        if ((IN(c->start - 50) != 0x00) ||
            (IN(c->start - 49) != 0x00) ||
            (IN(c->start - 48) != 0x03) ||
            (IN(c->start - 47) != 0xF3) ||
            (IN(c->start - 46) != 0x00) ||
            (IN(c->start - 45) != 0x00) ||
            (IN(c->start - 44) != 0x00) ||
            (IN(c->start - 43) != 0x00) ||
            (IN(c->start - 42) != 0x00) ||
            (IN(c->start - 41) != 0x00) ||
            (IN(c->start - 40) != 0x00) ||
            (IN(c->start - 39) != 0x02) ||
            (IN(c->start - 38) != 0x00) ||
            (IN(c->start - 37) != 0x00) ||
            (IN(c->start - 36) != 0x00) ||
            (IN(c->start - 35) != 0x00))
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

int16_t pw_test_BKCloneFLT(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((c->i + 308) > (uint32_t)c->in_size)
        return PW_BAD;

    if ((IN(c->start + 14) != 0x24) ||
        (IN(c->start + 15) != 0x60) ||
        (IN(c->start + 16) != 0xd5) ||
        (IN(c->start + 17) != 0xc9) ||
        (IN(c->start + 18) != 0x4a) ||
        (IN(c->start + 19) != 0xa0) ||
        (IN(c->start + 20) != 0x20) ||
        (IN(c->start + 21) != 0x20) ||
        (IN(c->start + 22) != 0x13) ||
        (IN(c->start + 23) != 0xf9) ||
        (IN(c->start + 24) != 0x00) ||
        (IN(c->start + 25) != 0xdf) ||
        (IN(c->start + 26) != 0xf0) ||
        (IN(c->start + 27) != 0x06) ||
        (IN(c->start + 28) != 0x00) ||
        (IN(c->start + 29) != 0xdf))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 304) * 256 * 256 * 256) +
            (IN(c->start + 305) * 256 * 256) +
            (IN(c->start + 306) * 256) +
            IN(c->start + 307));

    c->l += 348;

    if (c->i >= 36)
    {
        if ((IN(c->start - 36) != 0x00) ||
            (IN(c->start - 35) != 0x00) ||
            (IN(c->start - 34) != 0x03) ||
            (IN(c->start - 33) != 0xF3) ||
            (IN(c->start - 32) != 0x00) ||
            (IN(c->start - 31) != 0x00) ||
            (IN(c->start - 30) != 0x00) ||
            (IN(c->start - 29) != 0x00) ||
            (IN(c->start - 28) != 0x00) ||
            (IN(c->start - 27) != 0x00) ||
            (IN(c->start - 26) != 0x00) ||
            (IN(c->start - 25) != 0x02) ||
            (IN(c->start - 24) != 0x00) ||
            (IN(c->start - 23) != 0x00) ||
            (IN(c->start - 22) != 0x00) ||
            (IN(c->start - 21) != 0x00))
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

/* exe found in Anarchy demo Seeing Is Believing (added 20130415) */
int16_t pw_test_BKClone5(pw_ctx *c)
{
    c->start = (int32_t)c->i;

    if ((c->i + 284) > (uint32_t)c->in_size)
        return PW_BAD;

    if ((IN(c->start + 18) != 0xD5) ||
        (IN(c->start + 19) != 0xC9) ||
        (IN(c->start + 20) != 0x20) ||
        (IN(c->start + 21) != 0x20) ||
        (IN(c->start + 22) != 0x47) ||
        (IN(c->start + 23) != 0xFA) ||
        (IN(c->start + 24) != 0x00) ||
        (IN(c->start + 25) != 0xE0) ||
        (IN(c->start + 26) != 0x72) ||
        (IN(c->start + 27) != 0x03) ||
        (IN(c->start + 28) != 0x4E) ||
        (IN(c->start + 29) != 0x93))
    {
        /* should be enough :))) */
        return PW_BAD;
    }

    /* packed size */
    c->l = (((uint32_t)IN(c->start + 6) * 256 * 256 * 256) +
            (IN(c->start + 7) * 256 * 256) +
            (IN(c->start + 8) * 256) +
            IN(c->start + 9));

    c->l += 284;

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

/* Writes the 24 bytes ending a 2-hunk exe (HUNK_RELOC32 .. HUNK_END) over
 * the last 24 bytes of the rip: they are frequently 'removed'. */
static void put_exe_tail24(pw_ctx *c)
{
    static const uint8_t tail[24] = {
        0x00, 0x00, 0x03, 0xEC, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xF2, 0x00, 0x00, 0x03, 0xEB,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0xF2};
    uint32_t k;

    for (k = 0; k < 24; k++)
        IN_SET(c->start + c->out_size - 24 + k, tail[k]);
}

void pw_rip_ByteKiller(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t hdr[36];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 36;
        memset(hdr, 0, sizeof hdr);
        hdr[2] = hdr[30] = 0x03;
        hdr[3] = 0xF3;
        hdr[11] = 0x02;
        hdr[19] = 0x01;
        hdr[27] = 0x01;
        hdr[31] = 0xE9;

        /* the original copied the bytes of PW_j in reverse (x86) order */
        c->j = c->l - 60;
        c->j /= 4;
        hdr[20] = hdr[32] = (uint8_t)(c->j >> 24);
        hdr[21] = hdr[33] = (uint8_t)(c->j >> 16);
        hdr[22] = hdr[34] = (uint8_t)(c->j >> 8);
        hdr[23] = hdr[35] = (uint8_t)c->j;

        /* also the last 24 bytes are 'removed' frequently ... Here they are */
        put_exe_tail24(c);

        pw_save_rip_special(c, "ByteKiller 1.3/2.0 Exe-file", PWF_ByteKiller, hdr, 36);
    }
    else
    {
        c->start -= 36;
        pw_save_rip(c, "ByteKiller 1.3/2.0 Exe-file", PWF_ByteKiller);
    }

    if (c->save_status == PW_GOOD)
        c->i += 173; /* 171 should do but call it "just to be sure" :) */
}

void pw_rip_ByteKiller30(pw_ctx *c)
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

        c->j = c->l - 36;
        c->j /= 4;
        hdr[20] = hdr[28] = (uint8_t)(c->j >> 24);
        hdr[21] = hdr[29] = (uint8_t)(c->j >> 16);
        hdr[22] = hdr[30] = (uint8_t)(c->j >> 8);
        hdr[23] = hdr[31] = (uint8_t)c->j;
        pw_save_rip_special(c, "ByteKiller 3.0 Exe-file", PWF_ByteKiller, hdr, 32);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "ByteKiller 3.0 Exe-file", PWF_ByteKiller);
    }

    if (c->save_status == PW_GOOD)
        c->i += 36; /* 32 should do but call it "just to be sure" :) */
}

void pw_rip_bytekillerpro10(pw_ctx *c)
{
    /* c->l is still the whole size */
    /* The original filled a 36 byte block but saved 50 bytes of it; the 14
     * bytes past the block are zero here. */
    uint8_t hdr[50];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 50;
        memset(hdr, 0, sizeof hdr);
        hdr[2] = hdr[30] = 0x03;
        hdr[3] = 0xF3;
        hdr[11] = 0x02;
        hdr[19] = 0x01;
        hdr[27] = 0x01;
        hdr[31] = 0xE9;

        c->j = c->l - 60;
        c->j /= 4;
        hdr[20] = hdr[32] = (uint8_t)(c->j >> 24);
        hdr[21] = hdr[33] = (uint8_t)(c->j >> 16);
        hdr[22] = hdr[34] = (uint8_t)(c->j >> 8);
        hdr[23] = hdr[35] = (uint8_t)c->j;

        /* also the last 24 bytes are 'removed' frequently ... Here they are */
        put_exe_tail24(c);

        pw_save_rip_special(c, "ByteKillerPro 1.0 Exe-file", PWF_ByteKiller, hdr, 50);
    }
    else
    {
        c->start -= 50;
        pw_save_rip(c, "ByteKillerPro 1.0 Exe-file", PWF_ByteKiller);
    }

    if (c->save_status == PW_GOOD)
        c->i += 54; /* 51 should do but call it "just to be sure" :) */
}

void pw_rip_BKCloneFLT(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t hdr[36];

    c->out_size = c->l;

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 40;
        memset(hdr, 0, sizeof hdr);
        hdr[2] = hdr[30] = 0x03;
        hdr[3] = 0xF3;
        hdr[11] = 0x02;
        hdr[19] = 0x01;
        hdr[27] = 0x01;
        hdr[31] = 0xE9;

        c->j = c->l - 40;
        c->j /= 4;
        hdr[20] = hdr[32] = (uint8_t)(c->j >> 24);
        hdr[21] = hdr[33] = (uint8_t)(c->j >> 16);
        hdr[22] = hdr[34] = (uint8_t)(c->j >> 8);
        hdr[23] = hdr[35] = (uint8_t)c->j;

        /* also the last 4 bytes could have been 'removed' ... Here they are */
        /* I know, now, it's not safe method ... */
        IN_SET(c->start + c->out_size - 4, 0x00);
        IN_SET(c->start + c->out_size - 3, 0x00);
        IN_SET(c->start + c->out_size - 2, 0x03);
        IN_SET(c->start + c->out_size - 1, 0xF2);

        pw_save_rip_special(c, "ByteKiller Clone FLT Exe-file", PWF_BKCloneFLT, hdr, 36);
    }
    else
    {
        c->start -= 36;
        pw_save_rip(c, "ByteKiller Clone FLT Exe-file", PWF_BKCloneFLT);
    }

    if (c->save_status == PW_GOOD)
        c->i += 37;
}

void pw_rip_BKClone5(pw_ctx *c)
{
    /* c->l is still the whole size */
    uint8_t hdr[32];

    c->out_size = c->l + 36; /* + 36 for header/footer sizes */

    c->convert = PW_BAD;

    if (c->amiga_exe_header == PW_BAD)
    {
        c->out_size -= 32;
        memset(hdr, 0, sizeof hdr);
        hdr[2] = hdr[26] = 0x03;
        hdr[3] = 0xF3;
        hdr[11] = 0x01;
        hdr[27] = 0xE9;

        c->j = c->l - 36;
        c->j /= 4;
        hdr[20] = hdr[28] = (uint8_t)(c->j >> 24);
        hdr[21] = hdr[29] = (uint8_t)(c->j >> 16);
        hdr[22] = hdr[30] = (uint8_t)(c->j >> 8);
        hdr[23] = hdr[31] = (uint8_t)c->j;
        pw_save_rip_special(c, "Unknown ByteKiller Clone 5 Exe-file", PWF_BKClone5, hdr, 32);
    }
    else
    {
        c->start -= 32;
        pw_save_rip(c, "Unknown ByteKiller Clone 5 Exe-file", PWF_BKClone5);
    }

    if (c->save_status == PW_GOOD)
        c->i += 36; /* 32 should do but call it "just to be sure" :) */
}
