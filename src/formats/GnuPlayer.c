/*
 * GnuPlayer.c - GnuPlayer ("GnPl"), based on XtC's description
 *               (upstream r/GnuPlayer.c, 22 mar 2003).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "formats.h"

int16_t pw_test_GnuPlayer(pw_ctx *c)
{
    /* test #1 */
    if (c->i < 0x92)
        return PW_BAD;

    /* test #2 smp size and loop start */
    c->wss = 0;
    c->o = 0; /* will hold the number of non-null samples */
    c->start = (int32_t)(c->i - 0x92);
    for (c->k = 0; c->k < 31; c->k++)
    {
        c->j = (IN(c->start + 20 + c->k * 4) * 256) + IN(c->start + 21 + c->k * 4);
        c->l = (IN(c->start + 22 + c->k * 4) * 256) + IN(c->start + 23 + c->k * 4);
        if (c->l > (c->j + 1))
            return PW_BAD;
        if (c->j != 0)
            c->o += 1;
        c->wss += c->j;
    }

    return PW_GOOD;
}

void pw_rip_GnuPlayer(pw_ctx *c)
{
    /* c->wss is the whole sample size */
    /* but it seems to be a fake sample values */

    /* size of 1st track + header */
    c->k = (IN(c->start + 0x96) * 256) + IN(c->start + 0x97) + 0x96;
    /* size of 2nd track (read from the start of the input, as upstream) */
    c->j = (IN(c->k) * 256) + IN(c->k + 1) + c->k;
    /* c->j points now to the first sample size */

    /* real sample sizes */
    c->m = 0;
    while (c->m < c->o)
    {
        c->k = (IN(c->start + c->j) * 256) + IN(c->start + c->j + 1);
        c->j += (c->k + 2);
        c->m += 1;
    }

    c->out_size = c->j;

    c->convert = PW_GOOD;
    pw_save_rip(c, "GnuPlayer module", PWF_GnuPlayer);

    if (c->save_status == PW_GOOD)
        c->i += 0x96;
}

/*
 * The original decodes both track pairs into a 64 KiB buffer, but "bypass
 * rows" commands move the write position without limit, so long tracks wrote
 * past it and the converted file then reads those bytes back. Here the buffer
 * grows instead (zero-filled), up to a cap; beyond the cap writes are dropped
 * and reads give 0.
 */
#define GNU_PATTERN_MIN 65536
#define GNU_PATTERN_CAP (16u * 1024u * 1024u)

typedef struct
{
    uint8_t *p;
    size_t   size;
} gnu_buf;

static uint8_t gnu_get(const gnu_buf *b, int32_t idx)
{
    return (idx >= 0 && (size_t)idx < b->size) ? b->p[idx] : 0;
}

static void gnu_set(gnu_buf *b, int32_t idx, uint8_t v)
{
    size_t   n;
    uint8_t *q;

    if (idx < 0 || (size_t)idx >= GNU_PATTERN_CAP)
        return;
    if ((size_t)idx >= b->size)
    {
        n = b->size;
        while (n <= (size_t)idx)
            n *= 2;
        if (n > GNU_PATTERN_CAP)
            n = GNU_PATTERN_CAP;
        q = (uint8_t *)realloc(b->p, n);
        if (q == NULL)
            return;
        memset(q + b->size, 0, n - b->size);
        b->p = q;
        b->size = n;
    }
    b->p[idx] = v;
}

/* one track pair: 'base' is 0 for voices 1-2 and 8 for voices 3-4 */
static int32_t gnu_track(pw_ctx *c, gnu_buf *pattern, int32_t where, int32_t siz_of_track,
                         int32_t base, int16_t *sfx_nbr, int add_for_a)
{
    int32_t i, l = 0;
    uint8_t arg;

    for (i = 0; i < siz_of_track; i += 2)
    {
        arg = IN(where + 1 + i);
        switch (IN(where + i))
        {
        case 0: /* track end */
            i = siz_of_track;
            break;
        case 1: /* set volume */
            *sfx_nbr += 1;
            gnu_set(pattern, l + base + 2, gnu_get(pattern, l + base + 2) | 0x0C);
            gnu_set(pattern, l + base + 3, arg);
            gnu_set(pattern, l + base + 6, gnu_get(pattern, l + base + 6) | 0x0C);
            gnu_set(pattern, l + base + 7, arg);
            break;
        case 2: /* same a A */
            *sfx_nbr += 1;
            /* voices 1-2 add the effect number, voices 3-4 or it in */
            if (add_for_a)
            {
                gnu_set(pattern, l + base + 2, (uint8_t)(gnu_get(pattern, l + base + 2) + 0x0A));
                gnu_set(pattern, l + base + 3, arg);
                gnu_set(pattern, l + base + 6, (uint8_t)(gnu_get(pattern, l + base + 6) + 0x0A));
                gnu_set(pattern, l + base + 7, arg);
            }
            else
            {
                gnu_set(pattern, l + base + 2, gnu_get(pattern, l + base + 2) | 0x0A);
                gnu_set(pattern, l + base + 3, arg);
                gnu_set(pattern, l + base + 6, gnu_get(pattern, l + base + 6) | 0x0A);
                gnu_set(pattern, l + base + 7, arg);
            }
            break;
        case 3: /* set speed */
            *sfx_nbr += 1;
            if (*sfx_nbr == 1)
            {
                gnu_set(pattern, l + base + 2, gnu_get(pattern, l + base + 2) | 0x0f);
                gnu_set(pattern, l + base + 3, arg);
            }
            else /* if sfx_nbr == 3, I'm in trouble :( */
            {
                gnu_set(pattern, l + base + 6, gnu_get(pattern, l + base + 6) | 0x0f);
                gnu_set(pattern, l + base + 7, arg);
            }
            break;
        case 4: /* bypass rows */
            l += (arg * 16);
            *sfx_nbr = 0;
            break;
        case 5: /* set note */
            gnu_set(pattern, l + base, (uint8_t)(arg & 0xf0));
            gnu_set(pattern, l + base, gnu_get(pattern, l + base) | IN(c->start + 0x90));
            gnu_set(pattern, l + base + 1, IN(c->start + 0x91));
            gnu_set(pattern, l + base + 4, (uint8_t)(arg & 0xf0));
            gnu_set(pattern, l + base + 4, gnu_get(pattern, l + base + 4) | IN(c->start + 0x90));
            gnu_set(pattern, l + base + 5, IN(c->start + 0x91));
            gnu_set(pattern, l + base + 2, (uint8_t)(gnu_get(pattern, l + base + 2) | (arg << 4)));
            gnu_set(pattern, l + base + 6, (uint8_t)(gnu_get(pattern, l + base + 6) | (arg << 4)));
            break;
        default:
            pw_printf(c, "\nunsupported case in Depack_GnuPlayer(). Please send this file to \"asle@free.fr\" :)\n");
            break;
        }
    }
    return l;
}

/*
 *   GnuPlayer.c   2003 (c) Asle@free.fr
 */
#define MAXI(a, b) ((a) > (b) ? (a) : (b))

/* The original allocated 65436 bytes for a sample (a typo for 65536) and
 * cleared 65536 per sample, but a sample of j words decodes into indices
 * 0..2*j+1 (up to 131071) and up to 131070 bytes of it are written out, so
 * the buffer holds all of that. Bytes past 64 KiB are not cleared between
 * samples, as upstream. */
#define GNU_SAMPLE_BUF 131072

void pw_depack_GnuPlayer(pw_ctx *c)
{
    uint8_t  whatever[2048];
    uint8_t *smp;
    gnu_buf  pattern;
    int32_t  i = 0, j = 0, k = 0, l = 0;
    int32_t  where = c->start;
    pw_file *out;
    int32_t  siz_of_track, len1, len2;
    int32_t  smp_sizes[31];
    int32_t  nbr_smp = 0;
    int16_t  sfx_nbr = 0;

    if (c->save_status == PW_BAD)
        return;

    snprintf(c->depacked_name, sizeof c->depacked_name, "%d.mod", c->file_counter - 1);
    out = pw_fopen(c, c->depacked_name);

    /* read and write title */
    pw_fwrite_in(out, where, 20);

    /* take care of pattern right now */
    pattern.size = GNU_PATTERN_MIN;
    pattern.p = (uint8_t *)calloc(pattern.size, 1);
    smp = (uint8_t *)calloc(GNU_SAMPLE_BUF, 1);
    if (pattern.p == NULL || smp == NULL)
    {
        free(pattern.p);
        free(smp);
        c->fatal = PW_ERR_NOMEM;
        pw_fdiscard(out);
        return;
    }
    where += 0x96;
    /* track 1 & 2 */
    siz_of_track = (IN(where) * 256) + IN(where + 1); /* size of track 1 */
    where += 2;
    l = gnu_track(c, &pattern, where, siz_of_track, 0, &sfx_nbr, 1);
    len1 = l / 1024;

    /* track 3 & 4 */
    where += (siz_of_track - 2);
    siz_of_track = (IN(where) * 256) + IN(where + 1); /* size of track 2 */
    where += 2;
    l = gnu_track(c, &pattern, where, siz_of_track, 8, &sfx_nbr, 0);
    len2 = l / 1024;
    where += (siz_of_track - 2);

    /* sample header stuff */
    memset(whatever, 0, sizeof whatever);
    /* get nbr of non-null samples */
    l = 0;
    for (i = 0; i < 31; i++)
    {
        k = (IN(c->start + 20 + (i * 4)) * 256) + IN(c->start + 21 + (i * 4));
        if (k != 0)
            nbr_smp += 1;
        smp_sizes[i] = k * 2;
        l += smp_sizes[i];

        whatever[22 + (i * 30)] = IN(c->start + 20 + (i * 4));
        whatever[23 + (i * 30)] = IN(c->start + 21 + (i * 4));
        whatever[25 + (i * 30)] = 0x40;
        whatever[26 + (i * 30)] = IN(c->start + 22 + (i * 4));
        whatever[27 + (i * 30)] = IN(c->start + 23 + (i * 4));
        whatever[29 + (i * 30)] = 0x01;
    }
    k = MAXI(len1, len2);
    whatever[930] = (uint8_t)k;
    whatever[931] = 0x7f;
    /* the original wrote on past its 2048 byte buffer; only 1064 bytes are kept */
    for (i = 0; i < k && i + 932 < (int32_t)sizeof whatever; i++)
        whatever[i + 932] = (uint8_t)i;
    whatever[1060] = 'M';
    whatever[1061] = '.';
    whatever[1062] = 'K';
    whatever[1063] = '.';
    pw_fwrite(out, whatever, 1064);

    /* write patterns */
    /* k being the max of both lengths of tracks */
    if ((size_t)k * 1024 <= pattern.size)
        pw_fwrite(out, pattern.p, (int64_t)k * 1024);
    else
    {
        pw_fwrite(out, pattern.p, (int64_t)pattern.size);
        pw_fzero(out, (int64_t)k * 1024 - (int64_t)pattern.size);
    }
    free(pattern.p);

    /* sample stuff */
    l = 0;
    for (i = 0; i < nbr_smp; i++)
    {
        int32_t out_end;
        int8_t  samp;
        k = 0;
        /* the original cleared only the first 64 KiB of its sample buffer */
        memset(smp, 0, 65536);
        j = (IN(where) * 256) + IN(where + 1);
        where += 2;
        out_end = (j * 2);
        l += smp_sizes[i];
        smp[k++] = IN(where++);
        while (k < out_end) /* k stays below GNU_SAMPLE_BUF: out_end <= 131070 */
        {
            samp = (int8_t)((IN(where) >> 4) & 0x0f);
            if (samp & 0x08)
                samp = (int8_t)(samp - 0x10);
            k += 1;
            smp[k] = (uint8_t)(smp[k - 1] + samp);
            samp = (int8_t)(IN(where) & 0x0f);
            if (samp & 0x08)
                samp = (int8_t)(samp - 0x10);
            k += 1;
            smp[k] = (uint8_t)(smp[k - 1] + samp);
            where += 1;
        }
        where -= 1;
        pw_fwrite(out, smp, smp_sizes[i]);
    }
    free(smp);

    /* crap */
    pw_crap("    GnuPlayer     ", PW_BAD, PW_BAD, out);

    pw_fclose(out);

    pw_printf(c, "done\n");
}
