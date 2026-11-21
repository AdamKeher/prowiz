/*
 * save.c - saving rips and the helpers shared by all format handlers
 *          (Save_Rip, Save_Rip_Special, Crap, the special cruncher-data
 *          routines and the common tests of the original misc.c/testbag.c).
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include <stdio.h>

#include "pw_internal.h"

static void found_begin(pw_ctx *c, const char *format, int fmt_id)
{
    memset(&c->cur, 0, sizeof c->cur);
    c->cur.index = -1;
    c->cur.format_id = fmt_id;
    strncpy(c->cur_format, format, sizeof c->cur_format - 1);
    c->cur_format[sizeof c->cur_format - 1] = 0;
    c->cur.format = c->cur_format;
    c->cur.ext = pw_ext_of(c, fmt_id);
    c->cur.kind = pw_kind_of(fmt_id);
    c->cur.offset = (uint32_t)c->start;
    c->cur.size = c->out_size;
}

static void found_emit(pw_ctx *c)
{
    c->result.found++;
    if (c->cur.status == PW_STATUS_SAVED)
        c->result.saved++;
    if (c->cb.found != NULL)
        c->cb.found(c->cb.user, &c->cur);
}

/* Hands a finished rip file to the host and remembers its bytes. */
static void emit_rip(pw_ctx *c, const uint8_t *data, size_t size)
{
    uint8_t *copy = NULL;

    if (size > 0)
    {
        copy = (uint8_t *)malloc(size);
        if (copy == NULL)
        {
            c->fatal = PW_ERR_NOMEM;
            return;
        }
        memcpy(copy, data, size);
    }
    free(c->last_rip);
    c->last_rip = copy;
    c->last_rip_size = size;

    c->result.files++;
    if (c->cb.file != NULL &&
        c->cb.file(c->cb.user, c->out_name, data, size, PW_FILE_RIP, &c->cur) != 0)
    {
        /* PW_fopen() printed this and exit()ed */
        pw_printf(c, "!!couldn't create the file \"%s\"!\nexiting...", c->out_name);
        c->fatal = PW_ERR_OUTPUT;
    }
}

static void emit_file(pw_ctx *c, pw_file *f)
{
    size_t size;
    const uint8_t *data = pw_fbuffer(f, &size);
    emit_rip(c, data, size);
    pw_fdiscard(f);
}

/* the part common to Save_Rip and Save_Rip_Special; returns 0 if truncated */
static int save_prologue(pw_ctx *c, const char *format, int fmt_id)
{
    c->save_status = PW_BAD;
    pw_printf(c, "%s found at %d !. its size is : %u\n", format, c->start, c->out_size);
    found_begin(c, format, fmt_id);

    /* (PW_Start_Address + (int32_t)OutputSize) > PW_in_size, in 32 bits */
    if ((int32_t)((uint32_t)c->start + c->out_size) > c->in_size)
    {
        uint32_t missing = ((uint32_t)c->start + c->out_size) - (uint32_t)c->in_size;
        pw_printf(c, "!!! Truncated, missing (%u byte(s) !)\n", missing);
        c->i += 2;
        c->cur.status = PW_STATUS_TRUNCATED;
        c->cur.missing = missing;
        found_emit(c);
        return 0;
    }

    memset(c->out_name, 0, sizeof c->out_name);
    snprintf(c->out_name, sizeof c->out_name, "%d.%s", c->file_counter, pw_ext_of(c, fmt_id));
    pw_printf(c, "  saving in file \"%s\" ... ", c->out_name);
    c->cur.index = c->file_counter;
    c->cur.status = PW_STATUS_SAVED;
    c->cur.converting = (c->convert == PW_GOOD);
    c->file_counter += 1;
    return 1;
}

void pw_save_rip(pw_ctx *c, const char *format, int fmt_id)
{
    pw_file *f;

    if (!save_prologue(c, format, fmt_id))
        return;
    found_emit(c);

    /* build the file through pw_file so out-of-input bytes are zero-filled */
    f = pw_fopen(c, c->out_name);
    if (f == NULL)
        return;
    pw_fwrite_in(f, c->start, (int64_t)c->out_size);
    emit_file(c, f);
    if (c->fatal)
        return;

    pw_printf(c, "done\n");
    if (c->convert == PW_GOOD)
        pw_printf(c, "  converting to Protracker ... ");
    c->save_status = PW_GOOD;
}

void pw_save_rip_special(pw_ctx *c, const char *format, int fmt_id,
                         const uint8_t *header, uint32_t header_size)
{
    static const uint8_t ending[4] = {0x00, 0x00, 0x03, 0xf2};
    pw_file *f;

    if (!save_prologue(c, format, fmt_id))
        return;
    c->cur.header_rebuilt = 1;
    found_emit(c);

    f = pw_fopen(c, c->out_name);
    if (f == NULL)
        return;
    pw_fwrite(f, header, header_size);
    pw_fwrite_in(f, c->start, (int64_t)(uint32_t)(c->out_size - 4));
    pw_fwrite(f, ending, 4);
    emit_file(c, f);
    if (c->fatal)
        return;

    pw_printf(c, "done\n");
    if (c->convert == PW_GOOD)
        pw_printf(c, "  converting to Protracker ... ");
    pw_printf(c, "  Header of this file was missing and has been rebuilt !\n");
    if (fmt_id == PWF_DragPack252)
        pw_printf(c, "  WARNING !: it's a fake header in this case !!\n");
    c->amiga_exe_header = PW_GOOD;
    c->save_status = PW_GOOD;
}

/* ------------------------------------- formats added by the port */

void pw_save_rip_data(pw_ctx *c, const char *format, int fmt_id,
                      const uint8_t *data, size_t size, int restored)
{
    if (!save_prologue(c, format, fmt_id))
        return;
    c->cur.restored = restored ? 1 : 0;
    found_emit(c);
    emit_rip(c, data, size);
    if (c->fatal)
        return;
    pw_printf(c, "done\n");
    if (restored)
        pw_printf(c, "  It was found as prepared by its player in memory: restored !\n");
    c->save_status = PW_GOOD;
}

void pw_save_companion(pw_ctx *c, const char *ext, const uint8_t *data, size_t size)
{
    char name[PW_NAME_MAX];

    if (c->fatal || c->save_status != PW_GOOD || c->cur.index < 0)
        return;
    snprintf(name, sizeof name, "%d.%s", c->cur.index, ext);
    pw_printf(c, "  saving its companion file \"%s\" ... ", name);
    c->result.files++;
    if (c->cb.file != NULL &&
        c->cb.file(c->cb.user, name, data, size, PW_FILE_COMPANION, &c->cur) != 0)
    {
        pw_printf(c, "!!couldn't create the file \"%s\"!\nexiting...", name);
        c->fatal = PW_ERR_OUTPUT;
        return;
    }
    pw_printf(c, "done\n");
}

void pw_save_companion_in(pw_ctx *c, const char *ext, uint32_t start, uint32_t size)
{
    uint8_t *buf;
    uint32_t k;

    if (c->fatal || c->save_status != PW_GOOD || c->cur.index < 0)
        return;
    buf = (uint8_t *)malloc(size ? size : 1);
    if (buf == NULL)
    {
        c->fatal = PW_ERR_NOMEM;
        return;
    }
    for (k = 0; k < size; k++)
        buf[k] = IN((int64_t)start + k);
    pw_save_companion(c, ext, buf, size);
    free(buf);
    pw_claim(c, start, size);
}

void pw_claim(pw_ctx *c, uint32_t start, uint32_t size)
{
    if (c->nclaimed == c->capclaimed)
    {
        uint32_t ncap = c->capclaimed ? c->capclaimed * 2 : 16;
        void *p = realloc(c->claimed, (size_t)ncap * sizeof *c->claimed);
        if (p == NULL)
            return; /* only costs a duplicate rip */
        c->claimed = p;
        c->capclaimed = ncap;
    }
    c->claimed[c->nclaimed].start = start;
    c->claimed[c->nclaimed].size = size;
    c->nclaimed++;
}

int pw_claimed(const pw_ctx *c, uint32_t start)
{
    uint32_t k;
    for (k = 0; k < c->nclaimed; k++)
        if (c->claimed[k].start == start)
            return 1;
    return 0;
}

void pw_skip_rip(pw_ctx *c)
{
    uint32_t next;

    if (c->save_status != PW_GOOD || c->out_size == 0)
        return;
    next = (uint32_t)c->start + c->out_size - 1; /* the scan loop adds 1 */
    if (next > c->i && next < (uint32_t)c->in_size)
        c->i = next;
}

void pw_report_unrippable(pw_ctx *c, int fmt_id, const char *format, uint32_t offset)
{
    int32_t  save_start = c->start;
    uint32_t save_size = c->out_size;

    c->start = (int32_t)offset;
    c->out_size = 0;
    found_begin(c, format, fmt_id);
    c->cur.status = PW_STATUS_UNRIPPABLE;
    found_emit(c);
    c->start = save_start;
    c->out_size = save_size;
}

/* writing craps in converted MODs */
void pw_crap(const char *format, uint8_t delta, uint8_t pack, pw_file *out)
{
    pw_fseek(out, 560, SEEK_SET);
    pw_fprintf(out, "[  Converted with  ]");
    pw_fseek(out, 590, SEEK_SET);
    pw_fprintf(out, "[ ProWizard for PC ]");
    pw_fseek(out, 620, SEEK_SET);
    pw_fprintf(out, "[ written by Asle! ]");

    pw_fseek(out, 680, SEEK_SET);
    pw_fprintf(out, "[ Original Format: ]");
    pw_fseek(out, 710, SEEK_SET);
    pw_fprintf(out, "[%s]", format);

    if (delta == PW_GOOD)
    {
        pw_fseek(out, 770, SEEK_SET);
        pw_fprintf(out, "[! smp were DELTA  ]");
    }
    if (pack == PW_GOOD)
    {
        pw_fseek(out, 800, SEEK_SET);
        pw_fprintf(out, "[! smp were PACKED ]");
    }
}

/* same, for 15 sample modules */
void pw_crap15(const char *format, uint8_t delta, uint8_t pack, pw_file *out)
{
    (void)delta;
    (void)pack;
    pw_fseek(out, 260, SEEK_SET);
    pw_fprintf(out, "[  Converted with  ]");
    pw_fseek(out, 290, SEEK_SET);
    pw_fprintf(out, "[ ProWizard for PC ]");
    pw_fseek(out, 320, SEEK_SET);
    pw_fprintf(out, "[ written by Asle! ]");

    pw_fseek(out, 380, SEEK_SET);
    pw_fprintf(out, "[ Original Format: ]");
    pw_fseek(out, 410, SEEK_SET);
    pw_fprintf(out, "[%s]", format);
}

static uint32_t be24(const pw_ctx *c, int64_t a)
{
    return ((uint32_t)pw_in8(c, a) << 16) | ((uint32_t)pw_in8(c, a + 1) << 8) | pw_in8(c, a + 2);
}

static uint32_t be32(const pw_ctx *c, int64_t a)
{
    return ((uint32_t)pw_in8(c, a) << 24) | be24(c, a + 1);
}

/*
 * testSpecialCruncherData(): data crunchers with a packed size field at
 * 'pack_addy' and an unpacked size field at 'unpack_addy'.
 * Leaves the packed size in c->l and the unpacked size in c->k.
 */
int16_t pw_test_special_cruncher_data(pw_ctx *c, int32_t pack_addy, int32_t unpack_addy)
{
    c->start = (int32_t)c->i;

    if (((int32_t)c->i + pack_addy) > c->in_size)
        return PW_BAD;

    /* packed size - the first byte is sometimes used for something else */
    c->l = be24(c, (int64_t)(c->start + pack_addy + 1));
    /* unpacked size */
    c->k = be32(c, (int64_t)(c->start + unpack_addy));

    if ((c->k <= 2) || (c->l <= 2))
        return PW_BAD;
    if (c->l > 0x989680) /* 10 mb */
        return PW_BAD;
    if (c->k <= c->l)
        return PW_BAD;
    if (c->k > 0x989689) /* 10 Megs ! */
        return PW_BAD;
    return PW_GOOD;
}

void pw_rip_special_cruncher_data(pw_ctx *c, const char *name, int header_size, int fmt_id)
{
    /* c->l is the whole size - header_size */
    switch (header_size)
    {
    case 999991: /* SQ data cruncher */
        c->l *= 4;
        c->l += 10;
        c->out_size = c->l;
        header_size = 6;
        break;
    default:
        c->out_size = c->l + (uint32_t)header_size;
    }

    c->convert = PW_BAD;
    pw_save_rip(c, name, fmt_id);

    if (c->save_status == PW_GOOD)
        c->i += (uint32_t)(header_size + 1); /* test to overcome fake datas */
    c->wss = 0;
}

/* start : if mod can possibly fit from the beginning of the file */
int16_t pw_test_1_start(pw_ctx *c, uint32_t limit)
{
    return (c->i < limit) ? PW_BAD : PW_GOOD;
}

int16_t pw_test_smps(int32_t smpsiz, int32_t lstart, int32_t lsiz, uint8_t vol, uint8_t fine)
{
    if (lstart > smpsiz)
        return PW_BAD;
    if (lsiz > (smpsiz + 2))
        return PW_BAD;
    if ((lstart + lsiz) > smpsiz + 2)
        return PW_BAD;
    if ((lstart != 0) && (lsiz < 2))
        return PW_BAD;
    /* The original also had
     *   if (((lstart != 0) || (lsiz > 2)) && (smpsiz = 0)) return BAD;
     * which assigns instead of comparing and therefore never fires. */
    if ((vol > 0x40) || (fine > 0x0f))
        return PW_BAD;
    return PW_GOOD;
}

/* Protracker periods, as {hi, lo} bytes, indexed by note number (0 = none) */
void pw_fill_ptk_table(uint8_t poss[37][2])
{
    static const uint16_t periods[37] = {
        0,
        856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
        428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
        214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113};
    int i;

    for (i = 0; i < 37; i++)
    {
        poss[i][0] = (uint8_t)(periods[i] >> 8);
        poss[i][1] = (uint8_t)periods[i];
    }
}

/* period tables for each finetune (tuning.h) */
const int16_t pw_tuning[16][36] = {
    {856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
     428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
     214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113},
    {850, 802, 757, 715, 674, 637, 601, 567, 535, 505, 477, 450,
     425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 239, 225,
     213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113},
    {844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447,
     422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237, 224,
     211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112},
    {838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444,
     419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235, 222,
     209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111},
    {832, 785, 741, 699, 660, 623, 588, 555, 524, 495, 467, 441,
     416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233, 220,
     208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110},
    {826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437,
     413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232, 219,
     206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109},
    {820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434,
     410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230, 217,
     205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109},
    {814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431,
     407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228, 216,
     204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108},
    {907, 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480,
     453, 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240,
     226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120},
    {900, 850, 802, 757, 715, 675, 636, 601, 567, 535, 505, 477,
     450, 425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 238,
     225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119},
    {894, 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474,
     447, 422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237,
     223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118},
    {887, 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470,
     444, 419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235,
     222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118},
    {881, 832, 785, 741, 699, 660, 623, 588, 555, 524, 494, 467,
     441, 416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233,
     220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117},
    {875, 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463,
     437, 413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232,
     219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116},
    {868, 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460,
     434, 410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230,
     217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115},
    {862, 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457,
     431, 407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228,
     216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114}};
