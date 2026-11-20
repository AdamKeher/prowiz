/*
 * StartrekkerPack.c - StarTrekker Packer (upstream r/StartrekkerPack.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 *
 * 27 dec 2001 : added some checks to prevent readings outside of
 * the input file.
 */
#include "formats.h"

static uint32_t be32(pw_ctx *c, int64_t a)
{
    return ((uint32_t)IN(a) << 24) + ((uint32_t)IN(a + 1) << 16) + ((uint32_t)IN(a + 2) << 8) + IN(a + 3);
}

int16_t pw_test_STARPACK(pw_ctx *c)
{
    /* test 1 */
    if ((c->i < 23) || ((c->i + 269 - 23) >= (uint32_t)c->in_size))
        return PW_BAD;

    /* test 2 */
    c->start = (int32_t)(c->i - 23);
    c->l = (IN(c->start + 268) * 256) + IN(c->start + 269);
    c->k = c->l / 4;
    if ((c->k * 4) != c->l)
        return PW_BAD;
    if (c->k > 128)
        return PW_BAD;
    if ((c->k == 0) || ((c->start + 784) > c->in_size))
        return PW_BAD;

    if (IN(c->start + 784) != 0)
        return PW_BAD;

    /* test #3  smp size < loop start + loop size ? */
    /* c->l is still the size of the pattern list */
    c->wss = 0;
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->j = (((IN(c->start + 20 + c->k * 8) * 256) + IN(c->start + 21 + c->k * 8)) * 2);
        c->m = (((IN(c->start + 24 + c->k * 8) * 256) + IN(c->start + 25 + c->k * 8)) * 2) +
               (((IN(c->start + 26 + c->k * 8) * 256) + IN(c->start + 27 + c->k * 8)) * 2);
        if ((c->j + 2) < c->m)
            return PW_BAD;
        c->wss += c->j;
    }

    /* test #4  finetunes & volumes */
    /* c->l is still the size of the pattern list */
    for (c->k = 0; c->k < 31; c->k++)
    {
        if ((IN(c->start + 22 + c->k * 8) > 0x0f) || (IN(c->start + 23 + c->k * 8) > 0x40))
            return PW_BAD;
    }

    /* test #5  pattern addresses > sample address ? */
    /* c->l is still the size of the pattern list */
    /* get sample data address */
    if ((c->start + 0x314) > c->in_size)
        return PW_BAD;
    /* c->k gets address of sample data */
    c->k = be32(c, (int64_t)c->start + 784);
    if ((c->k + c->start) > (uint32_t)c->in_size)
        return PW_BAD;
    if (c->k < 788)
        return PW_BAD;
    /* c->k is the address of the sample data */
    /* pattern addresses > sample address ? */
    for (c->j = 0; c->j < c->l; c->j += 4)
    {
        /* c->m gets each pattern address */
        c->m = be32(c, (uint32_t)(c->start + 272 + c->j));
        if (c->m > c->k)
            return PW_BAD;
    }
    /* test last patterns of the pattern list == 0 ? */
    c->j += 2;
    while (c->j < 128)
    {
        c->m = be32(c, (uint32_t)(c->start + 272 + c->j * 4));
        if (c->m != 0)
            return PW_BAD;
        c->j += 1;
    }

    /* test pattern data */
    /* c->k is the address of the sample data */
    c->j = c->start + 788;
    /* c->j points on pattern data */
    while (c->j < (c->k + c->start - 4))
    {
        if (IN(c->j) == 0x80)
        {
            c->j += 1;
            continue;
        }
        if (IN(c->j) > 0x80)
            return PW_BAD;
        /* empty row ? ... not possible ! */
        if ((IN(c->j) == 0x00) &&
            (IN(c->j + 1) == 0x00) &&
            (IN(c->j + 2) == 0x00) &&
            (IN(c->j + 3) == 0x00))
            return PW_BAD;
        /* fx = C .. arg > 64 ? (sic: '*' instead of '&') */
        if (((IN(c->j + 2) * 0x0f) == 0x0C) && (IN(c->j + 3) > 0x40))
            return PW_BAD;
        /* fx = D .. arg > 64 ? */
        if (((IN(c->j + 2) * 0x0f) == 0x0D) && (IN(c->j + 3) > 0x40))
            return PW_BAD;
        c->j += 4;
    }

    return PW_GOOD;
}

void pw_rip_STARPACK(pw_ctx *c)
{
    /* c->k is still the sample data address */
    /* c->wss is the whole sample size already */
    c->out_size = c->wss + c->k + 0x314;

    c->convert = PW_GOOD;
    pw_save_rip(c, "StarTrekker Packer module", PWF_Star_pack);

    if (c->save_status == PW_GOOD)
        c->i += 24; /* 23 after 1st vol */
}

/*
 *  StarTrekker _Packer.c   1997 (c) Asle / ReDoX
 *
 * Converts back to ptk StarTrekker packed MODs
 */

/* The original converted the patterns into a 65536 byte heap block and wrote
 * c1*1024 bytes of it (c1 <= 255), reading past its end what it had written
 * there. Keep enough for that; later writes are never read back. */
#define STARPACK_PAT_BUF (256 * 1024)

void pw_depack_STARPACK(pw_ctx *c)
{
    uint8_t  c1 = 0x00, c2 = 0x00;
    uint8_t  pat_pos;
    uint8_t  whatever[1024];
    uint8_t *pattern;
    int32_t  i = 0, j = 0;
    int32_t  total_sample_size = 0;
    int32_t  pats_address[128];
    int32_t  read_pats_address[128];
    int32_t  sample_data_address = 0;
    int32_t  where = c->start;
    int32_t  max_pat_addy = 0;
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    memset(pats_address, 0, sizeof pats_address);
    memset(read_pats_address, 0, sizeof read_pats_address);

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(whatever, 0, sizeof whatever);

    /* read and write title */
    pw_fwrite_in(out, where, 20);
    where += 20;

    /* read and write sample descriptions */
    for (i = 0; i < 31; i++)
    {
        /* sample name */
        pw_fwrite(out, &whatever[0], 22);

        total_sample_size += (((IN(where + i * 8) * 256) + IN(where + 1 + i * 8)) * 2);
        pw_fwrite_in(out, where + i * 8, 8);
    }
    where = c->start + 268;

    /* read size of pattern table */
    pat_pos = (uint8_t)(((IN(where) * 256) + IN(where + 1)) / 4);
    where += 4;
    pw_fwrite(out, &pat_pos, 1);
    /* bypass $0000 unknown bytes */

    for (i = 0; i < 128; i++)
    {
        pats_address[i] = (int32_t)be32(c, where);
        where += 4;
        if (pats_address[i] > max_pat_addy)
            max_pat_addy = pats_address[i];
    }

    /* write noisetracker byte */
    c1 = 0x7f;
    pw_fwrite(out, &c1, 1);

    /* read sample data address */
    where = c->start + 0x310;
    sample_data_address = (int32_t)(be32(c, where) + 0x314);

    /* pattern data */
    where += 4;

    pattern = (uint8_t *)calloc(STARPACK_PAT_BUF, 1);
    if (pattern == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        pw_fdiscard(out);
        return;
    }

#define PAT_SET(x, v)                                  \
    do                                                 \
    {                                                  \
        int32_t px_ = (x);                             \
        if (px_ >= 0 && px_ < STARPACK_PAT_BUF)        \
            pattern[px_] = (uint8_t)(v);               \
    } while (0)
#define PAT_GET(x) (((x) >= 0 && (x) < STARPACK_PAT_BUF) ? pattern[(x)] : 0)

    c1 = 0; /* will count patterns */
    i = 0;
    /* (terminates: j grows by 1 or 4 per turn and is checked against
     * MaxPatAddy every 256 turns; at most about 2^29 turns) */
    for (j = 0; j < (int32_t)((uint32_t)max_pat_addy + 0x400); j += 4)
    {
        if ((i % 1024) == 0)
        {
            if (j > max_pat_addy)
                break;
            /* Read_Pats_Address[] has 128 entries; c1 counts up to 255 */
            if (c1 < 128)
                read_pats_address[c1] = j;
            c1 += 0x01;
        }
        if (IN(where + j) == 0x80)
        {
            j -= 3;
            i += 4;
            continue;
        }

        c2 = (uint8_t)(((IN(where + j)) | ((IN(where + j + 2) >> 4) & 0x0f)) / 4);

        PAT_SET(i, IN(where + j) & 0x0f);
        PAT_SET(i, PAT_GET(i) | (c2 & 0xf0));
        PAT_SET(i + 1, IN(where + j + 1));
        PAT_SET(i + 2, IN(where + j + 2) & 0x0f);
        PAT_SET(i + 2, PAT_GET(i + 2) | ((c2 << 4) & 0xf0));
        PAT_SET(i + 3, IN(where + j + 3));

        i += 4;
    }

    /* write pattern list */
    /* write pattern table */
    memset(whatever, 0, 128);
    for (c2 = 0; c2 < 128; c2 += 0x01)
        for (i = 0; i < 128; i++)
            if (pats_address[c2] == read_pats_address[i])
            {
                whatever[c2] = (uint8_t)i;
                break;
            }
    pw_fwrite(out, &whatever[0], 128);

    /* write ptk's ID */
    whatever[0] = 'M';
    whatever[1] = '.';
    whatever[2] = 'K';
    pw_fwrite(out, &whatever[0], 1);
    pw_fwrite(out, &whatever[1], 1);
    pw_fwrite(out, &whatever[2], 1);
    pw_fwrite(out, &whatever[1], 1);

    /* write pattern data */
    /* c1 is still the number of patterns stored */
    pw_fwrite(out, pattern, c1 * 1024);
    free(pattern);

#undef PAT_SET
#undef PAT_GET

    /* sample data */
    where = c->start + sample_data_address;

    pw_fwrite_in(out, where, total_sample_size);

    pw_crap(" StarTrekker Pack ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
