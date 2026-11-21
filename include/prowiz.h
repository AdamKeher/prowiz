/*
 * prowiz.h - public API of the Pro-Wizard library.
 *
 * Pro-Wizard finds Amiga music modules, packed modules and crunched
 * executables/data inside arbitrary binary data (disk images, memory dumps,
 * intros, games ...). It saves each hit as a separate file and, for most
 * packed module formats, converts it back to a Protracker module.
 *
 * This library is a portable, re-entrant C99 port of Pro-Wizard for PC 1.70a
 * by Sylvain "Asle" Chipaux. Detection, rip sizes, file naming and conversion
 * output follow the original engine byte for byte. On top of that it finds a
 * set of formats the original never knew (see pw_options.formats); with
 * PW_FORMATS_170A it is exactly the original engine.
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation. See COPYING.
 */
#ifndef PROWIZ_H
#define PROWIZ_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROWIZ_VERSION          "2.0.0"
#define PROWIZ_UPSTREAM_VERSION "1.70a"

/* Scan modes (bit mask). The default, PW_MODE_ALL, matches upstream. */
#define PW_MODE_MUSIC 0x1u /* music modules, packed modules, synth formats */
#define PW_MODE_DATA  0x2u /* executable and data crunchers                */
#define PW_MODE_ALL   (PW_MODE_MUSIC | PW_MODE_DATA)

/* Format sets (pw_options.formats). */
#define PW_FORMATS_ALL  0 /* everything this library knows (default)              */
#define PW_FORMATS_170A 1 /* only Pro-Wizard 1.70a's formats: output identical to it */

/* Return codes of pw_scan(). */
#define PW_OK          0
#define PW_ERR_ARG    -1 /* invalid argument                          */
#define PW_ERR_NOMEM  -2 /* out of memory                             */
#define PW_ERR_OUTPUT -3 /* the file callback reported a write error  */

typedef enum pw_kind {
    PW_KIND_MUSIC = 0, /* module / packed module / synth music */
    PW_KIND_DATA  = 1  /* executable or data cruncher          */
} pw_kind;

typedef enum pw_status {
    PW_STATUS_SAVED      = 0, /* ripped and handed to the file callback        */
    PW_STATUS_TRUNCATED  = 1, /* detected, but the data runs past end of input */
    PW_STATUS_UNRIPPABLE = 2  /* detected, but Pro-Wizard cannot rip it        */
} pw_status;

typedef enum pw_file_role {
    PW_FILE_RIP       = 0, /* the raw data as found (exe header possibly rebuilt) */
    PW_FILE_CONVERTED = 1, /* the conversion to Protracker (or similar)          */
    PW_FILE_COMPANION = 2  /* a second file the format needs next to the rip
                              (LSP sample bank, TFMX samples, Startrekker .nt)   */
} pw_file_role;

/* One detection. */
typedef struct pw_found {
    int         index;          /* output file number ("<index>.<ext>"); -1 if none */
    int         format_id;      /* see pw_format_count() / pw_format_ext()          */
    const char *format;         /* human readable format name                       */
    const char *ext;            /* extension used for the rip file                  */
    pw_kind     kind;
    pw_status   status;
    uint32_t    offset;         /* start of the data in the input                   */
    uint32_t    size;           /* size of the ripped data                          */
    uint32_t    missing;        /* bytes missing when status is TRUNCATED           */
    int         header_rebuilt; /* an Amiga hunk header was synthesised             */
    int         converting;     /* a conversion file will follow                    */
    int         restored;       /* found as a player had prepared it in memory
                                   (pointers relocated, ...); the rip file is the
                                   data as it was before (formats not in 1.70a)     */
} pw_found;

typedef struct pw_callbacks {
    void *user;

    /* Classic Pro-Wizard console text, exactly as the original printed it
     * (may contain backspaces). Optional. */
    void (*log)(void *user, const char *text);

    /* A detection. Called before any file of that detection. Optional. */
    void (*found)(void *user, const pw_found *f);

    /* An output file. 'name' is the plain file name Pro-Wizard would create
     * ("12.mod"). Later calls may reuse a name; they replace the earlier file,
     * as the original program overwrote it. Return 0 on success; any other
     * value aborts the scan with PW_ERR_OUTPUT. Optional. */
    int (*file)(void *user, const char *name, const uint8_t *data, size_t size,
                pw_file_role role, const pw_found *f);
} pw_callbacks;

typedef struct pw_options {
    unsigned       modes;      /* PW_MODE_*; 0 means PW_MODE_ALL                 */
    const uint8_t *types_data; /* contents of a "_types_" extension file, or NULL */
    size_t         types_size; /* for the built-in extensions. It covers the
                                  1.70a formats; the others keep their own.      */
    int            formats;    /* PW_FORMATS_*; 0 means PW_FORMATS_ALL            */
} pw_options;

typedef struct pw_result {
    int found;     /* detections (all statuses)       */
    int saved;     /* detections with status SAVED     */
    int files;     /* file callbacks issued            */
} pw_result;

/* Scan 'size' bytes of 'data'. The input is copied; it is never modified.
 * 'opt' and 'result' may be NULL. Inputs shorter than 64 bytes produce no
 * detections. */
int pw_scan(const uint8_t *data, size_t size, const pw_options *opt,
            const pw_callbacks *cb, pw_result *result);

/* Parse a "_types_" file exactly as pw_scan() will and print, through
 * cb->log, the warning the original printed for a damaged one. pw_scan()
 * itself stays silent about it. Returns how many extensions differ from the
 * built-in ones, or PW_ERR_NOMEM. */
int pw_types_report(const uint8_t *data, size_t size, const pw_callbacks *cb);

/* Format table (index = pw_found.format_id). IDs below pw_format_count_170a()
 * are Pro-Wizard 1.70a's (the rows of a "_types_" file); the rest were added
 * by this library. */
int         pw_format_count(void);
int         pw_format_count_170a(void);
const char *pw_format_ext(int format_id);  /* built-in extension, NULL if out of range */
pw_kind     pw_format_kind(int format_id);

const char *pw_version(void);

#ifdef __cplusplus
}
#endif

#endif /* PROWIZ_H */
