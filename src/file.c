/*
 * file.c - in-memory emulation of the stdio files the original engine wrote.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "pw_internal.h"

struct pw_file
{
    pw_ctx  *c;
    char     name[PW_NAME_MAX];
    uint8_t *data;
    size_t   size;     /* logical file size                 */
    size_t   cap;
    int64_t  pos;      /* current position (may be > size)  */
    int      readonly; /* re-read of the rip file           */
    int      clipped;  /* a write was dropped (size cap)  */
    int64_t  since_seek; /* bytes written since open/seek   */
};

/*
 * A write the original could not have completed: a garbage size of hundreds
 * of megabytes or more (often a negative int passed to fwrite). The Windows
 * C library the original was built with buffers files in 4096 byte blocks:
 * it first fills the rest of its buffer from the source, flushes it, and
 * only then fails on the direct write of the remainder. Writes after that
 * work normally. Returns how many bytes of such a write still reach the
 * file. (The buffer is empty right after opening or seeking.)
 */
#define PW_STDIO_BUF 4096
static int64_t huge_write_prefix(const pw_file *f)
{
    int64_t fill = f->since_seek % PW_STDIO_BUF;
    return fill ? PW_STDIO_BUF - fill : 0;
}

/* negative sizes count: the original passed them to fwrite() as size_t */
static int is_huge_write(const pw_file *f, int64_t n)
{
    return n < 0 || n > (int64_t)PW_MAX_OUTPUT_FILE || f->pos + n > (int64_t)PW_MAX_OUTPUT_FILE;
}

static pw_file *file_new(pw_ctx *c, const char *name)
{
    pw_file *f = (pw_file *)calloc(1, sizeof *f);
    if (f == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        return NULL;
    }
    f->c = c;
    snprintf(f->name, sizeof f->name, "%s", name);
    return f;
}

pw_file *pw_fopen(pw_ctx *c, const char *name)
{
    return file_new(c, name);
}

pw_file *pw_fopen_last_rip(pw_ctx *c)
{
    pw_file *f = file_new(c, c->out_name);
    if (f == NULL)
        return NULL;
    f->readonly = 1;
    if (c->last_rip_size > 0)
    {
        f->data = (uint8_t *)malloc(c->last_rip_size);
        if (f->data == NULL)
        {
            c->fatal = PW_ERR_NOMEM;
            free(f);
            return NULL;
        }
        memcpy(f->data, c->last_rip, c->last_rip_size);
        f->size = f->cap = c->last_rip_size;
    }
    return f;
}

/* make room for writing 'n' bytes at the current position */
static int file_reserve(pw_file *f, int64_t n)
{
    int64_t end;
    size_t  need, cap;
    uint8_t *p;

    if (f->readonly || n <= 0 || f->c->fatal)
        return 0;
    end = f->pos + n;
    if (f->pos < 0 || end > (int64_t)PW_MAX_OUTPUT_FILE)
    {
        /* An absurd size from garbage data: drop this one write (as if it
         * failed) and keep the file usable for the writes that follow. */
        f->clipped = 1;
        return 0;
    }
    need = (size_t)end;
    if (need > f->cap)
    {
        cap = f->cap ? f->cap : 4096;
        while (cap < need)
            cap *= 2;
        p = (uint8_t *)realloc(f->data, cap);
        if (p == NULL)
        {
            f->c->fatal = PW_ERR_NOMEM;
            f->clipped = 1;
            return 0;
        }
        f->data = p;
        f->cap = cap;
    }
    /* writing past the end: the gap reads as zeros, like a real file */
    if ((size_t)f->pos > f->size)
        memset(f->data + f->size, 0, (size_t)f->pos - f->size);
    return 1;
}

static void file_advance(pw_file *f, int64_t n)
{
    f->pos += n;
    f->since_seek += n;
    if ((size_t)f->pos > f->size)
        f->size = (size_t)f->pos;
}

/* the part of an impossible write that still reaches the file */
static int64_t huge_write_kept(pw_file *f)
{
    int64_t p = huge_write_prefix(f);
    f->clipped = 1;
    return (p > 0 && !is_huge_write(f, p)) ? p : 0;
}

void pw_fwrite(pw_file *f, const void *src, int64_t n)
{
    if (f != NULL && is_huge_write(f, n))
    {
        /* the source buffer is not that large; zeros stand in for
         * whatever lay beyond it */
        pw_fzero(f, huge_write_kept(f));
        return;
    }
    if (f == NULL || !file_reserve(f, n))
        return;
    memcpy(f->data + f->pos, src, (size_t)n);
    file_advance(f, n);
}

void pw_fwrite_in(pw_file *f, int64_t off, int64_t n)
{
    const pw_ctx *c;
    uint8_t *dst;
    int64_t  k, lo, hi;

    if (f != NULL && is_huge_write(f, n))
    {
        pw_fwrite_in(f, off, huge_write_kept(f));
        return;
    }
    if (f == NULL || !file_reserve(f, n))
        return;
    c = f->c;
    dst = f->data + f->pos;
    if (c->outside.used)
    {
        /* bytes were stored outside the input: take every byte from IN() */
        for (k = 0; k < n; k++)
            dst[k] = pw_in8(c, off + k);
        file_advance(f, n);
        return;
    }
    /* copy the part inside the input, zero the rest */
    lo = off < 0 ? 0 : off;
    hi = off + n;
    if (hi > (int64_t)c->in_size)
        hi = c->in_size;
    if (lo >= hi)
        memset(dst, 0, (size_t)n);
    else
    {
        if (lo > off)
            memset(dst, 0, (size_t)(lo - off));
        memcpy(dst + (lo - off), c->in + lo, (size_t)(hi - lo));
        k = (lo - off) + (hi - lo);
        if (k < n)
            memset(dst + k, 0, (size_t)(n - k));
    }
    file_advance(f, n);
}

void pw_fzero(pw_file *f, int64_t n)
{
    if (f != NULL && is_huge_write(f, n))
    {
        pw_fzero(f, huge_write_kept(f));
        return;
    }
    if (f == NULL || !file_reserve(f, n))
        return;
    memset(f->data + f->pos, 0, (size_t)n);
    file_advance(f, n);
}

void pw_fputc(pw_file *f, int ch)
{
    uint8_t b = (uint8_t)ch;
    pw_fwrite(f, &b, 1);
}

void pw_fput16be(pw_file *f, uint32_t v)
{
    uint8_t b[2];
    b[0] = (uint8_t)(v >> 8);
    b[1] = (uint8_t)v;
    pw_fwrite(f, b, 2);
}

void pw_fput32be(pw_file *f, uint32_t v)
{
    uint8_t b[4];
    b[0] = (uint8_t)(v >> 24);
    b[1] = (uint8_t)(v >> 16);
    b[2] = (uint8_t)(v >> 8);
    b[3] = (uint8_t)v;
    pw_fwrite(f, b, 4);
}

void pw_fput16le(pw_file *f, uint32_t v)
{
    uint8_t b[2];
    b[0] = (uint8_t)v;
    b[1] = (uint8_t)(v >> 8);
    pw_fwrite(f, b, 2);
}

void pw_fput32le(pw_file *f, uint32_t v)
{
    uint8_t b[4];
    b[0] = (uint8_t)v;
    b[1] = (uint8_t)(v >> 8);
    b[2] = (uint8_t)(v >> 16);
    b[3] = (uint8_t)(v >> 24);
    pw_fwrite(f, b, 4);
}

void pw_fprintf(pw_file *f, const char *fmt, ...)
{
    char    buf[512];
    va_list ap;
    int     n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    if (n >= (int)sizeof buf)
        n = (int)sizeof buf - 1;
    pw_fwrite(f, buf, n);
}

int pw_fseek(pw_file *f, int64_t off, int whence)
{
    int64_t base;

    if (f == NULL)
        return -1;
    switch (whence)
    {
    case SEEK_SET: base = 0; break;
    case SEEK_CUR: base = f->pos; break;
    case SEEK_END: base = (int64_t)f->size; break;
    default: return -1;
    }
    if (base + off < 0)
        return -1;
    f->pos = base + off;
    f->since_seek = 0; /* fseek() flushes the stdio buffer */
    return 0;
}

int64_t pw_ftell(pw_file *f)
{
    return f ? f->pos : -1;
}

size_t pw_fread(void *dst, size_t size, size_t count, pw_file *f)
{
    size_t want, avail;

    if (f == NULL || size == 0 || count == 0)
        return 0;
    if (f->pos >= (int64_t)f->size)
        return 0;
    avail = f->size - (size_t)f->pos;
    want = size * count;
    if (count != 0 && want / count != size)
        want = avail; /* overflow: read what is there */
    if (want > avail)
        want = avail;
    memcpy(dst, f->data + f->pos, want);
    f->pos += (int64_t)want;
    return want / size;
}

const uint8_t *pw_fbuffer(const pw_file *f, size_t *size)
{
    *size = f->size;
    return f->data;
}

void pw_fdiscard(pw_file *f)
{
    if (f == NULL)
        return;
    free(f->data);
    free(f);
}

void pw_fclose(pw_file *f)
{
    pw_ctx *c;

    if (f == NULL)
        return;
    c = f->c;
    if (!f->readonly && c->fatal == 0)
    {
        c->result.files++;
        if (c->cb.file != NULL &&
            c->cb.file(c->cb.user, f->name, f->data, f->size, PW_FILE_CONVERTED, &c->cur) != 0)
        {
            pw_printf(c, "!!couldn't create the file \"%s\"!\nexiting...", f->name);
            c->fatal = PW_ERR_OUTPUT;
        }
    }
    free(f->data);
    free(f);
}
