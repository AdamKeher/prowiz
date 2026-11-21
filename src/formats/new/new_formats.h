/*
 * new_formats.h - handlers of the formats Pro-Wizard 1.70a never had.
 *
 * They run only when c->new_formats is set (not with PW_FORMATS_170A), so
 * the original engine stays exactly as it was. scan.c calls pw_scan_new() at
 * the end of each scan step, after all of the original's checks, plus a few
 * hooks inside the original's MMD and Startrekker checks (see the comments
 * at each function).
 *
 * Conventions (see docs/NEW_FORMATS.md for the formats themselves):
 *  - A handler starts at c->i. It checks its full signature first, cheaply:
 *    pw_scan_new() calls it for every byte equal to its first one. Then it
 *    validates and computes the exact size, reading the input only through
 *    IN() or the pw_rd*() helpers below (bounds checked, 0 outside).
 *  - On success it sets c->start and c->out_size, sets c->convert = PW_BAD
 *    and saves with pw_save_rip() (the bytes as found) or pw_save_rip_data()
 *    (bytes restored from an in-memory copy), plus pw_save_companion*() for
 *    a second file. Then it calls pw_skip_rip() and returns 1. Otherwise it
 *    returns 0 and leaves c->i alone.
 *  - A module that runs past the end of the input is still reported:
 *    pw_save_rip() prints it, reports it as TRUNCATED and moves c->i by 2.
 *  - Every loop is bounded by the input size or a format limit: these run on
 *    hostile data. No recursion, no unbounded allocation.
 *
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#ifndef PW_NEW_FORMATS_H
#define PW_NEW_FORMATS_H

#include "../formats.h"

/* big-endian reads from the input; 0 outside it */
static inline uint32_t pw_rd8(const pw_ctx *c, int64_t off)
{
    return pw_in8(c, off);
}
static inline uint32_t pw_rd16(const pw_ctx *c, int64_t off)
{
    return ((uint32_t)pw_in8(c, off) << 8) | pw_in8(c, off + 1);
}
static inline uint32_t pw_rd32(const pw_ctx *c, int64_t off)
{
    return ((uint32_t)pw_in8(c, off) << 24) | ((uint32_t)pw_in8(c, off + 1) << 16) |
           ((uint32_t)pw_in8(c, off + 2) << 8) | pw_in8(c, off + 3);
}

/* the 'n' bytes at 'off' equal 's' */
int pw_match(const pw_ctx *c, int64_t off, const char *s, size_t n);

/* a malloc()ed copy of 'size' input bytes at 'off', for handlers that
 * restore data before saving it; NULL (and c->fatal set) when out of memory */
uint8_t *pw_copy_in(pw_ctx *c, int64_t off, uint32_t size);

/* -------- OctaMED (MMD23.c) */
int  pw_new_MMD23(pw_ctx *c);           /* scan.c, at "MMD2" / "MMD3"              */
int  pw_new_MMD_relocated(pw_ctx *c);   /* scan.c, at an "MMD0".."MMD3" that the
                                           original (or pw_new_MMD23) did not
                                           take: a module relocated in memory     */

/* -------- single-file formats, called by pw_scan_new() */
int  pw_new_PreTracker(pw_ctx *c);      /* 'P'  "PRT"                               */
int  pw_new_SidMon2(pw_ctx *c);         /* 'S'  "SIDMON II - THE MIDI VERSION";
                                                the module starts at c->i - 0x3A    */
int  pw_new_DeltaMusic2(pw_ctx *c);     /* '.'  ".FNL"; the module starts at
                                                c->i - 0xBC6                        */
int  pw_new_ArtOfNoise(pw_ctx *c);      /* 'A'  "AON4" / "AON8"                     */
int  pw_new_DigiBoosterPro(pw_ctx *c);  /* 'D'  "DBM0"                              */
int  pw_new_Musicline(pw_ctx *c);       /* 'M'  "MLEDMODL"                          */
int  pw_new_Symphonie(pw_ctx *c);       /* 'S'  "SymM"                              */
int  pw_new_RobHubbard(pw_ctx *c);      /* 0x60 (bra.w ...)                         */
int  pw_new_BenDaglish(pw_ctx *c);      /* 0x60 (bra.w ...)                         */
int  pw_new_DavidWhittaker(pw_ctx *c);  /* 0x48 / 0x47 / 0x08 code prologues        */

/* -------- formats with a second file */
int  pw_new_LSP(pw_ctx *c);             /* 'L'  "LSP1" / "LSPm" (+ .lsbank)         */
int  pw_new_TFMX(pw_ctx *c);            /* 'T' / 't'  mdat (+ smpl), TFHD, TFMXPAK  */
int  pw_new_StartrekkerNT(pw_ctx *c);   /* 'S' / 'A'  a .nt not saved with its mod  */
void pw_new_StartrekkerAM_companion(pw_ctx *c); /* scan.c, right after a FLT4 / EXO4
                                           module was ripped (c->cur is that
                                           finding): save its .nt when found       */
int  pw_new_Startrekker8(pw_ctx *c);    /* scan.c, at "FLT8" / "EXO8"               */

#endif
