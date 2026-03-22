/*
 * prowiz_scan_data.c — data cruncher format detection (data mode library)
 */

#include "globals.h"
#include "extern.h"

void PW_ScanDataAt(void)
{
    if (in_data[PW_i] <= 0x40)
    {
#ifdef INCLUDEALL
      /* StoneCracker 2.92 data (ex-$08090A08 data cruncher) */
      if ((in_data[PW_i] == 0x08) &&
          (in_data[PW_i + 1] == 0x09) &&
          (in_data[PW_i + 2] == 0x0A) &&
          ((in_data[PW_i + 3] == 0x08) ||
           (in_data[PW_i + 3] == 0x0A) ||
           (in_data[PW_i + 3] == 0x0B) ||
           (in_data[PW_i + 3] == 0x0E) ||
           (in_data[PW_i + 3] == 0x0D)))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("StoneCracker 2.92 Data Cruncher", 12, STC292data);
        }
      }
      /* "1AM" data cruncher */
      if ((in_data[PW_i] == '1') &&
          (in_data[PW_i + 1] == 'A') &&
          (in_data[PW_i + 2] == 'M'))
      {
        if (testSpecialCruncherData(12, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Amnesty Design (1AM) Data Cruncher", 16, AmnestyDesign1);
        }
      }
      /* "2AM" data cruncher */
      if ((in_data[PW_i] == '2') &&
          (in_data[PW_i + 1] == 'A') &&
          (in_data[PW_i + 2] == 'M'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Amnesty Design (2AM) Data Cruncher", 12, AmnestyDesign2);
        }
      }
      /* =SB= data cruncher */
      if ((in_data[PW_i] == 0x3D) &&
          (in_data[PW_i + 1] == 'S') &&
          (in_data[PW_i + 2] == 'B') &&
          (in_data[PW_i + 3] == 0x3D))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("=SB= Data Cruncher", 12, SB_DataCruncher);
        }
      }

      /* B9AB data cruncher */
      if ((in_data[PW_i] == 0x0B) &&
          (in_data[PW_i + 1] == 0x09) &&
          (in_data[PW_i + 2] == 0x0A) &&
          (in_data[PW_i + 3] == 0x0B))
      {
        if (testB9AB() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("B9AB Data Cruncher", 274, B9AB);
        }
      }

      /* -CJ- data cruncher (CrackerJack/Mirage)*/
      if ((in_data[PW_i] == 0x2D) &&
          (in_data[PW_i + 1] == 'C') &&
          (in_data[PW_i + 2] == 'J') &&
          (in_data[PW_i + 3] == 0x2D))
      {
        if (testSpecialCruncherData(4, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("-CJ- Data Cruncher", 0, CJ_DataCruncher);
        }
      }

      /* Max Packer 1.2 */
      if ((in_data[PW_i] == 0x28) &&
          (in_data[PW_i + 1] == 0x3C) &&
          (in_data[PW_i + 6] == 0x26) &&
          (in_data[PW_i + 7] == 0x7A) &&
          (in_data[PW_i + 8] == 0x01) &&
          (in_data[PW_i + 9] == 0x6C) &&
          (in_data[PW_i + 10] == 0x41) &&
          (in_data[PW_i + 11] == 0xFA) &&
          (in_data[PW_i + 12] == 0x01) &&
          (in_data[PW_i + 13] == 0x7C) &&
          (in_data[PW_i + 14] == 0xD1) &&
          (in_data[PW_i + 15] == 0xFA))
      {
        if (testMaxPacker12() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_MaxPacker12();
        }
      }
#endif
    }

    /**********************************/
    /* ok, now, the files with ID ... */
    /**********************************/
    switch (in_data[PW_i])
    {
    case 'A': /* ATN! another Imploder case */
#ifdef INCLUDEALL
      if ((in_data[PW_i + 1] == 'T') &&
          (in_data[PW_i + 2] == 'N') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Imploder data", 50, IMP);
        }
      }
      /* Time Cruncher 1.7 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x01) &&
          (in_data[PW_i + 3] == 0x34) &&
          (in_data[PW_i + 4] == 0xD1) &&
          (in_data[PW_i + 5] == 0xFC) &&
          (in_data[PW_i + 10] == 0x43) &&
          (in_data[PW_i + 11] == 0xF9) &&
          (in_data[PW_i + 16] == 0x24) &&
          (in_data[PW_i + 17] == 0x60) &&
          (in_data[PW_i + 18] == 0xD5) &&
          (in_data[PW_i + 19] == 0xC9) &&
          (in_data[PW_i + 20] == 0x20) &&
          (in_data[PW_i + 21] == 0x20))
      {
        if (testTimeCruncher17() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_TimeCruncher17();
        }
      }
      /* IAM Cruncher 1.0 (another case (aka ICE)) */
      if ((in_data[PW_i + 1] == 'T') &&
          (in_data[PW_i + 2] == 'M') &&
          (in_data[PW_i + 3] == '5'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("IAM Packer 1.0 (ATM5) data", 12, ICE);
        }
      }
      /* ATOM - Atomik Packer (Atari ST) */
      if ((in_data[PW_i + 1] == 'T') &&
          (in_data[PW_i + 2] == 'O') &&
          (in_data[PW_i + 3] == 'M'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Atomik Packer (ATOM) data", 12, AtomikPackerData);
        }
      }
      /* ATM3 - Atomik Packer (Atari ST) */
      if ((in_data[PW_i + 1] == 'T') &&
          (in_data[PW_i + 2] == 'M') &&
          (in_data[PW_i + 3] == '3'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Atomik Packer (ATM3) data", 12, AtomikPackerData);
        }
      }
      /* "AU5!" - Automation Packer 5.* (Atari ST) */
      if ((in_data[PW_i + 1] == 'U') &&
          (in_data[PW_i + 2] == '5') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Automation Packer v5.01 (data)", 0, AutomationPackerData);
        }
      }
      /* Syncro Packer 4.6 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x01) &&
          (in_data[PW_i + 3] == 0x66) &&
          (in_data[PW_i + 4] == 0x22) &&
          (in_data[PW_i + 5] == 0x58) &&
          (in_data[PW_i + 6] == 0x20) &&
          (in_data[PW_i + 7] == 0x18) &&
          (in_data[PW_i + 8] == 0x26) &&
          (in_data[PW_i + 9] == 0x48) &&
          (in_data[PW_i + 10] == 0xD1) &&
          (in_data[PW_i + 11] == 0xC0) &&
          (in_data[PW_i + 12] == 0x1E) &&
          (in_data[PW_i + 13] == 0x20) &&
          (in_data[PW_i + 14] == 0x1C) &&
          (in_data[PW_i + 15] == 0x20))
      {
        if (testSyncroPacker() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SyncroPacker();
        }
      }
      /* "ArcD" data cruncher */
      if ((in_data[PW_i + 1] == 'r') &&
          (in_data[PW_i + 2] == 'c') &&
          (in_data[PW_i + 3] == 'D'))
      {
        if (testArcDDataCruncher() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("ArcD data Cruncher", 0, arcD);
        }
      }
      /* Bytekiller Clone FLT */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x01) &&
          (in_data[PW_i + 3] == 0x32) &&
          (in_data[PW_i + 4] == 0xD1) &&
          (in_data[PW_i + 5] == 0xFA) &&
          (in_data[PW_i + 6] == 0x01) &&
          (in_data[PW_i + 7] == 0x2A) &&
          (in_data[PW_i + 8] == 0x22) &&
          (in_data[PW_i + 9] == 0x7c))
      {
        if (testBKCloneFLT() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_BKCloneFLT();
        }
      }
      /* unknown Bytekiller Clone 5 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x01) &&
          (in_data[PW_i + 3] == 0x16) &&
          (in_data[PW_i + 4] == 0xD1) &&
          (in_data[PW_i + 5] == 0xFC) &&
          (in_data[PW_i + 10] == 0x43) &&
          (in_data[PW_i + 11] == 0xF9) &&
          (in_data[PW_i + 16] == 0x24) &&
          (in_data[PW_i + 17] == 0x60))
      {
        if (testBKClone5() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_BKClone5();
        }
      }
      /* Ace? (data cruncher) */
      if ((in_data[PW_i + 1] == 'c') &&
          (in_data[PW_i + 2] == 'e') &&
          (in_data[PW_i + 3] == '?'))
      {
        if (testSpecialCruncherData(4, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("(Ace?) Data Cruncher", 0, ACECruncherData);
        }
      }
#endif
      break;

    case 'B': /* BTB6 */
#ifdef INCLUDEALL
      /* ByteKiller 1.3 (exepack) */
      if ((in_data[PW_i + 1] == 'T') &&
          (in_data[PW_i + 2] == 'B') &&
          (in_data[PW_i + 3] == '6'))
      {
        if (testByteKiller_13() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_ByteKiller();
        }
        if (testByteKiller_20() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_ByteKiller();
        }
      }

      /* BHC2 (data cruncher) */
      if ((in_data[PW_i + 1] == 'H') &&
          (in_data[PW_i + 2] == 'C') &&
          (in_data[PW_i + 3] == '2'))
      {
        if (testSpecialCruncherData(6, 10) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("(BHC2) Data Cruncher", 0x3C, BHC2CruncherData);
        }
      }
      /* BHC3 (data cruncher) */
      if ((in_data[PW_i + 1] == 'H') &&
          (in_data[PW_i + 2] == 'C') &&
          (in_data[PW_i + 3] == '3'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("(BHC3) Data Cruncher", 0xC8, BHC3CruncherData);
        }
      }
#endif
      break;

    case 'C': /* 0x43 */
#ifdef INCLUDEALL
      /* CrM2 | Crm2 | CrM! */
      if (((in_data[PW_i + 1] == 'r') &&
           (in_data[PW_i + 2] == 'M') &&
           (in_data[PW_i + 3] == '2')) ||
          ((in_data[PW_i + 1] == 'r') &&
           (in_data[PW_i + 2] == 'm') &&
           (in_data[PW_i + 3] == '2')) ||
          ((in_data[PW_i + 1] == 'r') &&
           (in_data[PW_i + 2] == 'M') &&
           (in_data[PW_i + 3] == '!')))
      {
        if (testSpecialCruncherData(10, 6) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Crunchmania / Normal data", 14, CRM1);
        }
      }
      /* "CHFI"  another imploder case */
      if ((in_data[PW_i + 1] == 'H') &&
          (in_data[PW_i + 2] == 'F') &&
          (in_data[PW_i + 3] == 'I'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Imploder data", 50, IMP);
        }
      }
      /* "CRND" data cruncher */
      if ((in_data[PW_i + 1] == 'R') &&
          (in_data[PW_i + 2] == 'N') &&
          (in_data[PW_i + 3] == 'D'))
      {
        if (testCRND() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("CRND data cruncher", 20, CRND);
        }
      }
      /* Defjam Cruncher 3.2 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x02) &&
          (in_data[PW_i + 3] == 0x8C) &&
          ((in_data[PW_i + 4] == 0x4B) || (in_data[PW_i + 4] == 0x9B)) &&
          ((in_data[PW_i + 5] == 0xF9) || (in_data[PW_i + 5] == 0xCD)) &&
          ((in_data[PW_i + 6] == 0x00) || (in_data[PW_i + 6] == 0x4E)))
      {
        if (testDefjam32() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_Defjam32();
        }
      }
#endif
      break;

    case 'F': /* 0x46 */
      /* "FORM" : EA-IFF */
      if ((in_data[PW_i + 1] == 'O') &&
          (in_data[PW_i + 2] == 'R') &&
          (in_data[PW_i + 3] == 'M'))
      {
        if (testIFF() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_IFF();
        }
      }
#ifdef INCLUDEALL
      /* FIRE (RNC clone) Cruncher */
      if ((in_data[PW_i + 1] == 'I') &&
          (in_data[PW_i + 2] == 'R') &&
          (in_data[PW_i + 3] == 'E'))
      {
        if (testSpecialCruncherData(4, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("FIRE (RNC Clone) data Cruncher", 0, FIRE);
        }
      }
#endif
      break;

    case 'H': /* 0x48 */
#ifdef INCLUDEALL
      /* Powerpacker 4.0 library */
      if ((in_data[PW_i + 1] == 0x7A) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0x58) &&
          (in_data[PW_i + 4] == 0x48) &&
          (in_data[PW_i + 5] == 0xE7) &&
          (in_data[PW_i + 6] == 0xFF) &&
          (in_data[PW_i + 7] == 0xFE) &&
          (in_data[PW_i + 8] == 0x70) &&
          (in_data[PW_i + 9] == 0x23) &&
          (in_data[PW_i + 10] == 0x43) &&
          (in_data[PW_i + 11] == 0xFA) &&
          (in_data[PW_i + 12] == 0x00) &&
          (in_data[PW_i + 13] == 0x50) &&
          (in_data[PW_i + 14] == 0x2C) &&
          (in_data[PW_i + 15] == 0x78))
      {
        if (testPowerpacker4lib() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_Powerpacker4lib();
        }
      }
      /* StoneCracker 2.70 */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFE) &&
          (in_data[PW_i + 4] == 0x4D) &&
          (in_data[PW_i + 5] == 0xF9) &&
          (in_data[PW_i + 6] == 0x00) &&
          (in_data[PW_i + 7] == 0xDF) &&
          (in_data[PW_i + 8] == 0xF0) &&
          (in_data[PW_i + 9] == 0x06) &&
          (in_data[PW_i + 10] == 0x7E) &&
          (in_data[PW_i + 11] == 0x00) &&
          (in_data[PW_i + 12] == 0x7C) &&
          (in_data[PW_i + 13] == 0x00) &&
          (in_data[PW_i + 14] == 0x7A) &&
          (in_data[PW_i + 15] == 0x00))
      {
        if (testStoneCracker270() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_StoneCracker270();
        }
      }

      /* ByteKiller 3.0 */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFE) &&
          (in_data[PW_i + 4] == 0x41) &&
          (in_data[PW_i + 5] == 0xFA) &&
          (in_data[PW_i + 6] == 0x00) &&
          (in_data[PW_i + 7] == 0xB6) &&
          (in_data[PW_i + 8] == 0x43) &&
          (in_data[PW_i + 9] == 0xF9) &&
          (in_data[PW_i + 14] == 0x4D) &&
          (in_data[PW_i + 15] == 0xF9))
      {
        if (testByteKiller30() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_ByteKiller30();
        }
      }

      /* Powerpacker 2.3 */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFE) &&
          (in_data[PW_i + 4] == 0x41) &&
          (in_data[PW_i + 5] == 0xFA) &&
          (in_data[PW_i + 6] == 0xFF) &&
          (in_data[PW_i + 7] == 0xF6) &&
          (in_data[PW_i + 8] == 0x20) &&
          (in_data[PW_i + 9] == 0x50) &&
          (in_data[PW_i + 10] == 0xD1) &&
          (in_data[PW_i + 11] == 0xC8) &&
          (in_data[PW_i + 12] == 0xD1) &&
          (in_data[PW_i + 13] == 0xC8) &&
          (in_data[PW_i + 14] == 0x4A) &&
          (in_data[PW_i + 15] == 0x98))
      {
        if (testPowerpacker23() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_Powerpacker23();
        }
      }

      /* Powerpacker 3.0 */
      if ((in_data[PW_i + 1] == 0x7A) &&
          (in_data[PW_i + 2] == 0x01) &&
          (in_data[PW_i + 3] == 0x78) &&
          (in_data[PW_i + 4] == 0x48) &&
          (in_data[PW_i + 5] == 0xE7) &&
          (in_data[PW_i + 6] == 0xFF) &&
          (in_data[PW_i + 7] == 0xFE) &&
          (in_data[PW_i + 8] == 0x49) &&
          (in_data[PW_i + 9] == 0xFA) &&
          (in_data[PW_i + 10] == 0xFF) &&
          (in_data[PW_i + 11] == 0xF2) &&
          (in_data[PW_i + 12] == 0x20) &&
          (in_data[PW_i + 13] == 0x54) &&
          (in_data[PW_i + 14] == 0xD1) &&
          (in_data[PW_i + 15] == 0xC8))
      {
        if (testPowerpacker30() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_Powerpacker30();
        }
      }

      /* Powerpacker 4.0 */
      if ((in_data[PW_i + 1] == 0x7A) &&
          (in_data[PW_i + 2] == 0x01) &&
          (in_data[PW_i + 3] == 0xC8) &&
          (in_data[PW_i + 4] == 0x48) &&
          (in_data[PW_i + 5] == 0xE7) &&
          (in_data[PW_i + 6] == 0xFF) &&
          (in_data[PW_i + 7] == 0xFE) &&
          (in_data[PW_i + 8] == 0x49) &&
          (in_data[PW_i + 9] == 0xFA) &&
          (in_data[PW_i + 10] == 0xFF) &&
          (in_data[PW_i + 11] == 0xF2) &&
          (in_data[PW_i + 12] == 0x20) &&
          (in_data[PW_i + 13] == 0x54) &&
          (in_data[PW_i + 14] == 0xD1) &&
          (in_data[PW_i + 15] == 0xC8))
      {
        if (testPowerpacker40() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_Powerpacker40();
        }
      }

      /* Super Cruncher 2.7 */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFE) &&
          (in_data[PW_i + 4] == 0x2C) &&
          (in_data[PW_i + 5] == 0x79) &&
          (in_data[PW_i + 10] == 0x4E) &&
          (in_data[PW_i + 11] == 0xAE) &&
          (in_data[PW_i + 12] == 0xFF) &&
          (in_data[PW_i + 13] == 0x7C) &&
          (in_data[PW_i + 14] == 0x41) &&
          (in_data[PW_i + 15] == 0xFA))
      {
        if (testSuperCruncher27() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SuperCruncher27();
        }
      }

      /* RelokIt 1.0 */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFE) &&
          (in_data[PW_i + 4] == 0x41) &&
          (in_data[PW_i + 5] == 0xFA) &&
          (in_data[PW_i + 6] == 0x02) &&
          (in_data[PW_i + 7] == 0xC6) &&
          (in_data[PW_i + 8] == 0x70) &&
          (in_data[PW_i + 9] == 0x00) &&
          (in_data[PW_i + 10] == 0x30) &&
          (in_data[PW_i + 11] == 0x28) &&
          (in_data[PW_i + 12] == 0x00) &&
          (in_data[PW_i + 13] == 0x04) &&
          (in_data[PW_i + 14] == 0x23) &&
          (in_data[PW_i + 15] == 0xC0))
      {
        if (testRelokIt10() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_RelokIt10();
        }
      }
#endif
      break;

    case 'I': /* 0x48 */
#ifdef INCLUDEALL
      /* "ICE!" : ID of IAM packer 1.0 */
      if ((in_data[PW_i + 1] == 'C') &&
          (in_data[PW_i + 2] == 'E') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testSpecialCruncherData(4, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("IAM Packer 1.0 (ICE!) data", 0, ICE);
        }
      }
      /* "Ice!" : ID of Ice! Cruncher */
      if ((in_data[PW_i + 1] == 'c') &&
          (in_data[PW_i + 2] == 'e') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testSpecialCruncherData(4, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Ice! Cruncher (data)", 0, ICE);
        }
      }
      /* "IMP!" */
      if ((in_data[PW_i + 1] == 'M') &&
          (in_data[PW_i + 2] == 'P') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Imploder data", 50, IMP);
        }
      }
#endif
      break;

    case 'K': /* 0x4B */
#ifdef INCLUDEALL
      /* Try-It Cruncher 1.01 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x01) &&
          (in_data[PW_i + 3] == 0x54) &&
          (in_data[PW_i + 4] == 0x24) &&
          (in_data[PW_i + 5] == 0x6D) &&
          (in_data[PW_i + 6] == 0x00) &&
          (in_data[PW_i + 7] == 0x18) &&
          (in_data[PW_i + 8] == 0xB3) &&
          (in_data[PW_i + 9] == 0xED) &&
          (in_data[PW_i + 10] == 0x00) &&
          (in_data[PW_i + 11] == 0x18) &&
          (in_data[PW_i + 12] == 0x6F) &&
          (in_data[PW_i + 13] == 0x0E) &&
          (in_data[PW_i + 14] == 0x20) &&
          (in_data[PW_i + 15] == 0x4A))
      {
        if (testTryIt101() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_TryIt101();
        }
      }
#endif
      break;

    case 'L': /* 0x4C */
#ifdef INCLUDEALL
      /* "LSD!" : ID of Automation 2.3r packer (Atari ST) */
      if ((in_data[PW_i + 1] == 'S') &&
          (in_data[PW_i + 2] == 'D') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Automation 2.3r Data Cruncher", 4, LSDDataCruncher);
        }
      }

      /* "LZH!" : ID of Jam Packer (LZH! compression) (Atari ST) */
      if ((in_data[PW_i + 1] == 'Z') &&
          (in_data[PW_i + 2] == 'H') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Jam Packer Data Cruncher", 12, JamDataCruncher);
        }
      }
#endif
      break;

    case 'M': /* 0x4D */
      /* MASM data cruncher */
      if ((in_data[PW_i + 1] == 'A') &&
          (in_data[PW_i + 2] == 'S') &&
          (in_data[PW_i + 3] == 'M'))
      {
        if (testSpecialCruncherData(4, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("MASM Data Cruncher", 0, MASMDataCruncher);
        }
      }

#ifdef INCLUDEALL
      /* Defjam Cruncher 3.2 pro */
      if ((in_data[PW_i + 1] == 0xF9) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xDF) &&
          (in_data[PW_i + 4] == 0xF0) &&
          (in_data[PW_i + 5] == 0x00) &&
          (in_data[PW_i + 6] == 0x7E) &&
          (in_data[PW_i + 7] == 0x00) &&
          (in_data[PW_i + 8] == 0x30) &&
          (in_data[PW_i + 9] == 0x3C) &&
          (in_data[PW_i + 10] == 0x7F) &&
          (in_data[PW_i + 11] == 0xFF) &&
          (in_data[PW_i + 12] == 0x3D) &&
          (in_data[PW_i + 13] == 0x40) &&
          (in_data[PW_i + 14] == 0x00) &&
          (in_data[PW_i + 15] == 0x96))
      {
        if (testDefjam32pro() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_Defjam32();
        }
      }

      /* Mental Image Packer (data) */
      if ((in_data[PW_i + 1] == 'I') &&
          (in_data[PW_i + 2] == '1') &&
          (in_data[PW_i + 3] == '0'))
      {
        if (testSpecialCruncherData(12, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Mental Image Packer", 18, MentalImage);
        }
      }
#endif
      break;

    case 'P': /* 0x50 */
#ifdef INCLUDEALL
      /* PARA data Cruncher */
      if ((in_data[PW_i + 1] == 'A') &&
          (in_data[PW_i + 2] == 'R') &&
          (in_data[PW_i + 3] == 'A'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("PARA Data Cruncher", 46, PARA);
        }
      }

      /* Pac1 data Cruncher */
      if ((in_data[PW_i + 1] == 'a') &&
          (in_data[PW_i + 2] == 'c') &&
          (in_data[PW_i + 3] == '1'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Pac1 Data Cruncher", 12, Pac1);
        }
      }

      /* Master cruncher 3.0 data */
      if ((in_data[PW_i + 1] == 'A') &&
          (in_data[PW_i + 2] == 'C') &&
          (in_data[PW_i + 3] == 'K') &&
          (in_data[PW_i + 4] == 'V') &&
          (in_data[PW_i + 5] == '1') &&
          (in_data[PW_i + 6] == '.') &&
          (in_data[PW_i + 7] == '2'))
      {
        if (testSpecialCruncherData(12, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Master Cruncher 3.0 data", 8, MasterCruncher3data);
        }
      }
#endif
      break;

    case 'R': /* RNC */
#ifdef INCLUDEALL
      if ((in_data[PW_i + 1] == 'N') &&
          (in_data[PW_i + 2] == 'C'))
      {
        /* RNC */
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Propack (RNC) data", 18, RNC);
        }
      }
      /* RLE Data Cruncher */
      if ((in_data[PW_i + 1] == 'L') &&
          (in_data[PW_i + 2] == 'E'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("RLE Data Cruncher", 11, RLE);
        }
      }
#endif
      break;

    case 'S': /* 0x53 */
#ifdef INCLUDEALL
      /* SpeedPacker (SP20) Data Cruncher */
      if ((in_data[PW_i + 1] == 'P') &&
          (in_data[PW_i + 2] == '2') &&
          (in_data[PW_i + 3] == '0'))
      {
        if (testSpecialCruncherData(8, 12) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("SpeedPacker (SP20) Data Cruncher", 16, SP20);
        }
      }

      /* S404 StoneCracker 4.04 data */
      if ((in_data[PW_i + 1] == '4') &&
          (in_data[PW_i + 2] == '0') &&
          (in_data[PW_i + 3] == '4'))
      {
        if (testSpecialCruncherData(12, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("StoneCracker 4.04 data", 18, S404);
        }
      }

      /* S403 StoneCracker 4.03 data */
      if ((in_data[PW_i + 1] == '4') &&
          (in_data[PW_i + 2] == '0') &&
          (in_data[PW_i + 3] == '3'))
      {
        if (testSpecialCruncherData(12, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("StoneCracker 4.03 data", 20, S404);
        }
      }

      /* S401 StoneCracker 4.01 data */
      if ((in_data[PW_i + 1] == '4') &&
          (in_data[PW_i + 2] == '0') &&
          (in_data[PW_i + 3] == '1'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("StoneCracker 4.01 data", 12, S404);
        }
      }
      /* S400 StoneCracker 4.00 data */
      if ((in_data[PW_i + 1] == '4') &&
          (in_data[PW_i + 2] == '0') &&
          (in_data[PW_i + 3] == '0'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("StoneCracker 4.00 data", 12, S404);
        }
      }

      /* S310 StoneCracker 3.10 data */
      if ((in_data[PW_i + 1] == '3') &&
          (in_data[PW_i + 2] == '1') &&
          (in_data[PW_i + 3] == '0'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("StoneCracker 3.10 data", 16, S404);
        }
      }

      /* S300 StoneCracker 3.00 data */
      if ((in_data[PW_i + 1] == '3') &&
          (in_data[PW_i + 2] == '0') &&
          (in_data[PW_i + 3] == '0'))
      {
        if (testSpecialCruncherData(12, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("StoneCracker 3.00 data", 16, S404);
        }
      }

      /* SPv3 - Speed Packer 3 (Atari ST) */
      if ((in_data[PW_i + 1] == 'P') &&
          (in_data[PW_i + 2] == 'v') &&
          (in_data[PW_i + 3] == '3'))
      {
        if (testSpecialCruncherData(8, 12) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Speed Packer 3 data", 0, SpeedPacker3Data);
        }
      }
#endif
      break;

    case 'V': /* "V.2" */
#ifdef INCLUDEALL
      /* Virtual Dreams VDCO data cruncher */
      if ((in_data[PW_i + 1] == 'D') &&
          (in_data[PW_i + 2] == 'C') &&
          (in_data[PW_i + 3] == 'O'))
      {
        if (testSpecialCruncherData(12, 8) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("Virtual Dreams (VDCO) data cruncher", 13, VDCO);
        }
      }
#endif
      break;

    case 'X': /* XPKF */
#ifdef INCLUDEALL
      /* xpk'ed file */
      if ((in_data[PW_i + 1] == 'P') &&
          (in_data[PW_i + 2] == 'K') &&
          (in_data[PW_i + 3] == 'F'))
      {
        if (testSpecialCruncherData(4, 12) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("XPK", 8, XPK);
        }
      }
#endif
      break;

    case 0x61: /* "a" */
#ifdef INCLUDEALL
      /* TNM Cruncher 1.1 */
      if ((in_data[PW_i + 1] == 0x06) &&
          (in_data[PW_i + 2] == 0x4E) &&
          (in_data[PW_i + 3] == 0xF9) &&
          (in_data[PW_i + 4] == 0x00) &&
          (in_data[PW_i + 5] == 0x00) &&
          (in_data[PW_i + 6] == 0x00) &&
          (in_data[PW_i + 7] == 0x00) &&
          (in_data[PW_i + 8] == 0x48) &&
          (in_data[PW_i + 9] == 0xE7) &&
          (in_data[PW_i + 10] == 0xFF) &&
          (in_data[PW_i + 11] == 0xFE) &&
          (in_data[PW_i + 12] == 0x2C) &&
          (in_data[PW_i + 13] == 0x78) &&
          (in_data[PW_i + 14] == 0x00) &&
          (in_data[PW_i + 15] == 0x04))
      {
        if (testTNMCruncher11() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_TNMCruncher11();
        }
      }

      /* "arcD" data cruncher */
      if ((in_data[PW_i + 1] == 'r') &&
          (in_data[PW_i + 2] == 'c') &&
          (in_data[PW_i + 3] == 'D'))
      {
        if (testArcDDataCruncher() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("arcD data Cruncher", 0, arcD);
        }
      }

      /* High Pressure Cruncher */
      if ((in_data[PW_i + 1] == 0x00) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xA8) &&
          (in_data[PW_i + 4] == 0x20) &&
          (in_data[PW_i + 5] == 0x7C) &&
          (in_data[PW_i + 10] == 0x22) &&
          (in_data[PW_i + 11] == 0x7c) &&
          (in_data[PW_i + 16] == 0x24) &&
          (in_data[PW_i + 17] == 0x48) &&
          (in_data[PW_i + 18] == 0x26) &&
          (in_data[PW_i + 19] == 0x49) &&
          (in_data[PW_i + 20] == 0x61))
      {
        if (testHighPressureCruncher() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_HighPressureCruncher();
        }
      }
#endif
      break;

    case 'x': /* xVdg */
#ifdef INCLUDEALL
      /* AMOS sub file */
      if ((in_data[PW_i + 1] == 'V') &&
          (in_data[PW_i + 2] == 'd') &&
          (in_data[PW_i + 3] == 'g'))
      {
        if (testSpecialCruncherData(8, 4) != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_SpecialCruncherData("xVdg", 12, xVdg);
        }
      }
#endif
      break;

    case 0x7E:
#ifdef INCLUDEALL
      /* Defjam Cruncher 3.2T */
      if ((in_data[PW_i + 1] == 0x00) &&
          (in_data[PW_i + 2] == 0x43) &&
          (in_data[PW_i + 3] == 0xFA) &&
          (in_data[PW_i + 4] == 0x02) &&
          (in_data[PW_i + 5] == 0x8C) &&
          (in_data[PW_i + 6] == 0x4B) &&
          (in_data[PW_i + 7] == 0xF9) &&
          (in_data[PW_i + 8] == 0x00) &&
          (in_data[PW_i + 9] == 0xDF) &&
          (in_data[PW_i + 10] == 0xF1))
      {
        if (testDefjam32t() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_Defjam32();
        }
      }
#endif
      break;

    case 0xB4:
#ifdef INCLUDEALL
      /* DIET PC data packer */
      if ((in_data[PW_i + 1] == 0x4C) &&
          (in_data[PW_i + 2] == 0xCD) &&
          (in_data[PW_i + 3] == 0x21) &&
          (in_data[PW_i + 4] == 0x9D) &&
          (in_data[PW_i + 5] == 0x89) &&
          (((in_data[PW_i + 6] == 0x64) &&
            (in_data[PW_i + 7] == 0x6C) &&
            (in_data[PW_i + 8] == 0x7A)) ||
           ((in_data[PW_i + 6] == 0x45) &&
            (in_data[PW_i + 7] == 0x4F) &&
            (in_data[PW_i + 8] == 0x53))))
      {
        if (testDietDataPacker() != BAD)
        {
          Current_Is_Module = BAD;
          if (Do_Rip == GOOD)
            Rip_DietDataPacker();
        }
      }
#endif
      break;

    default: /* do nothing ... save continuing :) */
      break;

    } /* end of switch */
}
