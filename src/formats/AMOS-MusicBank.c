/*
 * AMOS-MusicBank.c - AMOS music bank "AmBk" (upstream r/AMOS-MusicBank.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

#define AMBK_HEADER_SIZE 1084

int16_t pw_test_AmBk(pw_ctx *c)
{
    if (c->i + 68 > (uint32_t)c->in_size)
        return PW_BAD;

    /* test #1  */
    c->start = (int32_t)c->i;
    if ((IN(c->start + 4) != 0x00) ||
        (IN(c->start + 5) != 0x03) ||
        (IN(c->start + 6) != 0x00) ||
        (IN(c->start + 7) > 0x01) ||
        (IN(c->start + 12) != 'M') ||
        (IN(c->start + 13) != 'u') ||
        (IN(c->start + 14) != 's') ||
        (IN(c->start + 15) != 'i') ||
        (IN(c->start + 16) != 'c') ||
        (IN(c->start + 17) != ' ') ||
        (IN(c->start + 18) != ' ') ||
        (IN(c->start + 19) != ' '))
        return PW_BAD;

    /* get the whole size */
    c->k = (IN(c->start + 9) * 256 * 256) + (IN(c->start + 10) * 256) + IN(c->start + 11) + 12;
    if (c->k + c->start > (uint32_t)c->in_size)
        return PW_BAD;

    return PW_GOOD;
}

void pw_rip_AmBk(pw_ctx *c)
{
    /* c->k is already the whole file size */

    c->out_size = c->k;

    c->convert = PW_GOOD;
    pw_save_rip(c, "AMOS Bank (AmBk)", PWF_AmBk);

    if (c->save_status == PW_GOOD)
        c->i += 4;
}

/* Header[] is a 1084 byte heap block upstream; the sample loop below can
 * index far past it with a large sample count. Only the first 1084 bytes are
 * ever written to the file, so writes past it are dropped. */
static void ambk_hset(uint8_t *header, int32_t idx, uint8_t v)
{
    if (idx >= 0 && idx < AMBK_HEADER_SIZE)
        header[idx] = v;
}

/*
 *   ambk.c   mar 2003 (c) Asle / ReDoX
 *
 *  based in Kyz description ... thx !.
 *
 */
void pw_depack_AmBk(pw_ctx *c)
{
    uint8_t  header[AMBK_HEADER_SIZE];
    int32_t  i, j, k;
    int32_t  where = c->start;
    int32_t  inst_hdata_addy, songs_data_addy;
    int32_t  smps_addys[31];
    pw_file *out;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    memset(header, 0, sizeof header);
    memset(smps_addys, 0, sizeof smps_addys);

    /* bank length (+0x09) and pattern data address (+0x1C) are not used */
    inst_hdata_addy = ((IN(where + 0x14) * 256 * 256 * 256) +
                       (IN(where + 0x15) * 256 * 256) +
                       (IN(where + 0x16) * 256) +
                       IN(where + 0x17)) + 0x14;
    songs_data_addy = ((IN(where + 0x18) * 256 * 256 * 256) +
                       (IN(where + 0x19) * 256 * 256) +
                       (IN(where + 0x1A) * 256) +
                       IN(where + 0x1B)) + 0x14;

    /* title stuff */
    where = c->start + songs_data_addy;
    j = IN(where) * 256 + IN(where + 1); /* number of songs */
    if (j > 1)
    {
        pw_printf(c, "\n!!! unsupported feature in depack_AmBk() - send this file to asle@free.fr !\n");
        /* upstream returns without writing or closing the file: it is left
         * on disk empty */
        pw_fclose(out);
        return;
    }
    j = ((IN(where + 2) * 256 * 256 * 256) + (IN(where + 3) * 256 * 256) + (IN(where + 4) * 256) + IN(where + 5));
    for (i = 0; i < 16; i++)
        header[i] = IN(where + j + 0x0c + i);

    where = c->start + inst_hdata_addy;

    /* samples header + data */
    j = (IN(where) * 256) + IN(where + 1); /* nbr of samples */
    where += 2;
    for (i = 0; i < j; i++)
    {
        int a;
        /* smps_addys[] (and the write-only smp_sizes[]) have 31 entries
         * upstream but the sample count is a 16 bit value from the file:
         * past entry 30 the write is dropped and the read gives 0 (upstream
         * overwrote its stack there). */
        if (i < 31)
            smps_addys[i] = ((IN(where) * 256 * 256 * 256) + (IN(where + 1) * 256 * 256) + (IN(where + 2) * 256) + IN(where + 3));
        /* sample name */
        for (a = 0; a < 16; a++)
            ambk_hset(header, 20 + (i * 30) + a, IN(where + 16 + a));

        /* size */
        k = 0x0e;
        if ((IN(where + 0x0e) == 0x00) && (IN(where + 0x0f) == 0x00))
            k = 0x08;

        ambk_hset(header, 42 + (i * 30), IN(where + k));
        ambk_hset(header, 43 + (i * 30), IN(where + k + 1));

        /* fine + vol */
        ambk_hset(header, 44 + (i * 30), IN(where + 0x0c));
        ambk_hset(header, 45 + (i * 30), IN(where + 0x0d));
        /* loop */
        k = (IN(where + 0x05) * 256 * 256) + (IN(where + 0x06) * 256) + IN(where + 0x07);
        if (k >= ((i < 31) ? smps_addys[i] : 0))
        {
            k -= ((i < 31) ? smps_addys[i] : 0);
            k /= 2;
            /* upstream: bytes 1 and 0 of k in memory (little endian) */
            ambk_hset(header, 46 + (i * 30), (uint8_t)((uint32_t)k >> 8));
            ambk_hset(header, 47 + (i * 30), (uint8_t)k);
        }

        /* loop size */
        if ((IN(where + 0x0a) == 0x00) && (IN(where + 0x0b) <= 0x02))
        {
            ambk_hset(header, 49 + (i * 30), 0x01);
        }
        else
        {
            ambk_hset(header, 48 + (i * 30), IN(where + 0x0a));
            ambk_hset(header, 49 + (i * 30), IN(where + 0x0b));
        }

        where += 32;
    }
    /* end of sample header */

    pw_fwrite(out, header, AMBK_HEADER_SIZE);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
