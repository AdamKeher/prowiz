/*
 * main.c - prowiz command line tool.
 *
 * Without options it behaves like Pro-Wizard for PC 1.70a: same console text,
 * same files ("<n>.<ext>" rips and "<n>.mod" conversions) in the current
 * directory. Options add mode selection, an output directory, a scan-only
 * listing and a JSON report.
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* stat(), mkdir() under -std=c99 */
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define pw_mkdir(p) _mkdir(p)
#else
#include <sys/types.h>
#define pw_mkdir(p) mkdir((p), 0777)
#endif

#if !defined(S_ISDIR) && defined(S_IFDIR) && defined(S_IFMT)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR) /* MSVC has no S_ISDIR */
#elif !defined(S_ISDIR) && defined(_S_IFDIR)
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif

#include "prowiz.h"

#define TYPES_FILENAME "_types_"

typedef struct file_rec
{
    char        *name;
    pw_file_role role;
    size_t       size;
    int          written;
} file_rec;

typedef struct finding
{
    pw_found  f;
    char     *format;
    char     *ext;
    file_rec *files;
    int       nfiles;
} finding;

typedef struct app
{
    /* options */
    int         json;
    int         scan_only;
    int         quiet;
    unsigned    modes;
    const char *outdir;
    const char *types_path;
    const char *input;
    /* state */
    finding    *finds;
    int         nfinds, capfinds;
    int         io_error;
} app;

static void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (p == NULL)
    {
        fputs("prowiz: out of memory\n", stderr);
        exit(2);
    }
    return p;
}

static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char  *p = (char *)xmalloc(n);
    memcpy(p, s, n);
    return p;
}

static int read_file(const char *path, uint8_t **data, size_t *size)
{
    FILE   *f = fopen(path, "rb");
    uint8_t *buf = NULL;
    size_t  len = 0, cap = 0, n;

    if (f == NULL)
        return -1;
    for (;;)
    {
        if (cap - len < 65536)
        {
            cap = cap ? cap * 2 : 1 << 20;
            if (cap > (size_t)0x7fffffff + 65536u)
            {
                fclose(f);
                free(buf);
                errno = EFBIG;
                return -2;
            }
            buf = (uint8_t *)realloc(buf, cap);
            if (buf == NULL)
            {
                fclose(f);
                return -3;
            }
        }
        n = fread(buf + len, 1, cap - len, f);
        len += n;
        if (n == 0)
            break;
    }
    if (ferror(f))
    {
        fclose(f);
        free(buf);
        return -1;
    }
    fclose(f);
    *data = buf;
    *size = len;
    return 0;
}

static char *join_path(const char *dir, const char *name)
{
    size_t dl, nl;
    char  *p;

    if (dir == NULL || dir[0] == '\0')
        return xstrdup(name);
    dl = strlen(dir);
    nl = strlen(name);
    p = (char *)xmalloc(dl + nl + 2);
    memcpy(p, dir, dl);
    if (dir[dl - 1] != '/' && dir[dl - 1] != '\\')
        p[dl++] = '/';
    memcpy(p + dl, name, nl + 1);
    return p;
}

/* ------------------------------------------------------------ callbacks */

static void on_log(void *user, const char *text)
{
    app *a = (app *)user;
    if (a->json || a->quiet || a->scan_only)
        return;
    fputs(text, stdout);
    fflush(stdout);
}

static void on_found(void *user, const pw_found *f)
{
    app     *a = (app *)user;
    finding *d;

    if (a->nfinds == a->capfinds)
    {
        a->capfinds = a->capfinds ? a->capfinds * 2 : 32;
        a->finds = (finding *)realloc(a->finds, (size_t)a->capfinds * sizeof *a->finds);
        if (a->finds == NULL)
        {
            fputs("prowiz: out of memory\n", stderr);
            exit(2);
        }
    }
    d = &a->finds[a->nfinds++];
    memset(d, 0, sizeof *d);
    d->f = *f;
    d->format = xstrdup(f->format ? f->format : "");
    d->ext = xstrdup(f->ext ? f->ext : "");
    d->f.format = d->format;
    d->f.ext = d->ext;
}

static int on_file(void *user, const char *name, const uint8_t *data, size_t size,
                   pw_file_role role, const pw_found *f)
{
    app      *a = (app *)user;
    finding  *d = a->nfinds ? &a->finds[a->nfinds - 1] : NULL;
    file_rec *r;
    int       ok = 1;

    (void)f;
    if (!a->scan_only)
    {
        char *path = join_path(a->outdir, name);
        FILE *out = fopen(path, "wb");
        if (out == NULL || (size && fwrite(data, 1, size, out) != size))
            ok = 0;
        if (out != NULL && fclose(out) != 0)
            ok = 0;
        if (!ok && !a->json)
            fprintf(stderr, "prowiz: cannot write \"%s\": %s\n", path, strerror(errno));
        free(path);
    }
    if (d != NULL)
    {
        d->files = (file_rec *)realloc(d->files, (size_t)(d->nfiles + 1) * sizeof *d->files);
        if (d->files == NULL)
        {
            fputs("prowiz: out of memory\n", stderr);
            exit(2);
        }
        r = &d->files[d->nfiles++];
        r->name = xstrdup(name);
        r->role = role;
        r->size = size;
        r->written = ok && !a->scan_only;
    }
    if (!ok)
    {
        a->io_error = 1;
        return -1;
    }
    return 0;
}

/* -------------------------------------------------------------- output */

static void json_str(FILE *o, const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    fputc('"', o);
    for (; *p; p++)
    {
        switch (*p)
        {
        case '"': fputs("\\\"", o); break;
        case '\\': fputs("\\\\", o); break;
        case '\n': fputs("\\n", o); break;
        case '\r': fputs("\\r", o); break;
        case '\t': fputs("\\t", o); break;
        default:
            if (*p < 0x20)
                fprintf(o, "\\u%04x", *p);
            else
                fputc(*p, o);
        }
    }
    fputc('"', o);
}

static const char *status_name(pw_status s)
{
    switch (s)
    {
    case PW_STATUS_SAVED: return "saved";
    case PW_STATUS_TRUNCATED: return "truncated";
    default: return "unrippable";
    }
}

static void print_json(const app *a, size_t insize, const pw_result *res, int rc)
{
    FILE *o = stdout;
    int   i, k;

    fputs("{\n", o);
    fprintf(o, "  \"program\": \"prowiz\",\n  \"version\": \"%s\",\n  \"engine\": \"Pro-Wizard %s\",\n",
            PROWIZ_VERSION, PROWIZ_UPSTREAM_VERSION);
    fputs("  \"input\": {\"path\": ", o);
    json_str(o, a->input);
    fprintf(o, ", \"size\": %lu},\n", (unsigned long)insize);
    fprintf(o, "  \"modes\": [%s%s%s],\n",
            (a->modes & PW_MODE_MUSIC) ? "\"music\"" : "",
            (a->modes & PW_MODE_MUSIC) && (a->modes & PW_MODE_DATA) ? ", " : "",
            (a->modes & PW_MODE_DATA) ? "\"data\"" : "");
    fprintf(o, "  \"scan_only\": %s,\n", a->scan_only ? "true" : "false");
    fputs("  \"output_dir\": ", o);
    if (a->scan_only)
        fputs("null", o);
    else
        json_str(o, a->outdir ? a->outdir : ".");
    fputs(",\n  \"findings\": [", o);
    for (i = 0; i < a->nfinds; i++)
    {
        const finding *d = &a->finds[i];
        fputs(i ? ",\n    {" : "\n    {", o);
        fprintf(o, "\"index\": %d, \"format\": ", d->f.index);
        json_str(o, d->format);
        fprintf(o, ", \"format_id\": %d, \"kind\": \"%s\", \"status\": \"%s\", "
                   "\"offset\": %lu, \"size\": %lu, \"ext\": ",
                d->f.format_id, d->f.kind == PW_KIND_DATA ? "data" : "music",
                status_name(d->f.status), (unsigned long)d->f.offset, (unsigned long)d->f.size);
        json_str(o, d->ext);
        if (d->f.status == PW_STATUS_TRUNCATED)
            fprintf(o, ", \"missing\": %lu", (unsigned long)d->f.missing);
        fprintf(o, ", \"header_rebuilt\": %s, \"files\": [", d->f.header_rebuilt ? "true" : "false");
        for (k = 0; k < d->nfiles; k++)
        {
            const file_rec *r = &d->files[k];
            char *path = join_path(a->outdir, r->name);
            fputs(k ? ", {\"name\": " : "{\"name\": ", o);
            json_str(o, r->name);
            fputs(", \"path\": ", o);
            if (a->scan_only)
                fputs("null", o);
            else
                json_str(o, path);
            fprintf(o, ", \"role\": \"%s\", \"size\": %lu, \"written\": %s}",
                    r->role == PW_FILE_RIP ? "rip" : "converted", (unsigned long)r->size,
                    r->written ? "true" : "false");
            free(path);
        }
        fputs("]}", o);
    }
    fputs(a->nfinds ? "\n  ],\n" : "],\n", o);
    fprintf(o, "  \"summary\": {\"found\": %d, \"saved\": %d, \"files\": %d},\n",
            res->found, res->saved, res->files);
    fputs("  \"error\": ", o);
    if (rc == PW_ERR_OUTPUT)
        fputs("\"cannot write output file\"", o);
    else if (rc == PW_ERR_NOMEM)
        fputs("\"out of memory\"", o);
    else if (rc != PW_OK)
        fputs("\"scan failed\"", o);
    else
        fputs("null", o);
    fputs("\n}\n", o);
}

static void print_listing(const app *a)
{
    int i, k;

    if (a->nfinds == 0)
    {
        printf("nothing found\n");
        return;
    }
    printf("%-10s %-10s %-10s %s\n", "offset", "size", "status", "format");
    for (i = 0; i < a->nfinds; i++)
    {
        const finding *d = &a->finds[i];
        printf("0x%08lx %-10lu %-10s %s", (unsigned long)d->f.offset, (unsigned long)d->f.size,
               status_name(d->f.status), d->format);
        for (k = 0; k < d->nfiles; k++)
            printf("%s%s", k ? ", " : "  -> ", d->files[k].name);
        printf("\n");
    }
}

/* ---------------------------------------------------------------- main */

static void usage(FILE *o)
{
    fprintf(o,
            "Pro-Wizard %s (portable port of Pro-Wizard for PC %s)\n"
            "Usage: prowiz [options] <input file>\n"
            "\n"
            "Finds Amiga music modules, packed modules and crunched files in <input file>,\n"
            "saves each one as <n>.<ext> and converts packed modules to <n>.mod.\n"
            "Without options the output is identical to Pro-Wizard for PC %s.\n"
            "\n"
            "Options:\n"
            "  -r          music only: modules, packed modules, synth formats\n"
            "  -d          data only: executable and data crunchers\n"
            "  -rd         both (default)\n"
            "  -s          scan only: list what is found, write no files\n"
            "  -j          print a JSON report on stdout instead of the classic text\n"
            "  -o <dir>    write files into <dir> (created if needed; default: current dir)\n"
            "  -t <file>   extension table to use (default: \"%s\" in the current dir)\n"
            "  -q          quiet: no console text\n"
            "  -v          print version and exit\n"
            "  -h          print this help and exit\n",
            PROWIZ_VERSION, PROWIZ_UPSTREAM_VERSION, PROWIZ_UPSTREAM_VERSION, TYPES_FILENAME);
}

static int make_dir(const char *dir)
{
    struct stat st;
    if (stat(dir, &st) == 0)
        return S_ISDIR(st.st_mode) ? 0 : -1;
    return pw_mkdir(dir) == 0 ? 0 : -1;
}

int main(int argc, char **argv)
{
    app        a;
    pw_options opt;
    pw_callbacks cb;
    pw_result  res;
    uint8_t   *data = NULL, *types = NULL;
    size_t     size = 0, types_size = 0;
    int        i, rc, have_types = 0, classic;

    memset(&a, 0, sizeof a);
    for (i = 1; i < argc; i++)
    {
        const char *s = argv[i];
        if (s[0] != '-' || s[1] == '\0')
        {
            if (a.input != NULL)
            {
                fprintf(stderr, "prowiz: more than one input file\n");
                return 2;
            }
            a.input = s;
            continue;
        }
        if (strcmp(s, "--") == 0 && i + 1 < argc)
        {
            a.input = argv[++i];
            continue;
        }
        if (strcmp(s, "-o") == 0 || strcmp(s, "-t") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "prowiz: %s needs an argument\n", s);
                return 2;
            }
            if (s[1] == 'o')
                a.outdir = argv[++i];
            else
                a.types_path = argv[++i];
            continue;
        }
        if (strcmp(s, "-h") == 0 || strcmp(s, "--help") == 0)
        {
            usage(stdout);
            return 0;
        }
        if (strcmp(s, "-v") == 0 || strcmp(s, "--version") == 0)
        {
            printf("prowiz %s (Pro-Wizard for PC %s engine)\n", PROWIZ_VERSION, PROWIZ_UPSTREAM_VERSION);
            return 0;
        }
        /* combined single letter flags: -rd, -sj, ... */
        {
            const char *p;
            for (p = s + 1; *p; p++)
            {
                switch (*p)
                {
                case 'r': a.modes |= PW_MODE_MUSIC; break;
                case 'd': a.modes |= PW_MODE_DATA; break;
                case 's': a.scan_only = 1; break;
                case 'j': a.json = 1; break;
                case 'q': a.quiet = 1; break;
                default:
                    fprintf(stderr, "prowiz: unknown option -%c\n", *p);
                    usage(stderr);
                    return 2;
                }
            }
        }
    }
    if (a.input == NULL)
    {
        usage(stderr);
        return 2;
    }
    if (a.modes == 0)
        a.modes = PW_MODE_ALL;
    classic = !a.json && !a.quiet && !a.scan_only;

    if (classic)
        printf("\n\n-<([ Pro-Wizard v%s ])>-\n\n", PROWIZ_UPSTREAM_VERSION);

    rc = read_file(a.input, &data, &size);
    if (rc != 0)
    {
        if (classic)
        {
            printf("cant find \"%s\" !\n", a.input);
            return 0; /* the original exits with 0 here */
        }
        fprintf(stderr, "prowiz: cannot read \"%s\"\n", a.input);
        return 1;
    }

    /* the extension table */
    if (a.types_path != NULL)
    {
        if (read_file(a.types_path, &types, &types_size) != 0)
        {
            fprintf(stderr, "prowiz: cannot read \"%s\"\n", a.types_path);
            free(data);
            return 1;
        }
        have_types = 1;
    }
    else if (read_file(TYPES_FILENAME, &types, &types_size) == 0)
        have_types = 1;
    else if (classic)
        printf("!!! couldn't find \"%s\" file !. Default extension used.\n", TYPES_FILENAME);

    memset(&cb, 0, sizeof cb);
    cb.user = &a;
    cb.log = on_log;
    if (have_types)
        pw_types_report(types, types_size, &cb); /* "Damaged _types_" warning */

    if (classic)
        printf("input file size : %d\n", (int)size);
    if (size < 64)
    {
        if (classic)
            printf("! input file size is too small ...\n");
        else if (!a.json)
            fprintf(stderr, "prowiz: input file is too small\n");
        if (a.json)
        {
            memset(&res, 0, sizeof res);
            print_json(&a, size, &res, PW_OK);
        }
        free(data);
        free(types);
        return 1;
    }

    if (a.outdir != NULL && !a.scan_only && make_dir(a.outdir) != 0)
    {
        fprintf(stderr, "prowiz: cannot create directory \"%s\"\n", a.outdir);
        free(data);
        free(types);
        return 1;
    }

    memset(&opt, 0, sizeof opt);
    opt.modes = a.modes;
    if (have_types)
    {
        opt.types_data = types;
        opt.types_size = types_size;
    }
    cb.found = on_found;
    cb.file = on_file;

    rc = pw_scan(data, size, &opt, &cb, &res);

    if (a.json)
        print_json(&a, size, &res, rc);
    else if (a.scan_only && !a.quiet)
        print_listing(&a);
    else if (classic && rc == PW_OK)
    {
        printf("\n");
        printf(" 1997-2016 (c) Sylvain \"Asle\" Chipaux (asle@free.fr)\n\n");
    }

    for (i = 0; i < a.nfinds; i++)
    {
        int k;
        for (k = 0; k < a.finds[i].nfiles; k++)
            free(a.finds[i].files[k].name);
        free(a.finds[i].files);
        free(a.finds[i].format);
        free(a.finds[i].ext);
    }
    free(a.finds);
    free(data);
    free(types);
    if (rc == PW_ERR_OUTPUT)
        return 255; /* the original exit(-1)ed */
    return rc == PW_OK ? 0 : 1;
}
