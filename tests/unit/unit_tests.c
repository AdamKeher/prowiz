/*
 * unit_tests.c - tests of the engine core and the public API.
 * (Format handlers are tested differentially against the original: see
 * tests/difftest.py.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prowiz.h"
#include "pw_internal.h"

static int failures = 0;

#define CHECK(cond)                                                        \
    do                                                                     \
    {                                                                      \
        if (!(cond))                                                       \
        {                                                                  \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                    \
        }                                                                  \
    } while (0)

/* ------------------------------------------------------------ pw_file */

typedef struct sink
{
    int      files;
    char     name[64];
    uint8_t  data[8192];
    size_t   size;
    int      role;
    int      found;
    pw_found last;
    char     format[128];
    char     log[4096];
    int      fail;
} sink;

static int sink_file(void *user, const char *name, const uint8_t *data, size_t size,
                     pw_file_role role, const pw_found *f)
{
    sink *s = (sink *)user;
    (void)f;
    s->files++;
    snprintf(s->name, sizeof s->name, "%s", name);
    s->size = size < sizeof s->data ? size : sizeof s->data;
    if (size)
        memcpy(s->data, data, s->size);
    s->role = role;
    return s->fail ? -1 : 0;
}

static void sink_found(void *user, const pw_found *f)
{
    sink *s = (sink *)user;
    s->found++;
    s->last = *f;
    snprintf(s->format, sizeof s->format, "%s", f->format);
}

static void sink_log(void *user, const char *text)
{
    sink *s = (sink *)user;
    size_t n = strlen(s->log);
    snprintf(s->log + n, sizeof s->log - n, "%s", text);
}

static void test_file_emulation(void)
{
    pw_ctx  c;
    sink    s;
    pw_file *f;
    uint8_t in[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t buf[16];
    size_t  n;

    memset(&c, 0, sizeof c);
    memset(&s, 0, sizeof s);
    c.cb.user = &s;
    c.cb.file = sink_file;
    c.in = in;
    c.in_size = 8;

    f = pw_fopen(&c, "x.mod");
    CHECK(f != NULL);
    pw_fwrite(f, "AB", 2);
    CHECK(pw_ftell(f) == 2);
    /* seeking past the end, then writing, leaves a zero gap */
    CHECK(pw_fseek(f, 6, SEEK_SET) == 0);
    pw_fputc(f, 'Z');
    CHECK(pw_ftell(f) == 7);
    /* a seek past the end without a write does not grow the file */
    CHECK(pw_fseek(f, 100, SEEK_SET) == 0);
    CHECK(pw_fseek(f, 0, SEEK_END) == 0);
    CHECK(pw_ftell(f) == 7);
    CHECK(pw_fseek(f, -1, SEEK_SET) != 0);
    /* reading back */
    pw_fseek(f, 0, SEEK_SET);
    memset(buf, 0xee, sizeof buf);
    n = pw_fread(buf, 1, 16, f);
    CHECK(n == 7);
    CHECK(memcmp(buf, "AB\0\0\0\0Z", 7) == 0);
    CHECK(buf[7] == 0xee);
    /* fread of items: partial item counts as 0 but is copied */
    pw_fseek(f, 4, SEEK_SET);
    memset(buf, 0xee, sizeof buf);
    CHECK(pw_fread(buf, 4, 1, f) == 0);
    CHECK(buf[2] == 'Z' && buf[3] == 0xee);
    /* in_data copies are zero padded outside the input */
    pw_fseek(f, 0, SEEK_END);
    pw_fwrite_in(f, 6, 4);  /* 7 8 0 0 */
    pw_fwrite_in(f, -2, 3); /* 0 0 1  */
    pw_fput16be(f, 0x1234);
    pw_fput16le(f, 0x1234);
    pw_fput32be(f, 0xa1b2c3d4u);
    pw_fput32le(f, 0xa1b2c3d4u);
    pw_fprintf(f, "[%s]", "hi");
    pw_fclose(f);

    CHECK(s.files == 1);
    CHECK(strcmp(s.name, "x.mod") == 0);
    CHECK(s.role == PW_FILE_CONVERTED);
    {
        static const uint8_t want[] = {'A', 'B', 0, 0, 0, 0, 'Z', 7, 8, 0, 0, 0, 0, 1,
                                       0x12, 0x34, 0x34, 0x12, 0xa1, 0xb2, 0xc3, 0xd4,
                                       0xd4, 0xc3, 0xb2, 0xa1, '[', 'h', 'i', ']'};
        CHECK(s.size == sizeof want);
        CHECK(memcmp(s.data, want, sizeof want) == 0);
    }

    /* input reads are bounds checked */
    CHECK(pw_in8(&c, -1) == 0);
    CHECK(pw_in8(&c, 7) == 8);
    CHECK(pw_in8(&c, 8) == 0);
    CHECK(pw_in8(&c, (int64_t)0xffffffffu) == 0);
}

/* --------------------------------------------------------------- API */

/* a minimal 4 channel Protracker module: 31 empty samples, 1 empty pattern */
static size_t make_mod(uint8_t *m)
{
    memset(m, 0, 2108);
    memcpy(m, "unit test module", 16);
    m[950] = 1;   /* song length */
    m[951] = 0x7f;
    memcpy(m + 1080, "M.K.", 4);
    return 2108;
}

static void test_scan_mod(void)
{
    static uint8_t buf[4096];
    pw_callbacks   cb;
    pw_options     opt;
    pw_result      res;
    sink           s;
    size_t         n = make_mod(buf + 100);
    int            rc;

    memset(buf, 0x55, 100); /* some data before the module */
    n += 100;
    memset(buf + n, 0xaa, 200);
    n += 200;

    memset(&s, 0, sizeof s);
    memset(&cb, 0, sizeof cb);
    cb.user = &s;
    cb.file = sink_file;
    cb.found = sink_found;
    cb.log = sink_log;
    rc = pw_scan(buf, n, NULL, &cb, &res);
    CHECK(rc == PW_OK);
    CHECK(res.found == 1 && res.saved == 1 && res.files == 1);
    CHECK(s.found == 1);
    CHECK(s.last.offset == 100);
    CHECK(s.last.size == 2108);
    CHECK(s.last.status == PW_STATUS_SAVED);
    CHECK(s.last.kind == PW_KIND_MUSIC);
    CHECK(s.last.format_id == PWF_Protracker);
    CHECK(strcmp(s.format, "Pro|Star|whatever tracker 4 channels module") == 0);
    CHECK(strcmp(s.name, "0.mod") == 0);
    CHECK(s.role == PW_FILE_RIP);
    CHECK(s.size == 2108 && memcmp(s.data, buf + 100, 2108) == 0);
    CHECK(strstr(s.log, "found at 100 !. its size is : 2108\n") != NULL);
    CHECK(strstr(s.log, "  saving in file \"0.mod\" ... done\n") != NULL);

    /* data-only mode does not look for modules */
    memset(&s, 0, sizeof s);
    memset(&opt, 0, sizeof opt);
    opt.modes = PW_MODE_DATA;
    rc = pw_scan(buf, n, &opt, &cb, &res);
    CHECK(rc == PW_OK && res.found == 0 && s.files == 0);

    /* a custom extension table */
    {
        static char types[4096];
        int i;
        types[0] = 0;
        for (i = 0; i < PWF_COUNT; i++)
            strcat(types, i == PWF_Protracker ? "ptk\n" : "x\n");
        memset(&s, 0, sizeof s);
        opt.modes = 0;
        opt.types_data = (const uint8_t *)types;
        opt.types_size = strlen(types);
        rc = pw_scan(buf, n, &opt, &cb, &res);
        CHECK(rc == PW_OK && strcmp(s.name, "0.ptk") == 0);
        /* every entry differs from the built-in table ("ptk" replaces "mod") */
        CHECK(pw_types_report((const uint8_t *)types, strlen(types), NULL) == PWF_COUNT);
        /* a damaged table falls back to the defaults and warns once asked to */
        memset(&s, 0, sizeof s);
        CHECK(pw_types_report((const uint8_t *)"a\nb\n", 4, &cb) == 0);
        CHECK(strstr(s.log, "Damaged \"_types_\" file. Missing up 165 extensions") != NULL);
    }

    /* the file callback can abort the scan */
    memset(&s, 0, sizeof s);
    s.fail = 1;
    rc = pw_scan(buf, n, NULL, &cb, &res);
    CHECK(rc == PW_ERR_OUTPUT);
    CHECK(strstr(s.log, "!!couldn't create the file \"0.mod\"!") != NULL);
}

static void test_truncated(void)
{
    /* "RNC" data: unpacked size at +4, packed size (24 bits) at +9 */
    static uint8_t buf[300];
    pw_callbacks cb;
    pw_result    res;
    sink         s;

    memset(buf, 0x11, sizeof buf);
    memcpy(buf + 10, "RNC\001", 4);
    buf[14] = 0; buf[15] = 0; buf[16] = 0x10; buf[17] = 0; /* unpacked 4096  */
    buf[18] = 0; buf[19] = 0; buf[20] = 0x08; buf[21] = 0; /* packed   2048  */
    memset(&s, 0, sizeof s);
    memset(&cb, 0, sizeof cb);
    cb.user = &s;
    cb.found = sink_found;
    cb.file = sink_file;
    pw_scan(buf, sizeof buf, NULL, &cb, &res);
    CHECK(s.found >= 1);
    CHECK(s.last.status == PW_STATUS_TRUNCATED);
    CHECK(s.last.offset == 10);
    CHECK(s.last.size == 2048 + 18);
    CHECK(s.last.missing == 10 + 2048 + 18 - 300);
    CHECK(s.last.kind == PW_KIND_DATA);
    CHECK(s.files == 0);
}

static void test_edge_inputs(void)
{
    uint8_t   small[63] = {0};
    pw_result res;
    CHECK(pw_scan(NULL, 0, NULL, NULL, &res) == PW_OK && res.found == 0);
    CHECK(pw_scan(NULL, 5, NULL, NULL, NULL) == PW_ERR_ARG);
    CHECK(pw_scan(small, sizeof small, NULL, NULL, &res) == PW_OK && res.found == 0);
    CHECK(pw_format_count() == PWF_ALL_COUNT);
    CHECK(pw_format_count_170a() == PWF_COUNT);
    CHECK(strcmp(pw_format_ext(PWF_Noisepacker2), "NoisePacker2") == 0);
    CHECK(strcmp(pw_format_ext(PWF_PreTracker), "prt") == 0);
    CHECK(pw_format_ext(-1) == NULL && pw_format_ext(PWF_ALL_COUNT) == NULL);
    CHECK(pw_format_kind(PWF_LSP) == PW_KIND_MUSIC);
    CHECK(pw_format_kind(PWF_Powerpacker3) == PW_KIND_DATA);
    CHECK(pw_format_kind(PWF_P61A) == PW_KIND_MUSIC);
    CHECK(strcmp(pw_version(), PROWIZ_VERSION) == 0);
}

int main(void)
{
    test_file_emulation();
    test_scan_mod();
    test_truncated();
    test_edge_inputs();
    if (failures)
    {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("all unit tests passed\n");
    return 0;
}
