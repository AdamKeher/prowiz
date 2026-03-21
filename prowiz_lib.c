/*
 * prowiz_lib.c — library API for WASM and Node.js bindings
 *
 * Exports:
 *   prowiz_scan()    — scan a buffer, returns malloc'd JSON (caller must free)
 *   prowiz_extract() — extract a single entry as raw bytes
 *
 * Build notes:
 *   CLI build : compile alongside prowiz.c (vars.h lives there)
 *   WASM build: emcc with --no-entry; main() present but not exported
 */

#include "globals.h"
#include "extern.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pw_lib_setup(const uint8_t *data, int32_t size,
                         int include_modules, int include_data)
{
  in_data         = (uint8_t *)(uintptr_t)data; /* caller owns buffer */
  PW_in_size      = (int64_t)size;
  PW_LibMode      = 1;
  Scan_Only       = GOOD;
  Do_Rip          = GOOD;
  Do_Depack       = BAD;
  Do_Module_Mode  = include_modules ? GOOD : BAD;
  Do_Data_Mode    = include_data    ? GOOD : BAD;
  Filter_By_Index = BAD;
  Current_Found_Index = 0;
  First_JSON_Entry    = GOOD;
  PW_ResultCount      = 0;
  Module_Found        = BAD;
  Script_Mode         = GOOD;
  CONVERT             = BAD;
  PW_WholeSampleSize  = 0;
  Cpt_Filename        = 0;
  Support_Types();
}

static void pw_lib_teardown(void)
{
  PW_LibMode = 0;
  in_data    = NULL; /* caller owns buffer; do not free here */
}

/*
 * prowiz_scan — scan `data` (length `size`) and return a JSON string.
 * The returned string is malloc'd; caller must call free() on it.
 * Returns NULL on allocation failure.
 *
 * include_modules : non-zero to include music module hits
 * include_data    : non-zero to include data cruncher hits
 */
char *prowiz_scan(const uint8_t *data, int32_t size,
                  int include_modules, int include_data)
{
  pw_lib_setup(data, size, include_modules, include_data);
  PW_ScanData();
  pw_lib_teardown();

  /* Serialise PW_Results[] to JSON */
  size_t buf_size = 64 + (size_t)PW_ResultCount * 384 + 32;
  char *out = (char *)malloc(buf_size);
  if (!out)
    return NULL;

  int pos = 0;
  pos += snprintf(out + pos, buf_size - pos, "{\n  \"results\": [\n");

  for (int i = 0; i < PW_ResultCount; i++) {
    PW_FindResult *r = &PW_Results[i];
    if (i > 0)
      pos += snprintf(out + pos, buf_size - pos, ",\n");
    pos += snprintf(out + pos, buf_size - pos,
                    "    {\n"
                    "      \"id\": \"%s\",\n"
                    "      \"offset\": %lld,\n"
                    "      \"size\": %u,\n"
                    "      \"index\": %d,\n"
                    "      \"is_module\": %s,\n"
                    "      \"truncated\": %s\n"
                    "    }",
                    r->format_id, (long long)r->offset, r->size, r->index,
                    r->is_module ? "true" : "false",
                    r->truncated ? "true" : "false");
  }

  pos += snprintf(out + pos, buf_size - pos, "\n  ]\n}\n");
  return out;
}

/*
 * prowiz_extract — copy the raw bytes of entry `index` into `out_data`.
 * Works only for formats that rip without transformation (no depacking).
 * Returns the number of bytes written, or -1 if the entry was not found,
 * is truncated, or requires depacking.
 *
 * out_data must be at least the `size` reported by prowiz_scan for this entry.
 */
int32_t prowiz_extract(const uint8_t *data, int32_t size,
                       int index, uint8_t *out_data)
{
  pw_lib_setup(data, size, 1 /*modules*/, 1 /*data*/);
  PW_ScanData();
  pw_lib_teardown();

  for (int i = 0; i < PW_ResultCount; i++) {
    PW_FindResult *r = &PW_Results[i];
    if (r->index != index)
      continue;
    if (r->truncated)
      return -1;
    if (r->offset + (int64_t)r->size > size)
      return -1;
    memcpy(out_data, data + r->offset, r->size);
    return (int32_t)r->size;
  }
  return -1; /* index not found */
}
