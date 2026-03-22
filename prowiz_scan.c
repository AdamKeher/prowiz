/*
 * prowiz_scan.c — scan engine driver (library code)
 *
 * Contains PW_ScanData() which is the core byte-scanning loop.
 * Format detection is delegated to:
 *   PW_ScanRipAt()  — module format checks  (prowiz_scan_rip.c)
 *   PW_ScanDataAt() — data cruncher checks  (prowiz_scan_data.c)
 */

#include "globals.h"
#include "extern.h"

/* ---------- scan engine (called by main() and prowiz_lib) ----------- */
void PW_ScanData(void)
{
  Current_Found_Index = 0;
  First_JSON_Entry = GOOD;
  PW_ResultCount = 0;
  if (!PW_LibMode && Script_Mode == GOOD)
  {
    printf("{\n  \"results\": [\n");
  }
  for (PW_i = 0; PW_i < (uint32_t)(PW_in_size - MINIMAL_FILE_LENGHT); PW_i += 1)
  {
    Current_Is_Module = GOOD;

    /* skip runs of zeroes */
    if (in_data[PW_i] == 0x00)
    {
      for (PW_j = 0; PW_j < MINIMAL_FILE_LENGHT; PW_j++)
      {
        if (in_data[PW_j + PW_i] != 0x00)
          break;
      }
      if (PW_j == MINIMAL_FILE_LENGHT)
      {
        PW_i += (MINIMAL_FILE_LENGHT - 2);
        continue;
      }
    }

    /* module format checks (rip mode library) */
    PW_ScanRipAt();

    /* data cruncher format checks (data mode library) */
    PW_ScanDataAt();
  }

  if (!PW_LibMode && Script_Mode == GOOD)
  {
    printf("\n  ]\n}\n");
  }
}
