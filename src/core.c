/*
 * core.c - library entry points, logging and the "_types_" extension table.
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "pw_internal.h"

void pw_printf(pw_ctx *c, const char *fmt, ...)
{
    char    small[512];
    char   *buf = small;
    va_list ap;
    int     n;

    if (c->cb.log == NULL)
        return;
    va_start(ap, fmt);
    n = vsnprintf(small, sizeof small, fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    if (n >= (int)sizeof small)
    {
        buf = (char *)malloc((size_t)n + 1);
        if (buf == NULL)
            return;
        va_start(ap, fmt);
        vsnprintf(buf, (size_t)n + 1, fmt, ap);
        va_end(ap);
    }
    c->cb.log(c->cb.user, buf);
    if (buf != small)
        free(buf);
}

/* ------------------------------------------- bytes outside the input */

static uint32_t outside_slot(const pw_outside *t, int64_t off)
{
    uint64_t h = (uint64_t)off * 0x9E3779B97F4A7C15ull;
    uint32_t mask = t->cap - 1;
    uint32_t s = (uint32_t)(h >> 32) & mask;
    while (t->key[s] != 0)
    {
        if (t->key[s] == off)
            return s;
        s = (s + 1) & mask;
    }
    return s; /* empty slot */
}

/* a slot is empty when key == 0 (offset 0 is always inside
 * the input, so key 0 never names a real outside byte) */
uint8_t pw_outside_get(const pw_ctx *c, int64_t off)
{
    const pw_outside *t = &c->outside;
    uint32_t s;
    if (t->cap == 0)
        return 0;
    s = outside_slot(t, off);
    return t->key[s] == off ? t->val[s] : 0;
}

static int outside_grow(pw_outside *t)
{
    pw_outside n;
    uint32_t   i;

    n.cap = t->cap ? t->cap * 2 : 256;
    n.used = 0;
    n.key = (int64_t *)calloc(n.cap, sizeof *n.key);
    n.val = (uint8_t *)calloc(n.cap, 1);
    if (n.key == NULL || n.val == NULL)
    {
        free(n.key);
        free(n.val);
        return 0;
    }
    for (i = 0; i < t->cap; i++)
    {
        if (t->key[i] != 0)
        {
            uint32_t s = outside_slot(&n, t->key[i]);
            n.key[s] = t->key[i];
            n.val[s] = t->val[i];
            n.used++;
        }
    }
    free(t->key);
    free(t->val);
    *t = n;
    return 1;
}

void pw_outside_set(pw_ctx *c, int64_t off, uint8_t v)
{
    pw_outside *t = &c->outside;
    uint32_t    s;

    if (off == 0)
        return; /* cannot happen: offset 0 is inside any scanned input */
    if (t->cap != 0)
    {
        s = outside_slot(t, off);
        if (t->key[s] == off)
        {
            t->val[s] = v;
            return;
        }
    }
    if (v == 0)
        return; /* unwritten outside bytes already read as 0 */
    if (t->used >= PW_OUTSIDE_MAX)
        return; /* give up remembering; reads give 0 */
    if ((t->used + 1) * 2 > t->cap && !outside_grow(t))
        return;
    s = outside_slot(t, off);
    t->key[s] = off;
    t->val[s] = v;
    t->used++;
}

/* Support_Types_FileDefault() */
void pw_types_default(pw_ctx *c)
{
    int i;
    for (i = 0; i <= PWF_COUNT; i++)
        strcpy(PW_EXT(c, i), pw_default_ext[i]);
}

/*
 * Support_Types() reading an existing "_types_" file: non-comment lines give
 * the extension of format 0, 1, 2 ... in order. This keeps the original's
 * quirks: it stops when one byte is left, a line is read 255 bytes at a time,
 * and an overlong line spills into the following rows of the table.
 */
void pw_types_load(pw_ctx *c, const uint8_t *data, size_t size, int report)
{
    char  *flat = c->ext_flat;
    size_t flat_size = PW_EXT_TABLE;
    size_t pos = 0;
    uint32_t row = 0;
    char line[PW_TYPES_LINE_LENGTH];

    memset(c->ext_flat, 0, sizeof c->ext_flat);

    while ((int64_t)pos + 1 < (int64_t)(int32_t)size)
    {
        size_t len = 0, cpt;

        /* fgets(line, 256, f) */
        memset(line, 0, sizeof line);
        while (len < PW_TYPES_LINE_LENGTH - 1 && pos < size)
        {
            char ch = (char)data[pos++];
            line[len++] = ch;
            if (ch == '\n')
                break;
        }
        if (line[0] == '#')
            continue;
        cpt = 0;
        while (line[cpt] != 0x00 && line[cpt] != 0x0a && line[cpt] != 0x0d)
        {
            size_t at = (size_t)row * PW_EXT_LEN + cpt;
            if (at < flat_size)
                flat[at] = line[cpt];
            cpt += 1;
        }
        row += 1;
        if (row == PWF_COUNT)
            break;
    }

    if (row != PWF_COUNT)
    {
        if (report)
            pw_printf(c, "!!! Damaged \"%s\" file. Missing up %u extensions definitions\n",
                      "_types_", (uint32_t)PWF_COUNT - (row + 1));
        pw_types_default(c);
    }
}

int pw_types_report(const uint8_t *data, size_t size, const pw_callbacks *cb)
{
    pw_ctx *c = (pw_ctx *)calloc(1, sizeof *c);
    int     n = 0, i;

    if (c == NULL)
        return PW_ERR_NOMEM;
    if (cb != NULL)
        c->cb = *cb;
    pw_types_load(c, data, size, 1);
    for (i = 0; i < PWF_COUNT; i++)
        if (strcmp(PW_EXT(c, i), pw_default_ext[i]) != 0)
            n++;
    free(c);
    return n;
}

int pw_scan(const uint8_t *data, size_t size, const pw_options *opt,
            const pw_callbacks *cb, pw_result *result)
{
    pw_ctx *c;
    int     rc;

    if (result != NULL)
        memset(result, 0, sizeof *result);
    if (data == NULL && size != 0)
        return PW_ERR_ARG;
    /* the original stored the size in an int32_t */
    if (size > (size_t)INT32_MAX)
        return PW_ERR_ARG;

    c = (pw_ctx *)calloc(1, sizeof *c);
    if (c == NULL)
        return PW_ERR_NOMEM;
    if (cb != NULL)
        c->cb = *cb;
    c->modes = (opt != NULL && opt->modes != 0) ? (opt->modes & PW_MODE_ALL) : PW_MODE_ALL;
    c->new_formats = !(opt != NULL && opt->formats == PW_FORMATS_170A);
    c->save_status = PW_GOOD;
    c->convert = PW_BAD;
    c->amiga_exe_header = PW_GOOD;

    if (opt != NULL && opt->types_data != NULL)
        pw_types_load(c, opt->types_data, opt->types_size, 0);
    else
        pw_types_default(c);

    c->in_size = (int32_t)size;
    if (size >= PW_MIN_FILE_LENGTH)
    {
        c->in = (uint8_t *)malloc(size);
        if (c->in == NULL)
        {
            free(c);
            return PW_ERR_NOMEM;
        }
        memcpy(c->in, data, size);
        pw_scan_run(c);
    }

    rc = c->fatal;
    if (result != NULL)
        *result = c->result;
    free(c->in);
    free(c->outside.key);
    free(c->outside.val);
    free(c->last_rip);
    free(c->claimed);
    free(c);
    return rc;
}

int pw_format_count(void)
{
    return PWF_ALL_COUNT;
}

int pw_format_count_170a(void)
{
    return PWF_COUNT;
}

const char *pw_format_ext(int format_id)
{
    if (format_id < 0 || format_id >= PWF_ALL_COUNT)
        return NULL;
    if (format_id >= PWF_COUNT)
        return pw_new_ext[format_id - PWF_COUNT];
    return pw_default_ext[format_id];
}

pw_kind pw_format_kind(int format_id)
{
    if (format_id < 0 || format_id >= PWF_ALL_COUNT)
        return PW_KIND_MUSIC;
    return pw_kind_of(format_id);
}

const char *pw_version(void)
{
    return PROWIZ_VERSION;
}
