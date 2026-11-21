/*
 * pw_internal.h - engine internals shared by the scanner and format handlers.
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 *
 * PORTING MODEL
 * -------------
 * The original engine keeps its whole state in globals (PW_i, PW_j .. PW_o,
 * PW_Start_Address, OutputSize, ...). Format handlers pass results from their
 * test function to their rip function through those globals, and the order in
 * which the scanner calls handlers makes some of that state leak from one
 * handler to the next. To reproduce the original output exactly, the port keeps
 * every one of those variables, with the SAME integer type, as a field of a
 * pw_ctx. Nothing is global, so independent scans can run concurrently.
 *
 *   PW_i                 -> c->i            uint32_t  scan position
 *   PW_j..PW_o           -> c->j..c->o      uint32_t  shared scratch
 *   PW_Start_Address     -> c->start        int32_t
 *   OutputSize           -> c->out_size     uint32_t
 *   PW_in_size           -> c->in_size      int32_t
 *   PW_WholeSampleSize   -> c->wss          uint32_t
 *   Cpt_Filename         -> c->file_counter int32_t
 *   Save_Status          -> c->save_status  uint8_t   PW_GOOD / PW_BAD
 *   CONVERT              -> c->convert      uint8_t
 *   Amiga_EXE_Header     -> c->amiga_exe_header uint8_t
 *   OutName_final        -> c->out_name (larger: see PW_NAME_MAX)
 *   Depacked_OutName     -> c->depacked_name
 *   Extensions[]         -> c->ext_flat, read with PW_EXT(c, id)
 *   in_data[x]           -> IN(x)           bounds checked, 0 outside input
 *   in_data[x] = v       -> IN_SET(x, v)    ignored outside input
 *
 * Memory safety: every input access is bounds checked (reads outside the
 * input yield 0) and output files live in growable memory buffers (pw_file).
 * Where the original read past the end of its buffers its behaviour was
 * undefined; the port behaves as if those bytes were zero.
 */
#ifndef PW_INTERNAL_H
#define PW_INTERNAL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "prowiz.h"
#include "format_ids.h"

#define PW_GOOD 0
#define PW_BAD  1

#define PW_MIN_FILE_LENGTH 64            /* MINIMAL_FILE_LENGHT  */
#define PW_TYPES_LINE_LENGTH 256         /* _TYPES_LINE_LENGHT   */
#define PW_EXT_LEN 33                    /* width of Extensions[] rows */
#define PW_EXT_TABLE ((PWF_COUNT + 1) * PW_EXT_LEN)
/* The original sprintf()ed "<n>.<ext>" into a 33 byte buffer; an extension
 * from "_types_" may run on through later rows of the table, so allow that. */
#define PW_NAME_MAX (PW_EXT_TABLE + 16)

/* Hard cap for one output file built in memory. The original would happily
 * try to write gigabytes when fed garbage sizes; we stop growing the file. */
#define PW_MAX_OUTPUT_FILE (256u * 1024u * 1024u)

#if defined(__GNUC__) || defined(__clang__)
#define PW_PRINTF(f, a) __attribute__((format(printf, f, a)))
#else
#define PW_PRINTF(f, a)
#endif

typedef struct pw_file pw_file;

/* Bytes stored outside the input. Several rippers patch bytes at
 * in_data[start + size - n] before checking that the rip fits the input, and
 * later tests may read them back. The port models the memory around the
 * input as zero-filled and writable: such writes are kept here (a small hash
 * table, rarely used) and IN() returns them. */
#define PW_OUTSIDE_MAX (1u << 20)
typedef struct pw_outside
{
    int64_t *key;
    uint8_t *val;
    uint32_t cap;  /* power of two, or 0 */
    uint32_t used;
} pw_outside;

typedef struct pw_ctx
{
    /* input: private copy (Rip_DietDataPacker patches bytes, like upstream) */
    uint8_t   *in;
    int32_t    in_size;
    pw_outside outside;

    /* scan registers - see the table above */
    uint32_t i;
    uint32_t j, k, l, m, n, o;
    int32_t  start;
    uint32_t out_size;
    uint32_t wss;
    int32_t  file_counter;
    uint8_t  save_status;
    uint8_t  convert;
    uint8_t  amiga_exe_header;
    char     out_name[PW_NAME_MAX];
    char     depacked_name[33];
    /* Extensions[][33] as one block: an overlong "_types_" line spills into
     * the next rows and a row without NUL runs on into the next, as in the
     * original. The extra final byte is always 0. */
    char     ext_flat[PW_EXT_TABLE + 1];

    /* contents of the most recent rip file (Depack_SoundFX13 re-reads it) */
    uint8_t *last_rip;
    size_t   last_rip_size;

    /* host side */
    unsigned     modes;
    int          new_formats; /* 0 with PW_FORMATS_170A: the original engine only */
    pw_callbacks cb;
    int          fatal;     /* PW_ERR_* once the scan must stop */
    pw_found     cur;       /* the detection files belong to   */
    char         cur_format[128]; /* copy of cur.format (handlers pass stack buffers) */
    pw_result    result;

    /* formats added by the port: input ranges already saved as companion
     * files, so their own signature is not ripped a second time */
    struct { uint32_t start, size; } *claimed;
    uint32_t     nclaimed, capclaimed;
} pw_ctx;

/* ---------------------------------------------------------------- input */

uint8_t pw_outside_get(const pw_ctx *c, int64_t off);
void    pw_outside_set(pw_ctx *c, int64_t off, uint8_t v);

static inline uint8_t pw_in8(const pw_ctx *c, int64_t off)
{
    if (off >= 0 && off < (int64_t)c->in_size)
        return c->in[off];
    return c->outside.used ? pw_outside_get(c, off) : 0;
}

static inline void pw_in_set(pw_ctx *c, int64_t off, uint8_t v)
{
    if (off >= 0 && off < (int64_t)c->in_size)
        c->in[off] = v;
    else
        pw_outside_set(c, off, v);
}

/* Expressions keep the upstream integer types; only the final index is
 * widened, which reproduces what in_data[expr] addressed on a 64-bit build. */
#define IN(x)        pw_in8(c, (int64_t)(x))
#define IN_SET(x, v) pw_in_set(c, (int64_t)(x), (uint8_t)(v))

/* ------------------------------------------------------------- logging */

/* printf() replacement: text goes to the log callback unchanged. */
void pw_printf(pw_ctx *c, const char *fmt, ...) PW_PRINTF(2, 3);

/* ------------------------------------------------------- output files
 *
 * pw_file emulates a FILE opened with "w+b" (or "r+b" for the rip re-read),
 * including seeking past the end (the gap reads back as zeros) and reading
 * back what was written. The finished file is handed to the host on close. */

pw_file *pw_fopen(pw_ctx *c, const char *name);  /* PW_fopen(name, "w+b")           */
pw_file *pw_fopen_last_rip(pw_ctx *c);           /* fopen(OutName_final, "r+b")     */
void     pw_fclose(pw_file *f);                  /* emits the file (if writable)    */
void     pw_fdiscard(pw_file *f);                /* close without emitting          */
const uint8_t *pw_fbuffer(const pw_file *f, size_t *size);

void    pw_fwrite(pw_file *f, const void *src, int64_t n);   /* fwrite(src, n, 1, f) */
void    pw_fwrite_in(pw_file *f, int64_t off, int64_t n);    /* fwrite(&in_data[off], n, 1, f) */
void    pw_fzero(pw_file *f, int64_t n);                     /* n zero bytes         */
void    pw_fputc(pw_file *f, int ch);
void    pw_fput16be(pw_file *f, uint32_t v);
void    pw_fput32be(pw_file *f, uint32_t v);
void    pw_fput16le(pw_file *f, uint32_t v); /* fwrite(&u16, 2, 1, f) on x86 */
void    pw_fput32le(pw_file *f, uint32_t v); /* fwrite(&u32, 4, 1, f) on x86 */
void    pw_fprintf(pw_file *f, const char *fmt, ...) PW_PRINTF(2, 3);
int     pw_fseek(pw_file *f, int64_t off, int whence);       /* SEEK_SET/CUR/END: 0/1/2 */
int64_t pw_ftell(pw_file *f);
/* fread(dst, size, count, f): returns complete items read; a partial item is
 * still copied, like the C library does. */
size_t  pw_fread(void *dst, size_t size, size_t count, pw_file *f);

/* --------------------------------------- shared routines (misc/testbag) */

void    pw_save_rip(pw_ctx *c, const char *format, int fmt_id);
void    pw_save_rip_special(pw_ctx *c, const char *format, int fmt_id,
                            const uint8_t *header, uint32_t header_size);
void    pw_crap(const char *format, uint8_t delta, uint8_t pack, pw_file *out);
void    pw_crap15(const char *format, uint8_t delta, uint8_t pack, pw_file *out);
int16_t pw_test_special_cruncher_data(pw_ctx *c, int32_t pack_addy, int32_t unpack_addy);
void    pw_rip_special_cruncher_data(pw_ctx *c, const char *name, int header_size, int fmt_id);
int16_t pw_test_1_start(pw_ctx *c, uint32_t limit);
int16_t pw_test_smps(int32_t smpsiz, int32_t lstart, int32_t lsiz, uint8_t vol, uint8_t fine);
void    pw_fill_ptk_table(uint8_t poss[37][2]);
/* note a structured "found but cannot rip" event (no file produced) */
void    pw_report_unrippable(pw_ctx *c, int fmt_id, const char *format, uint32_t offset);

extern const int16_t pw_tuning[16][36];          /* tuning.h */
extern const char *const pw_default_ext[PWF_COUNT + 1];
extern const unsigned char pw_format_kinds[PWF_COUNT];
extern const char *const pw_new_ext[PWF_NEW_COUNT];
extern const unsigned char pw_new_kinds[PWF_NEW_COUNT];

/* extension / kind of any format ID (1.70a ones through the "_types_" table) */
const char *pw_ext_of(const pw_ctx *c, int fmt_id);
pw_kind     pw_kind_of(int fmt_id);

/* ------------------------------------- formats added by the port
 *
 * Handlers of formats Pro-Wizard 1.70a never had (src/formats/new/). They
 * only run when c->new_formats is set, so with PW_FORMATS_170A the engine is
 * the original one. They report through the same pw_save_rip() as the
 * original handlers, plus these: */

/* Like pw_save_rip(), but the file gets 'size' bytes of 'data' instead of the
 * input bytes at c->start (c->start and c->out_size still say where, and how
 * much, was found in the input). 'restored' sets pw_found.restored: the data
 * was rebuilt from a copy a player had prepared in memory. */
void pw_save_rip_data(pw_ctx *c, const char *format, int fmt_id,
                      const uint8_t *data, size_t size, int restored);

/* A second file of the detection just saved (call only right after a rip
 * with c->save_status == PW_GOOD): "<index>.<ext>", role PW_FILE_COMPANION.
 * pw_save_companion_in() takes the bytes from the input and claims that
 * range (see pw_claimed()). */
void pw_save_companion(pw_ctx *c, const char *ext, const uint8_t *data, size_t size);
void pw_save_companion_in(pw_ctx *c, const char *ext, uint32_t start, uint32_t size);

/* input ranges saved as companions; a handler skips data starting there */
void pw_claim(pw_ctx *c, uint32_t start, uint32_t size);
int  pw_claimed(const pw_ctx *c, uint32_t start);

/* after a successful rip: continue scanning after the module
 * (c->start + c->out_size), never backwards */
void pw_skip_rip(pw_ctx *c);

/* types / extensions */
void pw_types_default(pw_ctx *c);
void pw_types_load(pw_ctx *c, const uint8_t *data, size_t size, int report);
#define PW_EXT(c, id) (&(c)->ext_flat[(size_t)(id) * PW_EXT_LEN])

/* the scanner (scan.c) and its hook for the port's formats (formats/new/) */
void pw_scan_run(pw_ctx *c);
void pw_scan_new(pw_ctx *c);

#endif /* PW_INTERNAL_H */
