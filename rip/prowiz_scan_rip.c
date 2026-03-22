/*
 * prowiz_scan_rip.c — module format detection (rip mode library)
 */

#include "globals.h"
#include "extern.h"

void PW_ScanRipAt(void)
{
    if (in_data[PW_i] <= 0x40)
    {
      /* "!PM!" : ID of Power Music */
      if ((in_data[PW_i] == '!') &&
          (in_data[PW_i + 1] == 'P') &&
          (in_data[PW_i + 2] == 'M') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testPM() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_PM();
          if (Do_Depack == GOOD)
            Depack_PM();
        }
      }
      /* Treasure Patterns ?*/
      /* 0x4000 : ID of Treasure Patterns ? not sure */
      /*
      if ( (in_data[PW_i]   == 0x00) )
      {
        if ( testTreasure() != BAD )
        {
          if(Do_Rip==GOOD)Rip_Treasure();
          if(Do_Depack==GOOD)Depack_Treasure();
        }
      }
      */
#ifdef INCLUDEALL
      /* -GD- GnoiPacker (Skizzo)*/
      if ((in_data[PW_i] == 0x2D) &&
          (in_data[PW_i + 1] == 'G') &&
          (in_data[PW_i + 2] == 'D') &&
          (in_data[PW_i + 3] == 0x2D))
      {
        if (testSkizzo() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Skizzo();
          if (Do_Depack == GOOD)
            Depack_Skizzo();
        }
      }
#endif
      /* "[1-9]CHN" FastTracker v1 */
      if (((in_data[PW_i] == '1') ||
           (in_data[PW_i] == '2') ||
           (in_data[PW_i] == '3') ||
           (in_data[PW_i] == '4') ||
           (in_data[PW_i] == '5') ||
           (in_data[PW_i] == '6') ||
           (in_data[PW_i] == '7') ||
           (in_data[PW_i] == '8') ||
           (in_data[PW_i] == '9')) &&
          (in_data[PW_i + 1] == 'C') &&
          (in_data[PW_i + 2] == 'H') &&
          (in_data[PW_i + 3] == 'N'))
      {
        if (testMOD(in_data[PW_i] - 0x30) != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MOD(in_data[PW_i] - 0x30);
        }
      }
      /* "[10-32]CH" FastTracker v1/v2 */
      if (((((in_data[PW_i] == '1') || (in_data[PW_i] == '2')) &&
            ((in_data[PW_i + 1] == '0') ||
             (in_data[PW_i + 1] == '1') ||
             (in_data[PW_i + 1] == '2') ||
             (in_data[PW_i + 1] == '3') ||
             (in_data[PW_i + 1] == '4') ||
             (in_data[PW_i + 1] == '5') ||
             (in_data[PW_i + 1] == '6') ||
             (in_data[PW_i + 1] == '7') ||
             (in_data[PW_i + 1] == '8') ||
             (in_data[PW_i + 1] == '9'))) ||
           ((in_data[PW_i] == '3') &&
            ((in_data[PW_i + 1] == '0') ||
             (in_data[PW_i + 1] == '1')))) &&
          (in_data[PW_i + 2] == 'C') &&
          (in_data[PW_i + 3] == 'H'))
      {
        if (testMOD((in_data[PW_i] - 0x30) * 10 + in_data[PW_i + 1] - 0x30) != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MOD((in_data[PW_i] - 0x30) * 10 + in_data[PW_i + 1] - 0x30);
        }
      }

      /* XANN packer */
      if (in_data[PW_i] == 0x3c)
      {
        if (testXANN() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_XANN();
          if (Do_Depack == GOOD)
            Depack_XANN();
        }
      }

      /* hum ... that's where things become interresting :) */
      /* Module Protector without ID */
      /* LEAVE IT THERE !!! ... at least before Heatseeker format since they are VERY similare ! */
      if (testMP_noID() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_MP_noID();
        if (Do_Depack == GOOD)
          Depack_MP();
      }

      /* Digital Illusion */
      if (testDI() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_DI();
        if (Do_Depack == GOOD)
          Depack_DI();
      }

      /* SGTPacker */
      /*
      if ( testSGT() != BAD )
      {
        if(Do_Rip==GOOD)Rip_SGT();
        if(Do_Depack==GOOD)Depack_SGT();
      }
      */
      /* eureka packer */
      if (testEUREKA() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_EUREKA();
        if (Do_Depack == GOOD)
          Depack_EUREKA();
      }

      /* The player 5.0a ? */
      if (testP50A() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_P50A();
        if (Do_Depack == GOOD)
          Depack_P50A();
      }

      /* The player 6.0a ? */
      if (testP60A_nopack() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_P60A();
        if (Do_Depack == GOOD)
          Depack_P60A();
      }

      /* The player 6.0a (packed samples)? */
      if (testP60A_pack() != BAD)
      {
        printf("\b\b\b\b\b\b\b\bThe Player 6.0A with PACKED samples found at %lld ... cant rip it!\n", (long long)PW_Start_Address);
        /*if(Do_Rip==GOOD)Rip_P60A();*/
        /*if(Do_Depack==GOOD)Depack_P60A();*/
      }

      /* The player 6.1a ? */
      if (testP61A_nopack() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_P61A();
        if (Do_Depack == GOOD)
          Depack_P61A();
      }

      /* The player 6.1a (packed samples)? */
      if (testP61A_pack() != BAD)
      {
        printf("\b\b\b\b\b\b\b\bThe Player 6.1A with PACKED samples found at %lld ... cant rip it!\n", (long long)PW_Start_Address);
        /*if(Do_Rip==GOOD)Rip_P61A();*/
        /*if(Do_Depack==GOOD)Depack_P61A();*/
      }

      /* Propacker 1.0 */
      if (testPP10() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_PP10();
        if (Do_Depack == GOOD)
          Depack_PP10();
      }

      /* Noise Packer v2 */
      /* LEAVE VERSION 2 BEFORE VERSION 1 !!!!! */
      if (testNoisepacker2() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_Noisepacker2();
        if (Do_Depack == GOOD)
          Depack_Noisepacker2();
      }

      /* Noise Packer v1 */
      if (testNoisepacker1() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_Noisepacker1();
        if (Do_Depack == GOOD)
          Depack_Noisepacker1();
      }

      /* Noise Packer v3 */
      if (testNoisepacker3() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_Noisepacker3();
        if (Do_Depack == GOOD)
          Depack_Noisepacker3();
      }

      /* Promizer 0.1 */
      if (testPM01() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_PM01();
        if (Do_Depack == GOOD)
          Depack_PM01();
      }

      /* ProPacker 2.1 */
      if (testPP21() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_PP21();
        if (Do_Depack == GOOD)
          Depack_PP21();
      }

      /* ProPacker 3.0 */
      if (testPP30() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_PP30();
        if (Do_Depack == GOOD)
          Depack_PP30();
      }

      /* StartTrekker pack */
      if (testSTARPACK() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_STARPACK();
        if (Do_Depack == GOOD)
          Depack_STARPACK();
      }

      /* Zen packer */
      if (testZEN() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_ZEN();
        if (Do_Depack == GOOD)
          Depack_ZEN();
      }

      /* Unic tracker v1 ? */
      if (testUNIC_withemptyID() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_UNIC_withID();
        if (Do_Depack == GOOD)
          Depack_UNIC();
      }

      /* Unic tracker v1 ? */
      if (testUNIC_noID() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_UNIC_noID();
        if (Do_Depack == GOOD)
          Depack_UNIC();
      }

      /* Unic trecker v2 ? */
      if (testUNIC2() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_UNIC2();
        if (Do_Depack == GOOD)
          Depack_UNIC2();
      }

      /* Game Music Creator ? */
      if (testGMC() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_GMC();
        if (Do_Depack == GOOD)
          Depack_GMC();
      }

      /* Heatseeker ? */
      if (testHEATSEEKER() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_HEATSEEKER();
        if (Do_Depack == GOOD)
          Depack_HEATSEEKER();
      }

      /* SoundTracker (15 smp) */
      if (testSoundTracker() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_SoundTracker();
      }

      /* The Dark Demon (group name) format */
      if (testTheDarkDemon() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_TheDarkDemon();
        if (Do_Depack == GOOD)
          Depack_TheDarkDemon();
      }

      /* Newtron */
      if (testNewtron() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_Newtron();
        if (Do_Depack == GOOD)
          Depack_Newtron();
      }

      /* Newtron Old */
      if (testNewtronOld() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_NewtronOld();
        if (Do_Depack == GOOD)
          Depack_NewtronOld();
      }

      /* Titanics Player ? */
      if (testTitanicsPlayer() != BAD)
      {
        if (Do_Rip == GOOD)
          Rip_TitanicsPlayer();
        if (Do_Depack == GOOD)
          Depack_TitanicsPlayer();
      }

      /* Struggle game ? */
      /*      if ( testSTRUGGLE() != BAD )
            {
              if(Do_Rip==GOOD)Rip_STRUGGLE();
              if(Do_Depack==GOOD)Depack_STRUGGLE();
              continue;
            }*/
    }

    /**********************************/
    /* ok, now, the files with ID ... */
    /**********************************/
    switch (in_data[PW_i])
    {
    case 'A': /* ATN! another Imploder case */
      /* AMOS Music bank "AmBk" */
      if ((in_data[PW_i + 1] == 'm') &&
          (in_data[PW_i + 2] == 'B') &&
          (in_data[PW_i + 3] == 'k'))
      {
        if (testAmBk() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_AmBk();
          if (Do_Depack == GOOD)
            Depack_AmBk();
        }
      }
      /* Sidmon v1 */
      if (in_data[PW_i + 1] == 0xFA)
      {
        if (testSIDMON1() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_SIDMON1();
        }
      }
#ifdef INCLUDEALL
      /* Tetrapack 1.02 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xE6) &&
          (in_data[PW_i + 4] == 0xD1) &&
          (in_data[PW_i + 5] == 0xFC) &&
          (in_data[PW_i + 10] == 0x22) &&
          (in_data[PW_i + 11] == 0x7C) &&
          (in_data[PW_i + 16] == 0x24) &&
          (in_data[PW_i + 17] == 0x60) &&
          (in_data[PW_i + 18] == 0xD5) &&
          (in_data[PW_i + 19] == 0xC9))
      {
        if (testTetrapack102() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Tetrapack102();
        }
      }
      /* Tetrapack 1.01 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xDE) &&
          (in_data[PW_i + 4] == 0xD1) &&
          (in_data[PW_i + 5] == 0xFC) &&
          (in_data[PW_i + 10] == 0x22) &&
          (in_data[PW_i + 11] == 0x7C) &&
          (in_data[PW_i + 16] == 0x24) &&
          (in_data[PW_i + 17] == 0x60) &&
          (in_data[PW_i + 18] == 0xD5) &&
          (in_data[PW_i + 19] == 0xC9))
      {
        if (testTetrapack101() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Tetrapack101();
        }
      }
      /* HQC Cruncher 2.0 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x06) &&
          (in_data[PW_i + 3] == 0x76) &&
          (in_data[PW_i + 4] == 0x20) &&
          (in_data[PW_i + 5] == 0x80) &&
          (in_data[PW_i + 6] == 0x41) &&
          (in_data[PW_i + 7] == 0xFA) &&
          (in_data[PW_i + 8] == 0x06) &&
          (in_data[PW_i + 9] == 0x64) &&
          (in_data[PW_i + 10] == 0x43) &&
          (in_data[PW_i + 11] == 0xFA) &&
          (in_data[PW_i + 12] == 0x05) &&
          (in_data[PW_i + 13] == 0x10) &&
          (in_data[PW_i + 14] == 0x20) &&
          (in_data[PW_i + 15] == 0x89))
      {
        if (testHQCCruncher2() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_HQCCruncher2();
        }
      }
      /* ByteKillerPro 1.0 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xDC) &&
          (in_data[PW_i + 4] == 0x2C) &&
          (in_data[PW_i + 5] == 0x78) &&
          (in_data[PW_i + 6] == 0x00) &&
          (in_data[PW_i + 7] == 0x04) &&
          (in_data[PW_i + 12] == 0x43) &&
          (in_data[PW_i + 13] == 0xF9) &&
          (in_data[PW_i + 18] == 0x20) &&
          (in_data[PW_i + 19] == 0x10) &&
          (in_data[PW_i + 20] == 0x22) &&
          (in_data[PW_i + 21] == 0x28))
      {
        if (testbytekillerpro10() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_bytekillerpro10();
        }
      }
#endif
      /* Delta Music 1 */
      if ((in_data[PW_i + 1] == 'L') &&
          (in_data[PW_i + 2] == 'L') &&
          (in_data[PW_i + 3] == ' '))
      {
        if (testDM1() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_DM1();
        }
      }

      break;

    case 'B': /* BTB6 */
      /* "BeEp" Jam Cracker */
      if ((in_data[PW_i + 1] == 'e') &&
          (in_data[PW_i + 2] == 'E') &&
          (in_data[PW_i + 3] == 'p'))
      {
        if (testJamCracker() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_JamCracker();
        }
      }

      /* "BNR!" Binary Packer */
      if ((in_data[PW_i + 1] == 'N') &&
          (in_data[PW_i + 2] == 'R') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testBNR() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_BNR();
          if (Do_Depack == GOOD)
            Depack_BNR();
        }
      }
      break;

    case 'C': /* 0x43 */
              /* CPLX_TP3 ?!? */
      if ((in_data[PW_i + 1] == 'P') &&
          (in_data[PW_i + 2] == 'L') &&
          (in_data[PW_i + 3] == 'X') &&
          (in_data[PW_i + 4] == '_') &&
          (in_data[PW_i + 5] == 'T') &&
          (in_data[PW_i + 6] == 'P') &&
          (in_data[PW_i + 7] == '3'))
      {
        if (testTP3() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_TP3();
          if (Do_Depack == GOOD)
            Depack_TP3();
        }
      }
      break;

    case 'D': /* 0x44 */
      /* Digibooster 1.7 */
      if ((in_data[PW_i + 1] == 'I') &&
          (in_data[PW_i + 2] == 'G') &&
          (in_data[PW_i + 3] == 'I'))
      {
        if (testDigiBooster17() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_DigiBooster17();
        }
      }
      break;

    case 'E': /* 0x45 */
      /* "EMOD" : ID of Quadra Composer */
      if ((in_data[PW_i + 1] == 'M') &&
          (in_data[PW_i + 2] == 'O') &&
          (in_data[PW_i + 3] == 'D'))
      {
        if (testQuadraComposer() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_QuadraComposer();
          /*if(Do_Depack==GOOD)Depack_QuadraComposer();*/
        }
      }
      /* "Extended Module" : ID of FastTracker 2 XM */
      if ((in_data[PW_i + 1] == 'x') &&
          (in_data[PW_i + 2] == 't') &&
          (in_data[PW_i + 3] == 'e') &&
          (in_data[PW_i + 4] == 'n') &&
          (in_data[PW_i + 5] == 'd') &&
          (in_data[PW_i + 6] == 'e') &&
          (in_data[PW_i + 7] == 'd') &&
          (in_data[PW_i + 8] == ' ') &&
          (in_data[PW_i + 9] == 'M') &&
          (in_data[PW_i + 10] == 'o'))
      {
        if (testXM() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_XM();
        }
      }
      break;

    case 'F': /* 0x46 */
      /* "FC-M" : ID of FC-M packer */
      if ((in_data[PW_i + 1] == 'C') &&
          (in_data[PW_i + 2] == '-') &&
          (in_data[PW_i + 3] == 'M'))
      {
        if (testFC_M() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_FC_M();
          if (Do_Depack == GOOD)
            Depack_FC_M();
        }
      }
      /* "FLT4" : ID of StarTrekker */
      if ((in_data[PW_i + 1] == 'L') &&
          (in_data[PW_i + 2] == 'T') &&
          (in_data[PW_i + 3] == '4'))
      {
        if (testMOD(4) != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MOD(4);
        }
      }
      /* "FC14" : Future Composer 1.4 */
      if ((in_data[PW_i + 1] == 'C') &&
          (in_data[PW_i + 2] == '1') &&
          (in_data[PW_i + 3] == '4'))
      {
        if (testFC14() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_FC14();
        }
      }
      /* "FUCO" : ID of BSI Future Composer */
      if ((in_data[PW_i + 1] == 'U') &&
          (in_data[PW_i + 2] == 'C') &&
          (in_data[PW_i + 3] == 'O'))
      {
        if (testBSIFutureComposer() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_BSIFutureComposer();
        }
      }
      /* "Fuck" : ID of Noise From Heaven chiptunes */
      if ((in_data[PW_i + 1] == 'u') &&
          (in_data[PW_i + 2] == 'c') &&
          (in_data[PW_i + 3] == 'k'))
      {
        if (testNFH() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_NFH();
          if (Do_Depack == GOOD)
            Depack_NFH();
        }
      }
      /* "FAST" : ID of Stone Arts Player */
      if ((in_data[PW_i + 1] == 'A') &&
          (in_data[PW_i + 2] == 'S') &&
          (in_data[PW_i + 3] == 'T'))
      {
        if (testStoneArtsPlayer() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_StoneArtsPlayer();
          if (Do_Depack == GOOD)
            Depack_StoneArtsPlayer();
        }
      }
      break;

    case 'G': /* 0x47 */
#ifdef INCLUDEALL
      /* Mega Cruncher 1.0 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x01) &&
          (in_data[PW_i + 3] == 0x2E) &&
          (in_data[PW_i + 4] == 0x20) &&
          (in_data[PW_i + 5] == 0x0B) &&
          (in_data[PW_i + 6] == 0x22) &&
          (in_data[PW_i + 7] == 0x2B) &&
          (in_data[PW_i + 8] == 0x00) &&
          (in_data[PW_i + 9] == 0x08) &&
          (in_data[PW_i + 10] == 0x41) &&
          (in_data[PW_i + 11] == 0xFA) &&
          (in_data[PW_i + 12] == 0x01) &&
          (in_data[PW_i + 13] == 0x30) &&
          (in_data[PW_i + 14] == 0xD1) &&
          (in_data[PW_i + 15] == 0xC1))
      {
        if (testMegaCruncher10() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MegaCruncher();
        }
      }

      /* Mega Cruncher 1.2 */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x01) &&
          (in_data[PW_i + 3] == 0x32) &&
          (in_data[PW_i + 4] == 0x20) &&
          (in_data[PW_i + 5] == 0x0B) &&
          (in_data[PW_i + 6] == 0x22) &&
          (in_data[PW_i + 7] == 0x2B) &&
          (in_data[PW_i + 8] == 0x00) &&
          (in_data[PW_i + 9] == 0x08) &&
          (in_data[PW_i + 10] == 0x41) &&
          (in_data[PW_i + 11] == 0xFA) &&
          (in_data[PW_i + 12] == 0x01) &&
          (in_data[PW_i + 13] == 0x34) &&
          (in_data[PW_i + 14] == 0xD1) &&
          (in_data[PW_i + 15] == 0xC1))
      {
        if (testMegaCruncher12() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MegaCruncher();
        }
      }

      /* Double Action v1.0 */
      if ((in_data[PW_i + 1] == 0xF9) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xDF) &&
          (in_data[PW_i + 137] == 0xAB) &&
          (in_data[PW_i + 138] == 0xD1) &&
          (in_data[PW_i + 139] == 0xC0) &&
          (in_data[PW_i + 140] == 0xD3) &&
          (in_data[PW_i + 141] == 0xC0) &&
          (in_data[PW_i + 142] == 0x23) &&
          (in_data[PW_i + 143] == 0x20))
      {
        if (testDoubleAction10() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_DoubleAction10();
        }
      }
#endif
      /* GPMO (crunch player ?)*/
      if ((in_data[PW_i + 1] == 'P') &&
          (in_data[PW_i + 2] == 'M') &&
          (in_data[PW_i + 3] == 'O'))
      {
        if (testGPMO() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_GPMO();
          if (Do_Depack == GOOD)
            Depack_GPMO();
        }
      }

      /* Gnu player */
      if ((in_data[PW_i + 1] == 'n') &&
          (in_data[PW_i + 2] == 'P') &&
          (in_data[PW_i + 3] == 'l'))
      {
        if (testGnuPlayer() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_GnuPlayer();
          if (Do_Depack == GOOD)
            Depack_GnuPlayer();
        }
      }

      break;

    case 'H': /* 0x48 */
      /* "HRT!" : ID of Hornet packer */
      if ((in_data[PW_i + 1] == 'R') &&
          (in_data[PW_i + 2] == 'T') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testHRT() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_HRT();
          if (Do_Depack == GOOD)
            Depack_HRT();
        }
      }

#ifdef INCLUDEALL
      /* Master Cruncher 3.0 Address */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFE) &&
          (in_data[PW_i + 4] == 0x4B) &&
          (in_data[PW_i + 5] == 0xFA) &&
          (in_data[PW_i + 6] == 0x01) &&
          (in_data[PW_i + 7] == 0x80) &&
          (in_data[PW_i + 8] == 0x41) &&
          (in_data[PW_i + 9] == 0xFA) &&
          (in_data[PW_i + 10] == 0xFF) &&
          (in_data[PW_i + 11] == 0xF2) &&
          (in_data[PW_i + 12] == 0x22) &&
          (in_data[PW_i + 13] == 0x50) &&
          (in_data[PW_i + 14] == 0xD3) &&
          (in_data[PW_i + 15] == 0xC9))
      {
        if (testMasterCruncher30addr() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MasterCruncher30addr();
        }
      }

      /* Crunchmania Address */
      if ((in_data[PW_i + 1] == 0xe7) &&
          (in_data[PW_i + 14] == 0x22) &&
          (in_data[PW_i + 15] == 0x1A) &&
          (in_data[PW_i + 16] == 0x24) &&
          (in_data[PW_i + 17] == 0x1A) &&
          (in_data[PW_i + 18] == 0x47) &&
          (in_data[PW_i + 19] == 0xEA) &&
          (in_data[PW_i + 24] == 0x6F) &&
          (in_data[PW_i + 25] == 0x1C) &&
          (in_data[PW_i + 26] == 0x26) &&
          (in_data[PW_i + 27] == 0x49) &&
          (in_data[PW_i + 28] == 0xD7) &&
          (in_data[PW_i + 29] == 0xC1) &&
          (in_data[PW_i + 30] == 0xB7) &&
          (in_data[PW_i + 31] == 0xCA))
      {
        if (testcrunchmaniaAddr(1) != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_CrunchmaniaAddr();
        }
      }

      /* Crunchmania Address (another)*/
      if ((in_data[PW_i + 1] == 0xe7) &&
          (in_data[PW_i + 14] == 0x20) &&
          (in_data[PW_i + 15] == 0x4C) &&
          (in_data[PW_i + 16] == 0x47) &&
          (in_data[PW_i + 17] == 0xFA) &&
          (in_data[PW_i + 18] == 0x00) &&
          (in_data[PW_i + 19] == 0x0C) &&
          (in_data[PW_i + 24] == 0x51) &&
          (in_data[PW_i + 25] == 0xCF) &&
          (in_data[PW_i + 26] == 0xFF) &&
          (in_data[PW_i + 27] == 0xFC) &&
          (in_data[PW_i + 28] == 0x4E) &&
          (in_data[PW_i + 29] == 0xD0) &&
          (in_data[PW_i + 30] == 0x43) &&
          (in_data[PW_i + 31] == 0xF9))
      {
        if (testcrunchmaniaAddr(1) != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_CrunchmaniaAddr();
        }
      }

      /* Crunchmania Address (another again)*/
      if ((in_data[PW_i + 1] == 0xe7) &&
          (in_data[PW_i + 14] == 0x1a) &&
          (in_data[PW_i + 15] == 0xbc) &&
          (in_data[PW_i + 16] == 0x00) &&
          (in_data[PW_i + 17] == 0xb9) &&
          (in_data[PW_i + 18] == 0x1a) &&
          (in_data[PW_i + 19] == 0xbc) &&
          (in_data[PW_i + 24] == 0x00) &&
          (in_data[PW_i + 25] == 0xe9) &&
          (in_data[PW_i + 26] == 0x1a) &&
          (in_data[PW_i + 27] == 0xbc) &&
          (in_data[PW_i + 28] == 0x00) &&
          (in_data[PW_i + 29] == 0xf1) &&
          (in_data[PW_i + 30] == 0x45) &&
          (in_data[PW_i + 31] == 0xfa))
      {
        if (testcrunchmaniaAddr(2) != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_CrunchmaniaAddr();
        }
      }

      /* Crunchmania Simple */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFF) &&
          (in_data[PW_i + 4] == 0x45) &&
          (in_data[PW_i + 5] == 0xFA) &&
          (in_data[PW_i + 6] == 0x01) &&
          (in_data[PW_i + 7] == 0x66) &&
          (in_data[PW_i + 8] == 0x22) &&
          (in_data[PW_i + 9] == 0x1A) &&
          (in_data[PW_i + 10] == 0x24) &&
          (in_data[PW_i + 11] == 0x1A) &&
          (in_data[PW_i + 12] == 0x22) &&
          (in_data[PW_i + 13] == 0x4A) &&
          (in_data[PW_i + 14] == 0x28) &&
          (in_data[PW_i + 15] == 0x7A))
      {
        if (testcrunchmaniaSimple() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_CrunchmaniaSimple();
        }
      }

      /* Mega Cruncher Obj */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFE) &&
          (in_data[PW_i + 4] == 0x2C) &&
          (in_data[PW_i + 5] == 0x78) &&
          (in_data[PW_i + 6] == 0x00) &&
          (in_data[PW_i + 7] == 0x04) &&
          (in_data[PW_i + 8] == 0x4B) &&
          (in_data[PW_i + 9] == 0xFA) &&
          (in_data[PW_i + 10] == 0x01) &&
          (in_data[PW_i + 11] == 0xC0))
      {
        if (testMegaCruncherObj() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MegaCruncherObj();
        }
      }

      /* Turbo Squeezer 6.1 */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFE) &&
          (in_data[PW_i + 4] == 0x2C) &&
          (in_data[PW_i + 5] == 0x79) &&
          (in_data[PW_i + 6] == 0x00) &&
          (in_data[PW_i + 7] == 0x00) &&
          (in_data[PW_i + 8] == 0x00) &&
          (in_data[PW_i + 9] == 0x04) &&
          (in_data[PW_i + 10] == 0x20) &&
          (in_data[PW_i + 11] == 0x7A))
      {
        if (testTurboSqueezer61() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_TurboSqueezer61();
        }
      }

      /* DragPack 2.52 */
      if ((in_data[PW_i + 1] == 0x7A) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0x46) &&
          (in_data[PW_i + 4] == 0x48) &&
          (in_data[PW_i + 5] == 0xE7) &&
          (in_data[PW_i + 6] == 0xFF) &&
          (in_data[PW_i + 7] == 0xFE) &&
          (in_data[PW_i + 8] == 0x49) &&
          (in_data[PW_i + 9] == 0xFA) &&
          (in_data[PW_i + 10] == 0xFF) &&
          (in_data[PW_i + 11] == 0xEE) &&
          (in_data[PW_i + 12] == 0x28) &&
          (in_data[PW_i + 13] == 0xFC) &&
          (in_data[PW_i + 14] == 0x00) &&
          (in_data[PW_i + 15] == 0x00))
      {
        if (testDragpack252() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Dragpack252();
        }
      }
      /* DragPack 1.00 */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFE) &&
          (in_data[PW_i + 4] == 0x41) &&
          (in_data[PW_i + 5] == 0xF9) &&
          (in_data[PW_i + 6] == 0x00) &&
          (in_data[PW_i + 7] == 0x00) &&
          (in_data[PW_i + 8] == 0x00) &&
          (in_data[PW_i + 9] == 0x00) &&
          (in_data[PW_i + 10] == 0x43) &&
          (in_data[PW_i + 11] == 0xF9) &&
          (in_data[PW_i + 12] == 0x00) &&
          (in_data[PW_i + 13] == 0x00) &&
          (in_data[PW_i + 14] == 0x00) &&
          (in_data[PW_i + 15] == 0x00))
      {
        if (testDragpack100() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Dragpack100();
        }
      }
      /* GNU Packer 1.2 */
      if ((in_data[PW_i + 1] == 0xE7) &&
          (in_data[PW_i + 2] == 0xFF) &&
          (in_data[PW_i + 3] == 0xFE) &&
          (in_data[PW_i + 4] == 0x4B) &&
          (in_data[PW_i + 5] == 0xFA) &&
          (in_data[PW_i + 6] == 0x02) &&
          (in_data[PW_i + 7] == 0x32) &&
          (in_data[PW_i + 8] == 0x4D) &&
          (in_data[PW_i + 9] == 0xFA) &&
          (in_data[PW_i + 10] == 0x02) &&
          (in_data[PW_i + 11] == 0x46) &&
          (in_data[PW_i + 12] == 0x20) &&
          (in_data[PW_i + 13] == 0x6D) &&
          (in_data[PW_i + 14] == 0x00) &&
          (in_data[PW_i + 15] == 0x0C))
      {
        if (testGNUPacker12() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_GNUPacker12();
        }
      }
#endif
      /* "HVL" - Hively tracker */
      if ((in_data[PW_i + 1] == 'V') &&
          (in_data[PW_i + 2] == 'L') &&
          ((in_data[PW_i + 3] == 0x00) || (in_data[PW_i + 3] == 0x01)))
      {
        if (testTHX() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_THX();
        }
      }
      break;

    case 'I': /* 0x48 */
#ifdef INCLUDEALL
      /* "IMPM" : ID of Impulse Tracker */
      if ((in_data[PW_i + 1] == 'M') &&
          (in_data[PW_i + 2] == 'P') &&
          (in_data[PW_i + 3] == 'M'))
      {
        if (testIT() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_IT();
        }
      }
#endif
      if ((in_data[PW_i + 1] == 'T') &&
          (in_data[PW_i + 2] == '1') &&
          (in_data[PW_i + 3] == '0'))
      {
        /* Ice Tracker 1.0 */
        if (testSTK26() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_STK26();
          if (Do_Depack == GOOD)
            Depack_STK26();
        }
      }
      break;

    case 'K': /* 0x4B */
      /* "KRIS" : ID of Chip Tracker */
      if ((in_data[PW_i + 1] == 'R') &&
          (in_data[PW_i + 2] == 'I') &&
          (in_data[PW_i + 3] == 'S'))
      {
        if (testKRIS() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_KRIS();
          if (Do_Depack == GOOD)
            Depack_KRIS();
        }
      }
      break;

    case 'M': /* 0x4D */
      if ((in_data[PW_i + 1] == '.') &&
          (in_data[PW_i + 2] == 'K') &&
          (in_data[PW_i + 3] == '.'))
      {
        /* protracker ? */
        if (testMOD(4) != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MOD(4);
        }

        /* Unic tracker v1 ? */
        if (testUNIC_withID() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_UNIC_withID();
          if (Do_Depack == GOOD)
            Depack_UNIC();
        }

        /* Noiserunner ? */
        if (testNoiserunner() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Noiserunner();
          if (Do_Depack == GOOD)
            Depack_Noiserunner();
        }

        /* Mosh packer ? */
        if (testMOSH() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MOSH();
          if (Do_Depack == GOOD)
            Depack_MOSH();
        }
        /* HCD-protector ? */
        if (testHCD() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_HCD();
          if (Do_Depack == GOOD)
            Depack_HCD();
        }
      }

      if ((in_data[PW_i + 1] == '1') &&
          (in_data[PW_i + 2] == '.') &&
          (in_data[PW_i + 3] == '0'))
      {
        /* Fuzzac packer */
        if (testFUZZAC() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Fuzzac();
          if (Do_Depack == GOOD)
            Depack_Fuzzac();
        }
      }

      if ((in_data[PW_i + 1] == 'E') &&
          (in_data[PW_i + 2] == 'X') &&
          (in_data[PW_i + 3] == 'X'))
      {
        /* tracker packer v2 */
        if (testTP2() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_TP2();
          if (Do_Depack == GOOD)
            Depack_TP2();
        }
        /* tracker packer v1 */
        if (testTP1() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_TP1();
          if (Do_Depack == GOOD)
            Depack_TP1();
        }
      }

      if (in_data[PW_i + 1] == '.')
      {
        /* Kefrens sound machine ? */
        if (testKSM() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_KSM();
          if (Do_Depack == GOOD)
            Depack_KSM();
        }
      }

      if ((in_data[PW_i + 1] == 'O') &&
          (in_data[PW_i + 2] == 'D') &&
          (in_data[PW_i + 3] == 'U'))
      {
        /* NovoTrade */
        if (testNovoTrade() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_NovoTrade();
          if (Do_Depack == GOOD)
            Depack_NovoTrade();
        }
      }

      if ((in_data[PW_i + 1] == 'T') &&
          (in_data[PW_i + 2] == 'N') &&
          (in_data[PW_i + 3] == 0x00))
      {
        /* SoundTracker 2.6 */
        if (testSTK26() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_STK26();
          if (Do_Depack == GOOD)
            Depack_STK26();
        }
      }

      if ((in_data[PW_i + 1] == 'M') &&
          (in_data[PW_i + 2] == 'D') &&
          ((in_data[PW_i + 3] == '0') ||
           (in_data[PW_i + 3] == '1'))) /* ||
             (in_data[PW_i+3] == '2') ||
             (in_data[PW_i+3] == '3')) )*/
      {
        /* MED (MMD0) */
        if (testMMD0() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MMD0();
        }
      }

#ifdef INCLUDEALL
      /* StoneCracker 2.99d */
      if ((in_data[PW_i + 1] == 0xF9) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xDF) &&
          (in_data[PW_i + 4] == 0xF0) &&
          (in_data[PW_i + 5] == 0x00) &&
          (in_data[PW_i + 6] == 0x4B) &&
          (in_data[PW_i + 7] == 0xfa) &&
          (in_data[PW_i + 8] == 0x01) &&
          (in_data[PW_i + 9] == 0x54) &&
          (in_data[PW_i + 10] == 0x49) &&
          (in_data[PW_i + 11] == 0xf9) &&
          (in_data[PW_i + 12] == 0x00) &&
          (in_data[PW_i + 13] == 0xbf) &&
          (in_data[PW_i + 14] == 0xd1) &&
          (in_data[PW_i + 15] == 0x00))
      {
        if (testSTC299d() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_STC299d();
        }
      }

      /* StoneCracker 2.99b */
      if ((in_data[PW_i + 1] == 0xF9) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xDF) &&
          (in_data[PW_i + 4] == 0xF0) &&
          (in_data[PW_i + 5] == 0x00) &&
          (in_data[PW_i + 6] == 0x4B) &&
          (in_data[PW_i + 7] == 0xfa) &&
          (in_data[PW_i + 8] == 0x01) &&
          (in_data[PW_i + 9] == 0x58) &&
          (in_data[PW_i + 10] == 0x49) &&
          (in_data[PW_i + 11] == 0xf9) &&
          (in_data[PW_i + 12] == 0x00) &&
          (in_data[PW_i + 13] == 0xbf) &&
          (in_data[PW_i + 14] == 0xd1) &&
          (in_data[PW_i + 15] == 0x00))
      {
        if (testSTC299b() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_STC299b();
        }
      }

      /* StoneCracker 2.99 */
      if ((in_data[PW_i + 1] == 0xF9) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xDF) &&
          (in_data[PW_i + 4] == 0xF0) &&
          (in_data[PW_i + 5] == 0x00) &&
          (in_data[PW_i + 6] == 0x4B) &&
          (in_data[PW_i + 7] == 0xfa) &&
          (in_data[PW_i + 8] == 0x01) &&
          (in_data[PW_i + 9] == 0x5c) &&
          (in_data[PW_i + 10] == 0x49) &&
          (in_data[PW_i + 11] == 0xf9) &&
          (in_data[PW_i + 12] == 0x00) &&
          (in_data[PW_i + 13] == 0xbf) &&
          (in_data[PW_i + 14] == 0xd1) &&
          (in_data[PW_i + 15] == 0x00))
      {
        if (testSTC299() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_STC299();
        }
      }

      /* StoneCracker 3.00 */
      if ((in_data[PW_i + 1] == 0xF9) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xDF) &&
          (in_data[PW_i + 4] == 0xF0) &&
          (in_data[PW_i + 5] == 0x96) &&
          (in_data[PW_i + 6] == 0x4b) &&
          (in_data[PW_i + 7] == 0xfa) &&
          (in_data[PW_i + 8] == 0x01) &&
          (in_data[PW_i + 9] == 0x5c) &&
          (in_data[PW_i + 10] == 0x49) &&
          (in_data[PW_i + 11] == 0xf9) &&
          (in_data[PW_i + 12] == 0x00) &&
          (in_data[PW_i + 13] == 0xbf) &&
          (in_data[PW_i + 14] == 0xd1) &&
          (in_data[PW_i + 15] == 0x00))
      {
        if (testSTC300() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_STC300();
        }
      }

      /* StoneCracker 3.10 */
      if ((in_data[PW_i + 1] == 0xF9) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0xDF) &&
          (in_data[PW_i + 4] == 0xF1) &&
          (in_data[PW_i + 5] == 0x80) &&
          (in_data[PW_i + 6] == 0x47) &&
          (in_data[PW_i + 7] == 0xf9) &&
          (in_data[PW_i + 8] == 0x00) &&
          (in_data[PW_i + 9] == 0xbf) &&
          (in_data[PW_i + 10] == 0xd1) &&
          (in_data[PW_i + 11] == 0x00) &&
          (in_data[PW_i + 12] == 0x41) &&
          (in_data[PW_i + 13] == 0xfa) &&
          (in_data[PW_i + 14] == 0x00) &&
          (in_data[PW_i + 15] == 0x62))
      {
        if (testSTC310() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_STC310();
        }
      }
#endif
      break;

    case 'N': /* Sonic Arranger (no hunk) */
      if ((in_data[PW_i + 1] == 0xFA) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 4] == 'N') &&
          (in_data[PW_i + 5] == 0xFA) &&
          (in_data[PW_i + 8] == 'N') &&
          (in_data[PW_i + 9] == 0xFA) &&
          (in_data[PW_i + 12] == 'N') &&
          (in_data[PW_i + 13] == 0xFA) &&
          (in_data[PW_i + 16] == 'N') &&
          (in_data[PW_i + 17] == 0xFA) &&
          (in_data[PW_i + 20] == 'N') &&
          (in_data[PW_i + 21] == 0xFA) &&
          (in_data[PW_i + 24] == 'N') &&
          (in_data[PW_i + 25] == 0xFA))
      {
        if (testSA() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_SA();
        }
      }
      break;

    case 'O': /* 0x4F */
      /* "OKTASONG" : ID of Oktalizer */
      if ((in_data[PW_i + 1] == 'K') &&
          (in_data[PW_i + 2] == 'T') &&
          (in_data[PW_i + 3] == 'A') &&
          (in_data[PW_i + 4] == 'S') &&
          (in_data[PW_i + 5] == 'O') &&
          (in_data[PW_i + 6] == 'N') &&
          (in_data[PW_i + 7] == 'G') &&
          (in_data[PW_i + 8] == 'C') &&
          (in_data[PW_i + 9] == 'M') &&
          (in_data[PW_i + 10] == 'O') &&
          (in_data[PW_i + 11] == 'D'))
      {
        if (testOkta() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Okta();
        }
      }
      break;

    case 'P': /* 0x50 */
#ifdef INCLUDEALL
      /* "PP20" : ID of PowerPacker */
      if ((in_data[PW_i + 1] == 'P') &&
          (in_data[PW_i + 2] == '2') &&
          (in_data[PW_i + 3] == '0'))
      {
        printf("PowerPacker ID (PP20) found at %u ... cant rip it!\n", PW_i);
      }
#endif
      /* "P30A" : ID of The Player */
      if ((in_data[PW_i + 1] == '3') &&
          (in_data[PW_i + 2] == '0') &&
          (in_data[PW_i + 3] == 'A'))
      {
        if (testP40A() != BAD) /* yep same tests apply */
        {
          if (Do_Rip == GOOD)
            Rip_P30A();
          if (Do_Depack == GOOD)
            Depack_P30();
        }
      }

      /* "P22A" : ID of The Player */
      if ((in_data[PW_i + 1] == '2') &&
          (in_data[PW_i + 2] == '2') &&
          (in_data[PW_i + 3] == 'A'))
      {
        if (testP40A() != BAD) /* yep, same tests apply */
        {
          if (Do_Rip == GOOD)
            Rip_P22A();
          if (Do_Depack == GOOD)
            Depack_P22();
        }
      }

      /* "P40A" : ID of The Player */
      if ((in_data[PW_i + 1] == '4') &&
          (in_data[PW_i + 2] == '0') &&
          (in_data[PW_i + 3] == 'A'))
      {
        if (testP40A() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_P40A();
          if (Do_Depack == GOOD)
            Depack_P40();
        }
      }

      /* "P40B" : ID of The Player */
      if ((in_data[PW_i + 1] == '4') &&
          (in_data[PW_i + 2] == '0') &&
          (in_data[PW_i + 3] == 'B'))
      {
        if (testP40A() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_P40B();
          if (Do_Depack == GOOD)
            Depack_P40();
        }
      }

      /* "P41A" : ID of The Player */
      if ((in_data[PW_i + 1] == '4') &&
          (in_data[PW_i + 2] == '1') &&
          (in_data[PW_i + 3] == 'A'))
      {
        if (testP41A() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_P41A();
          if (Do_Depack == GOOD)
            Depack_P41A();
        }
      }

      /* "PMd3/PMD3 : ID of ?!? TSCC format - 8CHN */
      if ((in_data[PW_i + 1] == 'M') &&
          ((in_data[PW_i + 2] == 'd') || (in_data[PW_i + 2] == 'D')) &&
          (in_data[PW_i + 3] == '3'))
      {
        if (testPMD3() != BAD) /* yep, same tests apply */
        {
          if (Do_Rip == GOOD)
            Rip_PMD3();
          if (Do_Depack == GOOD)
            Depack_PMD3();
        }
      }

      /* "PM40" : ID of Promizer 4 */
      if ((in_data[PW_i + 1] == 'M') &&
          (in_data[PW_i + 2] == '4') &&
          (in_data[PW_i + 3] == '0'))
      {
        if (testPM40() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_PM40();
          if (Do_Depack == GOOD)
            Depack_PM40();
        }
      }

#ifdef INCLUDEALL
      /* "PPbk" : ID of AMOS PowerPacker Bank */
      if ((in_data[PW_i + 1] == 'P') &&
          (in_data[PW_i + 2] == 'b') &&
          (in_data[PW_i + 3] == 'k'))
      {
        if (testPPbk() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_PPbk();
        }
      }
#endif
      /* POLKA Packer */
      if (((in_data[PW_i + 1] == 'W') &&
           (in_data[PW_i + 2] == 'R') &&
           (in_data[PW_i + 3] == '.')) ||
          ((in_data[PW_i + 1] == 'S') &&
           (in_data[PW_i + 2] == 'U') &&
           (in_data[PW_i + 3] == 'X')))
      {
        if (testPolka() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Polka();
          if (Do_Depack == GOOD)
            Depack_Polka();
        }
      }

      /* PERFSONG Packer */
      if ((in_data[PW_i + 1] == 'E') &&
          (in_data[PW_i + 2] == 'R') &&
          (in_data[PW_i + 3] == 'F') &&
          (in_data[PW_i + 4] == 'S') &&
          (in_data[PW_i + 5] == 'O') &&
          (in_data[PW_i + 6] == 'N') &&
          (in_data[PW_i + 7] == 'G'))
      {
        if (testPERFSONG() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_PERFSONG();
          if (Do_Depack == GOOD)
            Depack_PERFSONG();
        }
      }
      break;

    case 'S': /* 0x53 */
      /* "SNT!" ProRunner 2 */
      if ((in_data[PW_i + 1] == 'N') &&
          (in_data[PW_i + 2] == 'T') &&
          (in_data[PW_i + 3] == '!'))
      {
        if (testPRUN2() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_PRUN2();
          if (Do_Depack == GOOD)
            Depack_PRUN2();
        }
      }
      /* "SNT." ProRunner 1 */
      if ((in_data[PW_i + 1] == 'N') &&
          (in_data[PW_i + 2] == 'T') &&
          (in_data[PW_i + 3] == '.'))
      {
        if (testPRUN1() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_PRUN1();
          if (Do_Depack == GOOD)
            Depack_PRUN1();
        }
      }

      /* SKYT packer */
      if ((in_data[PW_i + 1] == 'K') &&
          (in_data[PW_i + 2] == 'Y') &&
          (in_data[PW_i + 3] == 'T'))
      {
        if (testSKYT() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_SKYT();
          if (Do_Depack == GOOD)
            Depack_SKYT();
        }
      }

      /* SMOD Future Composer 1.0 - 1.3 */
      if ((in_data[PW_i + 1] == 'M') &&
          (in_data[PW_i + 2] == 'O') &&
          (in_data[PW_i + 3] == 'D'))
      {
        if (testFC13() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_FC13();
        }
      }

      /* SIDMON 2 */
      /*        if ( (in_data[PW_i+1] == 'I') &&
                   (in_data[PW_i+2] == 'D') &&
                   (in_data[PW_i+3] == 'M') &&
                   (in_data[PW_i+4] == 'O') &&
                   (in_data[PW_i+5] == 'N') &&
                   (in_data[PW_i+6] == ' ') &&
                   (in_data[PW_i+7] == 'I') &&
                   (in_data[PW_i+8] == 'I') &&
                   (in_data[PW_i+9] == ' ') )
              {
                if ( testSIDMON2() != BAD )
                {
                  if(Do_Rip==GOOD)Rip_SIDMON2();
                }
              }*/

#ifdef INCLUDEALL
      /* "SCRM" : ID of ScreamTracker 3 S3M */
      if ((in_data[PW_i + 1] == 'C') &&
          (in_data[PW_i + 2] == 'R') &&
          (in_data[PW_i + 3] == 'M'))
      {
        if (testS3M() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_S3M();
        }
      }
#endif
      /* STIM Slamtilt */
      if ((in_data[PW_i + 1] == 'T') &&
          (in_data[PW_i + 2] == 'I') &&
          (in_data[PW_i + 3] == 'M'))
      {
        if (testSTIM() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_STIM();
          if (Do_Depack == GOOD)
            Depack_STIM();
        }
      }

      /* SA hunk */
      if ((in_data[PW_i + 1] == 'O') &&
          (in_data[PW_i + 2] == 'A') &&
          (in_data[PW_i + 3] == 'R') &&
          (in_data[PW_i + 4] == 'V') &&
          (in_data[PW_i + 5] == '1') &&
          (in_data[PW_i + 6] == '.') &&
          (in_data[PW_i + 7] == '0'))
      {
        if (testSAhunk() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_SA();
        }
      }

      /* SONG Fuchs Tracker */
      if ((in_data[PW_i + 1] == 'O') &&
          (in_data[PW_i + 2] == 'N') &&
          (in_data[PW_i + 3] == 'G'))
      {
        if (testFuchsTracker() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_FuchsTracker();
          if (Do_Depack == GOOD)
            Depack_FuchsTracker();
        }
        /* Sound FX */
        if (testSoundFX13() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_SoundFX13();
#ifndef UNIX
          if (Do_Depack == GOOD)
            Depack_SoundFX13();
#endif
        }
      }
      break;

    case 'T': /* "THX" - AHX */
      if ((in_data[PW_i + 1] == 'H') &&
          (in_data[PW_i + 2] == 'X') &&
          ((in_data[PW_i + 3] == 0x00) || (in_data[PW_i + 3] == 0x01)))
      {
        if (testTHX() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_THX();
        }
      }

      /* "TRK1" Module Protector */
      if ((in_data[PW_i + 1] == 'R') &&
          (in_data[PW_i + 2] == 'K') &&
          (in_data[PW_i + 3] == '1'))
      {
        /* Module Protector */
        if (testMP_withID() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MP_withID();
          if (Do_Depack == GOOD)
            Depack_MP();
        }
      }

      /* "TMK. Timetracker ?!? */
      if ((in_data[PW_i + 1] == 'M') &&
          (in_data[PW_i + 2] == 'K') &&
          (in_data[PW_i + 3] == 0x01))
      {
        if (testTMK() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_TMK();
          if (Do_Depack == GOOD)
            Depack_TMK();
        }
      }
      break;

    case 'U': /* "UNIC" */
      if ((in_data[PW_i + 1] == 'N') &&
          (in_data[PW_i + 2] == 'I') &&
          (in_data[PW_i + 3] == 'C'))
      {
        /* Unic tracker v1 ? */
        if (testUNIC_withID() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_UNIC_withID();
          if (Do_Depack == GOOD)
            Depack_UNIC();
        }
      }
      /* Mugician */
      if ((in_data[PW_i + 1] == 'G') &&
          (in_data[PW_i + 2] == 'I') &&
          (in_data[PW_i + 3] == 'C'))
      {
        /*  */
        if (testMUGICIAN() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_MUGICIAN();
        }
      }
      break;

    case 'V': /* "V.2" */
      if (((in_data[PW_i + 1] == '.') &&
           (in_data[PW_i + 2] == '2')) ||
          ((in_data[PW_i + 1] == '.') &&
           (in_data[PW_i + 2] == '3')))
      {
        /* Sound Monitor v2/v3 */
        if (testBP() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_BP();
        }
      }
      break;

    case 'W': /* 0x57 */
      /* "WN" Wanton Packer */
      if ((in_data[PW_i + 1] == 'N') &&
          (in_data[PW_i + 2] == 0x00))
      {
        if (testWN() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_WN();
          if (Do_Depack == GOOD)
            Depack_WN();
        }
      }
      break;

    case 0x60:
      /* promizer 1.8a ? */
      if ((in_data[PW_i + 1] == 0x38) &&
          (in_data[PW_i + 2] == 0x60) &&
          (in_data[PW_i + 3] == 0x00) &&
          (in_data[PW_i + 4] == 0x00) &&
          (in_data[PW_i + 5] == 0xa0) &&
          (in_data[PW_i + 6] == 0x60) &&
          (in_data[PW_i + 7] == 0x00) &&
          (in_data[PW_i + 8] == 0x01) &&
          (in_data[PW_i + 9] == 0x3e) &&
          (in_data[PW_i + 10] == 0x60) &&
          (in_data[PW_i + 11] == 0x00) &&
          (in_data[PW_i + 12] == 0x01) &&
          (in_data[PW_i + 13] == 0x0c) &&
          (in_data[PW_i + 14] == 0x48) &&
          (in_data[PW_i + 15] == 0xe7)) /* gosh !, should be enough :) */
      {
        if (testPMZ() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_PM18a();
          if (Do_Depack == GOOD)
            Depack_PM18a();
        }

        /* Promizer 1.0c */
        if (testPM10c() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_PM10c();
          if (Do_Depack == GOOD)
            Depack_PM10c();
        }
      }

      /* promizer 2.0 ? */
      if ((in_data[PW_i + 1] == 0x00) &&
          (in_data[PW_i + 2] == 0x00) &&
          (in_data[PW_i + 3] == 0x16) &&
          (in_data[PW_i + 4] == 0x60) &&
          (in_data[PW_i + 5] == 0x00) &&
          (in_data[PW_i + 6] == 0x01) &&
          (in_data[PW_i + 7] == 0x40) &&
          (in_data[PW_i + 8] == 0x60) &&
          (in_data[PW_i + 9] == 0x00) &&
          (in_data[PW_i + 10] == 0x00) &&
          (in_data[PW_i + 11] == 0xf0) &&
          (in_data[PW_i + 12] == 0x3f) &&
          (in_data[PW_i + 13] == 0x00) &&
          (in_data[PW_i + 14] == 0x10) &&
          (in_data[PW_i + 15] == 0x3a)) /* gosh !, should be enough :) */
      {
        if (testPM2() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_PM20();
          if (Do_Depack == GOOD)
            Depack_PM20();
        }
      }

#ifdef INCLUDEALL
      /* Spike Cruncher */
      if ((in_data[PW_i + 1] == 0x16) &&
          (in_data[PW_i + 24] == 0x48) &&
          (in_data[PW_i + 25] == 0xE7) &&
          (in_data[PW_i + 26] == 0xFF) &&
          (in_data[PW_i + 27] == 0xFE) &&
          (in_data[PW_i + 28] == 0x26) &&
          (in_data[PW_i + 29] == 0x7A) &&
          (in_data[PW_i + 30] == 0xFF) &&
          (in_data[PW_i + 31] == 0xDE) &&
          (in_data[PW_i + 32] == 0xD7) &&
          (in_data[PW_i + 33] == 0xCB) &&
          (in_data[PW_i + 34] == 0xD7) &&
          (in_data[PW_i + 35] == 0xCB) &&
          (in_data[PW_i + 36] == 0x58) &&
          (in_data[PW_i + 37] == 0x8B))
      {
        if (testSpikeCruncher() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_SpikeCruncher();
        }
      }
#endif
      break;

    case 0x61: /* "a" */
#ifdef INCLUDEALL
      /* Tetrapack 2.1 */
      if ((in_data[PW_i + 1] == 0x00) &&
          (in_data[PW_i + 2] == 0x41) &&
          (in_data[PW_i + 3] == 0xFA) &&
          (in_data[PW_i + 4] == 0x00) &&
          (in_data[PW_i + 5] == 0xE4) &&
          (in_data[PW_i + 6] == 0x4B) &&
          (in_data[PW_i + 7] == 0xF9) &&
          (in_data[PW_i + 8] == 0x00) &&
          (in_data[PW_i + 9] == 0xDF) &&
          (in_data[PW_i + 12] == 0xD1) &&
          (in_data[PW_i + 13] == 0xFC))
      {
        if (testTetrapack_2_1() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Tetrapack_2_1();
        }
      }

      /* Tetrapack 2.2 */
      if ((in_data[PW_i + 1] == 0x00) &&
          (in_data[PW_i + 2] == 0x43) &&
          (in_data[PW_i + 3] == 0xFA) &&
          (in_data[PW_i + 4] == 0x00) &&
          (in_data[PW_i + 5] == 0xFC) &&
          (in_data[PW_i + 12] == 0x28) &&
          (in_data[PW_i + 13] == 0x7A))
      {
        if (testTetrapack_2_2() != BAD)
          if (Do_Rip == GOOD)
            Rip_Tetrapack_2_2();
      }
#endif
      break;

    case 0x7E:
#ifdef INCLUDEALL
      /* Tetrapack 2.2 case #2 */
      if ((in_data[PW_i + 1] == 0x00) &&
          (in_data[PW_i + 2] == 0x43) &&
          (in_data[PW_i + 3] == 0xFA) &&
          (in_data[PW_i + 4] == 0x00) &&
          (in_data[PW_i + 5] == 0xFC) &&
          (in_data[PW_i + 12] == 0x28) &&
          (in_data[PW_i + 13] == 0x7A))
      {
        if (testTetrapack_2_2() != BAD)
          if (Do_Rip == GOOD)
            Rip_Tetrapack_2_2();
      }

      /* Tetrapack 2.1 case #2*/
      if ((in_data[PW_i + 1] == 0x00) &&
          (in_data[PW_i + 2] == 0x41) &&
          (in_data[PW_i + 3] == 0xFA) &&
          (in_data[PW_i + 4] == 0x00) &&
          (in_data[PW_i + 5] == 0xE4) &&
          (in_data[PW_i + 6] == 0x4B) &&
          (in_data[PW_i + 7] == 0xF9) &&
          (in_data[PW_i + 8] == 0x00))
      {
        if (testTetrapack_2_1() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_Tetrapack_2_1();
        }
      }
#endif
      break;

    case 0xAC:
      /* AC1D packer ?!? */
      if (in_data[PW_i + 1] == 0x1D)
      {
        if (testAC1D() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_AC1D();
          if (Do_Depack == GOOD)
            Depack_AC1D();
        }
      }
      break;

    case 0xC0:
      /* Pha Packer */
      if ((PW_i >= 1) && (in_data[PW_i - 1] == 0x03))
      {
        if (testPHA() != BAD)
        {
          if (Do_Rip == GOOD)
            Rip_PHA();
          if (Do_Depack == GOOD)
            Depack_PHA();
        }
      }
      break;

    default: /* do nothing ... save continuing :) */
      break;

    } /* end of switch */
}
