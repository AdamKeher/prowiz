/*
 * scan.c - the scanner: walks the input byte by byte and hands candidate
 *          positions to the format handlers.
 *
 * This is the main loop of the original prowiz.c, converted mechanically so
 * that the order of the checks is untouched. That order matters: a handler
 * that rips something moves the scan position (c->i), and every following
 * check of the same iteration - including the 'case' already selected by the
 * switch - runs at the new position, exactly as in the original.
 *
 * Copyright (C) 1997-2016 Sylvain "Asle" Chipaux (original Pro-Wizard for PC)
 * Copyright (C) 2026 the Pro-Wizard port contributors
 * Licensed under the GNU GPL version 2. See COPYING.
 */
#include "pw_internal.h"
#include "formats/formats.h"
#include "formats/new/new_formats.h"

/* byte at offset k from the scan position; like in_data[PW_i+k], the index
 * is computed in 32 bits and wraps */
#define B(k) pw_in8(c, (int64_t)(uint32_t)(c->i + (k)))

/* scan mode filters (all enabled by default, as in the original) */
#define WANT_MUSIC ((c->modes & PW_MODE_MUSIC) != 0)
#define WANT_DATA  ((c->modes & PW_MODE_DATA) != 0)
/* formats the original never had (src/formats/new/); off with PW_FORMATS_170A */
#define NEW_FORMATS (c->new_formats)

void pw_scan_run(pw_ctx *c)
{
  for ( c->i=0 ; c->i<(uint32_t)(c->in_size-PW_MIN_FILE_LENGTH) ; c->i+=1 )
  {
    if ( c->fatal )
      break;
    /* display where we are every 10 Kbytes */

    /*    {*/
      /* ... and rewrites on itself. */

      /* force printing on stdout (i.e. the screen) */
    /*      fflush ( stdout );*/
    /*    }*/

    /*******************************************************************/
    /* ok, now the real job starts here. So, we look for ID or Volume  */
    /* values. A simple switch case follows .. based on Hex values of, */
    /* as foretold, ID or volume (not all file have ID ... few in fact */
    /* do ..).                                                         */
    /*******************************************************************/

    if ( B(0) <= 0x40 )
    {
      /* try to get rid of those 00 hanging all the time everywhere :(
         */
      if ( B(0) == 0x00 )
      {
            for ( c->j = 0 ; c->j<PW_MIN_FILE_LENGTH ; c->j++)
            {
              if ( IN((int64_t)c->j + c->i) != 0x00 )
                break;
            }
            if ( c->j == PW_MIN_FILE_LENGTH )
            {
              c->i += (PW_MIN_FILE_LENGTH-2);
              continue;
            }
      }

      /* first, let's take care of the formats with 'ID' value <= 0x40 */
      /* "!PM!" : ID of Power Music */
      if ( (B(0)   == '!') &&
           (B(1) == 'P') &&
           (B(2) == 'M') &&
           (B(3) == '!') )
      {
        if ( WANT_MUSIC && pw_test_PM(c) != PW_BAD )
        {
          pw_rip_PM(c);
          pw_depack_PM(c);
        }
      }
      /* Treasure Patterns ?*/
      /* 0x4000 : ID of Treasure Patterns ? not sure */

      /* StoneCracker 2.92 data (ex-$08090A08 data cruncher) */
      if ( (B(0)   == 0x08) &&
           (B(1) == 0x09) &&
           (B(2) == 0x0A) &&
          ((B(3) == 0x08) ||
           (B(3) == 0x0A) ||
           (B(3) == 0x0B) ||
           (B(3) == 0x0E) ||
           (B(3) == 0x0D)))
      {
        if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
        {
          pw_rip_special_cruncher_data(c, "StoneCracker 2.92 Data Cruncher", 12, PWF_STC292data);
        }
      }
      /* "1AM" data cruncher */
      if ( (B(0)   == '1') &&
           (B(1) == 'A') &&
           (B(2) == 'M') )
      {
        if ( WANT_DATA && pw_test_special_cruncher_data(c, 12, 8) != PW_BAD )
        {
          pw_rip_special_cruncher_data(c, "Amnesty Design (1AM) Data Cruncher", 16, PWF_AmnestyDesign1);
        }
      }
      /* "2AM" data cruncher */
      if ( (B(0)   == '2') &&
           (B(1) == 'A') &&
           (B(2) == 'M') )
      {
        if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
        {
          pw_rip_special_cruncher_data(c, "Amnesty Design (2AM) Data Cruncher", 12, PWF_AmnestyDesign2);
        }
      }
      /* "[1-9]CHN" FastTracker v1 */
      if ( ((B(0)   == '1') ||
            (B(0)   == '2') ||
            (B(0)   == '3') ||
            (B(0)   == '4') ||
            (B(0)   == '5') ||
            (B(0)   == '6') ||
            (B(0)   == '7') ||
            (B(0)   == '8') ||
            (B(0)   == '9'))&&
           (B(1) == 'C') &&
           (B(2) == 'H') &&
           (B(3) == 'N') )
      {
        if ( WANT_MUSIC && pw_test_MOD(c, B(0)-0x30) != PW_BAD )
        {
          pw_rip_MOD(c, B(0)-0x30);
        }
      }
      /* "[10-32]CH" FastTracker v1/v2 */
      if ( ((((B(0)   == '1') || (B(0)   == '2')) &&
             ((B(1)   == '0') ||
              (B(1)   == '1') ||
              (B(1)   == '2') ||
              (B(1)   == '3') ||
              (B(1)   == '4') ||
              (B(1)   == '5') ||
              (B(1)   == '6') ||
              (B(1)   == '7') ||
              (B(1)   == '8') ||
              (B(1)   == '9'))) ||
            ((B(0)   == '3') &&
             ((B(1)   == '0') ||
              (B(1)   == '1')))) &&
            (B(2) == 'C') &&
            (B(3) == 'H') )
      {
        if ( WANT_MUSIC && pw_test_MOD(c, (B(0)-0x30)*10+B(1)-0x30) != PW_BAD )
        {
          pw_rip_MOD(c, (B(0)-0x30)*10+B(1)-0x30);
        }
      }
      /* =SB= data cruncher */
      if ( (B(0)   == 0x3D) &&
           (B(1) == 'S') &&
           (B(2) == 'B') &&
           (B(3) == 0x3D) )
      {
        if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
        {
          pw_rip_special_cruncher_data(c, "=SB= Data Cruncher", 12, PWF_SB_DataCruncher);
        }
      }

      /* B9AB data cruncher */
      if ( (B(0)   == 0x0B) &&
           (B(1) == 0x09) &&
           (B(2) == 0x0A) &&
           (B(3) == 0x0B) )
      {
        if ( WANT_DATA && pw_test_B9AB(c) != PW_BAD )
        {
          pw_rip_special_cruncher_data(c, "B9AB Data Cruncher", 274, PWF_B9AB);
        }
      }

      /* -CJ- data cruncher (CrackerJack/Mirage)*/
      if ( (B(0)   == 0x2D) &&
           (B(1) == 'C') &&
           (B(2) == 'J') &&
           (B(3) == 0x2D) )
      {
        if ( WANT_DATA && pw_test_special_cruncher_data(c, 4, 8) != PW_BAD )
        {
          pw_rip_special_cruncher_data(c, "-CJ- Data Cruncher", 0, PWF_CJ_DataCruncher);
        }
      }

      /* -GD- GnoiPacker (Skizzo)*/
      if ( (B(0)   == 0x2D) &&
           (B(1) == 'G') &&
           (B(2) == 'D') &&
           (B(3) == 0x2D) )
      {
        if ( WANT_MUSIC && pw_test_Skizzo(c) != PW_BAD )
        {
          pw_rip_Skizzo(c);
          pw_depack_Skizzo(c);
        }
      }

      /* Max Packer 1.2 */
      if ((B(0)   == 0x28) &&
         (B(1)  == 0x3C) &&
         (B(6)  == 0x26) &&
         (B(7)  == 0x7A) &&
         (B(8)  == 0x01) &&
         (B(9)  == 0x6C) &&
         (B(10) == 0x41) &&
         (B(11) == 0xFA) &&
         (B(12) == 0x01) &&
         (B(13) == 0x7C) &&
         (B(14) == 0xD1) &&
         (B(15) == 0xFA) )
      {
        if ( WANT_DATA && pw_test_MaxPacker12(c) != PW_BAD )
        {
          pw_rip_MaxPacker12(c);
        }
      }

      /* XANN packer */
      if ( B(0) == 0x3c )
      {
        if ( WANT_MUSIC && pw_test_XANN(c) != PW_BAD )
        {
          pw_rip_XANN(c);
          pw_depack_XANN(c);
        }
      }

      /* hum ... that's where things become interresting :) */
      /* Module Protector without ID */
      /* LEAVE IT THERE !!! ... at least before Heatseeker format since they are VERY similare ! */
      if ( WANT_MUSIC && pw_test_MP_noID(c) != PW_BAD )
      {
        pw_rip_MP_noID(c);
        pw_depack_MP(c);
      }

      /* Digital Illusion */
      if ( WANT_MUSIC && pw_test_DI(c) != PW_BAD )
      {
        pw_rip_DI(c);
        pw_depack_DI(c);
      }

      /* SGTPacker */

      /* eureka packer */
      if ( WANT_MUSIC && pw_test_EUREKA(c) != PW_BAD )
      {
        pw_rip_EUREKA(c);
        pw_depack_EUREKA(c);
      }

      /* The player 5.0a ? */
      if ( WANT_MUSIC && pw_test_P50A(c) != PW_BAD )
      {
        pw_rip_P50A(c);
        pw_depack_P50A(c);
      }

      /* The player 6.0a ? */
      if ( WANT_MUSIC && pw_test_P60A_nopack(c) != PW_BAD )
      {
        pw_rip_P60A(c);
        pw_depack_P60A(c);
      }

      /* The player 6.0a (packed samples)? */
      if ( WANT_MUSIC && pw_test_P60A_pack(c) != PW_BAD )
      {
        pw_printf(c,  "\b\b\b\b\b\b\b\bThe Player 6.0A with PACKED samples found at %d ... cant rip it!\n" , c->start );
        pw_report_unrippable(c, PWF_P60A, "The Player 6.0A with PACKED samples", (uint32_t)c->start);

        /*pw_depack_P60A(c);*/
      }

      /* The player 6.1a ? */
      if ( WANT_MUSIC && pw_test_P61A_nopack(c) != PW_BAD )
      {
        pw_rip_P61A(c);
        pw_depack_P61A(c);
      }

      /* The player 6.1a (packed samples)? */
      if ( WANT_MUSIC && pw_test_P61A_pack(c) != PW_BAD )
      {
        pw_printf(c,  "\b\b\b\b\b\b\b\bThe Player 6.1A with PACKED samples found at %d ... cant rip it!\n" , c->start );
        pw_report_unrippable(c, PWF_P61A, "The Player 6.1A with PACKED samples", (uint32_t)c->start);

        /*pw_depack_P61A(c);*/
      }

      /* Propacker 1.0 */
      if ( WANT_MUSIC && pw_test_PP10(c) != PW_BAD )
      {
        pw_rip_PP10(c);
        pw_depack_PP10(c);
      }

      /* Noise Packer v2 */
      /* LEAVE VERSION 2 BEFORE VERSION 1 !!!!! */
      if ( WANT_MUSIC && pw_test_Noisepacker2(c) != PW_BAD )
      {
        pw_rip_Noisepacker2(c);
        pw_depack_Noisepacker2(c);
      }

      /* Noise Packer v1 */
      if ( WANT_MUSIC && pw_test_Noisepacker1(c) != PW_BAD )
      {
        pw_rip_Noisepacker1(c);
        pw_depack_Noisepacker1(c);
      }

      /* Noise Packer v3 */
      if ( WANT_MUSIC && pw_test_Noisepacker3(c) != PW_BAD )
      {
        pw_rip_Noisepacker3(c);
        pw_depack_Noisepacker3(c);
      }

      /* Promizer 0.1 */
      if ( WANT_MUSIC && pw_test_PM01(c) != PW_BAD )
      {
        pw_rip_PM01(c);
        pw_depack_PM01(c);
      }

      /* ProPacker 2.1 */
      if ( WANT_MUSIC && pw_test_PP21(c) != PW_BAD )
      {
        pw_rip_PP21(c);
        pw_depack_PP21(c);
      }

      /* ProPacker 3.0 */
      if ( WANT_MUSIC && pw_test_PP30(c) != PW_BAD )
      {
        pw_rip_PP30(c);
        pw_depack_PP30(c);
      }

      /* StartTrekker pack */
      if ( WANT_MUSIC && pw_test_STARPACK(c) != PW_BAD )
      {
        pw_rip_STARPACK(c);
        pw_depack_STARPACK(c);
      }

      /* Zen packer */
      if ( WANT_MUSIC && pw_test_ZEN(c) != PW_BAD )
      {
        pw_rip_ZEN(c);
        pw_depack_ZEN(c);
      }

      /* Unic tracker v1 ? */
      if ( WANT_MUSIC && pw_test_UNIC_withemptyID(c) != PW_BAD )
      {
        pw_rip_UNIC_withID(c);
        pw_depack_UNIC(c);
      }

      /* Unic tracker v1 ? */
      if ( WANT_MUSIC && pw_test_UNIC_noID(c) != PW_BAD )
      {
        pw_rip_UNIC_noID(c);
        pw_depack_UNIC(c);
      }

      /* Unic trecker v2 ? */
      if ( WANT_MUSIC && pw_test_UNIC2(c) != PW_BAD )
      {
        pw_rip_UNIC2(c);
        pw_depack_UNIC2(c);
      }

      /* Game Music Creator ? */
      if ( WANT_MUSIC && pw_test_GMC(c) != PW_BAD )
      {
        pw_rip_GMC(c);
        pw_depack_GMC(c);
      }

      /* Heatseeker ? */
      if ( WANT_MUSIC && pw_test_HEATSEEKER(c) != PW_BAD )
      {
        pw_rip_HEATSEEKER(c);
        pw_depack_HEATSEEKER(c);
      }

      /* SoundTracker (15 smp) */
      if ( WANT_MUSIC && pw_test_SoundTracker(c) != PW_BAD )
      {
        pw_rip_SoundTracker(c);
      }

      /* The Dark Demon (group name) format */
      if ( WANT_MUSIC && pw_test_TheDarkDemon(c) != PW_BAD )
      {
        pw_rip_TheDarkDemon(c);
        pw_depack_TheDarkDemon(c);
      }

      /* Newtron */
      if ( WANT_MUSIC && pw_test_Newtron(c) != PW_BAD )
      {
        pw_rip_Newtron(c);
        pw_depack_Newtron(c);
      }

      /* Newtron Old */
      if ( WANT_MUSIC && pw_test_NewtronOld(c) != PW_BAD )
      {
        pw_rip_NewtronOld(c);
        pw_depack_NewtronOld(c);
      }

      /* Titanics Player ? */
      if ( WANT_MUSIC && pw_test_TitanicsPlayer(c) != PW_BAD )
      {
        pw_rip_TitanicsPlayer(c);
        pw_depack_TitanicsPlayer(c);
      }

      /* Struggle game ? */

    }

    /**********************************/
    /* ok, now, the files with ID ... */
    /**********************************/
    switch ( B(0) )
    {
      case 'A': /* ATN! another Imploder case */
        if ( (B(1) == 'T') &&
             (B(2) == 'N') &&
             (B(3) == '!') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Imploder data", 50, PWF_IMP);
          }
        }
        /* AMOS Music bank "AmBk" */
        if ( (B(1) == 'm') &&
             (B(2) == 'B') &&
             (B(3) == 'k') )
        {
          if ( WANT_MUSIC && pw_test_AmBk(c) != PW_BAD )
          {
            pw_rip_AmBk(c);
            pw_depack_AmBk(c);
          }
        }
        /* Sidmon v1 */
        if ( B(1) == 0xFA )
        {
          if ( WANT_MUSIC && pw_test_SIDMON1(c) != PW_BAD )
          {
            pw_rip_SIDMON1(c);
          }
        }
        /* Time Cruncher 1.7 */
        if ( (B(1)  == 0xFA) &&
             (B(2)  == 0x01) &&
             (B(3)  == 0x34) &&
             (B(4)  == 0xD1) &&
             (B(5)  == 0xFC) &&
             (B(10) == 0x43) &&
             (B(11) == 0xF9) &&
             (B(16) == 0x24) &&
             (B(17) == 0x60) &&
             (B(18) == 0xD5) &&
             (B(19) == 0xC9) &&
             (B(20) == 0x20) &&
             (B(21) == 0x20) )
        {
          if ( WANT_DATA && pw_test_TimeCruncher17(c) != PW_BAD )
          {
            pw_rip_TimeCruncher17(c);
          }
        }
        /* IAM Cruncher 1.0 (another case (aka ICE)) */
        if ( (B(1) == 'T') &&
             (B(2) == 'M') &&
             (B(3) == '5') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "IAM Packer 1.0 (ATM5) data", 12, PWF_ICE);
          }
        }
        /* ATOM - Atomik Packer (Atari ST) */
        if ( (B(1) == 'T') &&
             (B(2) == 'O') &&
             (B(3) == 'M') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Atomik Packer (ATOM) data", 12, PWF_AtomikPackerData);
          }
        }
        /* ATM3 - Atomik Packer (Atari ST) */
        if ( (B(1) == 'T') &&
             (B(2) == 'M') &&
             (B(3) == '3') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Atomik Packer (ATM3) data", 12, PWF_AtomikPackerData);
          }
        }
          /* "AU5!" - Automation Packer 5.* (Atari ST) */
        if ( (B(1) == 'U') &&
             (B(2) == '5') &&
             (B(3) == '!') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Automation Packer v5.01 (data)", 0, PWF_AutomationPackerData);
          }
        }
        /* Syncro Packer 4.6 */
        if ( (B(1)  == 0xFA ) &&
             (B(2)  == 0x01 ) &&
             (B(3)  == 0x66 ) &&
             (B(4)  == 0x22 ) &&
             (B(5)  == 0x58 ) &&
             (B(6)  == 0x20 ) &&
             (B(7)  == 0x18 ) &&
             (B(8)  == 0x26 ) &&
             (B(9)  == 0x48 ) &&
             (B(10) == 0xD1 ) &&
             (B(11) == 0xC0 ) &&
             (B(12) == 0x1E ) &&
             (B(13) == 0x20 ) &&
             (B(14) == 0x1C ) &&
             (B(15) == 0x20 ) )
        {
          if ( WANT_DATA && pw_test_SyncroPacker(c) != PW_BAD )
          {
            pw_rip_SyncroPacker(c);
          }
        }
        /* Tetrapack 1.02 */
        if ( (B(1)  == 0xFA ) &&
             (B(2)  == 0x00 ) &&
             (B(3)  == 0xE6 ) &&
             (B(4)  == 0xD1 ) &&
             (B(5)  == 0xFC ) &&
             (B(10) == 0x22 ) &&
             (B(11) == 0x7C ) &&
             (B(16) == 0x24 ) &&
             (B(17) == 0x60 ) &&
             (B(18) == 0xD5 ) &&
             (B(19) == 0xC9 ) )
        {
          if ( WANT_DATA && pw_test_Tetrapack102(c) != PW_BAD )
          {
            pw_rip_Tetrapack102(c);
          }
        }
        /* Tetrapack 1.01 */
        if ( (B(1)  == 0xFA ) &&
             (B(2)  == 0x00 ) &&
             (B(3)  == 0xDE ) &&
             (B(4)  == 0xD1 ) &&
             (B(5)  == 0xFC ) &&
             (B(10) == 0x22 ) &&
             (B(11) == 0x7C ) &&
             (B(16) == 0x24 ) &&
             (B(17) == 0x60 ) &&
             (B(18) == 0xD5 ) &&
             (B(19) == 0xC9 ) )
        {
          if ( WANT_DATA && pw_test_Tetrapack101(c) != PW_BAD )
          {
            pw_rip_Tetrapack101(c);
          }
        }
        /* "ArcD" data cruncher */
        if ( (B(1)  == 'r') &&
             (B(2)  == 'c') &&
             (B(3)  == 'D'))
        {
          if ( WANT_DATA && pw_test_ArcDDataCruncher(c) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "ArcD data Cruncher", 0, PWF_arcD);
          }
        }
        /* HQC Cruncher 2.0 */
        if ( (B(1)  == 0xFA ) &&
             (B(2)  == 0x06 ) &&
             (B(3)  == 0x76 ) &&
             (B(4)  == 0x20 ) &&
             (B(5)  == 0x80 ) &&
             (B(6)  == 0x41 ) &&
             (B(7)  == 0xFA ) &&
             (B(8)  == 0x06 ) &&
             (B(9)  == 0x64 ) &&
             (B(10) == 0x43 ) &&
             (B(11) == 0xFA ) &&
             (B(12) == 0x05 ) &&
             (B(13) == 0x10 ) &&
             (B(14) == 0x20 ) &&
             (B(15) == 0x89 ) )
        {
          if ( WANT_DATA && pw_test_HQCCruncher2(c) != PW_BAD )
          {
            pw_rip_HQCCruncher2(c);
          }
        }
        /* ByteKillerPro 1.0 */
        if ( (B(1)  == 0xFA) &&
             (B(2)  == 0x00) &&
             (B(3)  == 0xDC) &&
             (B(4)  == 0x2C) &&
             (B(5)  == 0x78) &&
             (B(6)  == 0x00) &&
             (B(7)  == 0x04) &&
             (B(12) == 0x43) &&
             (B(13) == 0xF9) &&
             (B(18) == 0x20) &&
             (B(19) == 0x10) &&
             (B(20) == 0x22) &&
             (B(21) == 0x28) )
        {
          if ( WANT_DATA && pw_test_bytekillerpro10(c) != PW_BAD )
          {
            pw_rip_bytekillerpro10(c);
          }
        }
        /* Bytekiller Clone FLT */
        if ( (B(1) == 0xFA) &&
             (B(2) == 0x01) &&
             (B(3) == 0x32) &&
             (B(4) == 0xD1) &&
             (B(5) == 0xFA) &&
             (B(6) == 0x01) &&
             (B(7) == 0x2A) &&
             (B(8) == 0x22) &&
             (B(9) == 0x7c) )
        {
          if ( WANT_DATA && pw_test_BKCloneFLT(c) != PW_BAD )
          {
            pw_rip_BKCloneFLT(c);
          }
        }
        /* unknown Bytekiller Clone 5 */
        if ( (B(1) == 0xFA) &&
             (B(2) == 0x01) &&
             (B(3) == 0x16) &&
             (B(4) == 0xD1) &&
             (B(5) == 0xFC) &&
             (B(10) == 0x43) &&
             (B(11) == 0xF9) &&
             (B(16) == 0x24) &&
             (B(17) == 0x60) )
        {
          if ( WANT_DATA && pw_test_BKClone5(c) != PW_BAD )
          {
            pw_rip_BKClone5(c);
          }
        }
        /* Ace? (data cruncher) */
        if ( (B(1) == 'c') &&
             (B(2) == 'e') &&
             (B(3) == '?') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 4, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "(Ace?) Data Cruncher", 0, PWF_ACECruncherData);
          }
        }
        /* Delta Music 1 */
        if ( (B(1) == 'L') &&
             (B(2) == 'L') &&
             (B(3) == ' ') )
        {
          if ( WANT_MUSIC && pw_test_DM1(c) != PW_BAD )
          {
            pw_rip_DM1(c);
          }
        }

        break;

      case 'B': /* BTB6 */
        /* ByteKiller 1.3 (exepack) */
        if ( (B(1) == 'T') &&
             (B(2) == 'B') &&
             (B(3) == '6') )
        {
          if ( WANT_DATA && pw_test_ByteKiller_13(c) != PW_BAD )
          {
            pw_rip_ByteKiller(c);
          }
          if ( WANT_DATA && pw_test_ByteKiller_20(c) != PW_BAD )
          {
            pw_rip_ByteKiller(c);
          }
        }

        /* BHC2 (data cruncher) */
        if ( (B(1) == 'H') &&
             (B(2) == 'C') &&
             (B(3) == '2') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 6, 10) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "(BHC2) Data Cruncher", 0x3C, PWF_BHC2CruncherData);
          }
        }
        /* BHC3 (data cruncher) */
        if ( (B(1) == 'H') &&
             (B(2) == 'C') &&
             (B(3) == '3') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "(BHC3) Data Cruncher", 0xC8, PWF_BHC3CruncherData);
          }
        }
        /* "BeEp" Jam Cracker */
        if ( (B(1) == 'e') &&
             (B(2) == 'E') &&
             (B(3) == 'p') )
        {
          if ( WANT_MUSIC && pw_test_JamCracker(c) != PW_BAD )
          {
            pw_rip_JamCracker(c);
          }
        }

        /* "BNR!" Binary Packer */
        if ( (B(1) == 'N') &&
             (B(2) == 'R') &&
             (B(3) == '!') )
        {
          if ( WANT_MUSIC && pw_test_BNR(c) != PW_BAD )
          {
            pw_rip_BNR(c);
            pw_depack_BNR(c);
          }
        }
        break;

      case 'C': /* 0x43 */
      /* CPLX_TP3 ?!? */
        if ( (B(1) == 'P') &&
             (B(2) == 'L') &&
             (B(3) == 'X') &&
             (B(4) == '_') &&
             (B(5) == 'T') &&
             (B(6) == 'P') &&
             (B(7) == '3') )
        {
          if ( WANT_MUSIC && pw_test_TP3(c) != PW_BAD )
          {
            pw_rip_TP3(c);
            pw_depack_TP3(c);
          }
        }
        /* CrM2 | Crm2 | CrM! */
        if ( ((B(1) == 'r') &&
              (B(2) == 'M') &&
              (B(3) == '2'))||
             ((B(1) == 'r') &&
              (B(2) == 'm') &&
              (B(3) == '2'))||
             ((B(1) == 'r') &&
              (B(2) == 'M') &&
              (B(3) == '!')) )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 10, 6) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Crunchmania / Normal data", 14, PWF_CRM1);
          }
        }
        /* "CHFI"  another imploder case */
        if ( (B(1) == 'H') &&
             (B(2) == 'F') &&
             (B(3) == 'I') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Imploder data", 50, PWF_IMP);
          }
        }
        /* "CRND" data cruncher */
        if ( (B(1) == 'R') &&
             (B(2) == 'N') &&
             (B(3) == 'D') )
        {
          if ( WANT_DATA && pw_test_CRND(c) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "CRND data cruncher", 20, PWF_CRND);
          }
        }
        /* Defjam Cruncher 3.2 */
        if ( (B(1) == 0xFA) &&
             (B(2) == 0x02) &&
             (B(3) == 0x8C) &&
             ((B(4) == 0x4B) || (B(4) == 0x9B))&&
             ((B(5) == 0xF9) || (B(5) == 0xCD))&&
             ((B(6) == 0x00) || (B(6) == 0x4E)))
        {
          if ( WANT_DATA && pw_test_Defjam32(c) != PW_BAD )
          {
            pw_rip_Defjam32(c);
          }
        }
        break;

      case 'D': /* 0x44 */
        /* Digibooster 1.7 */
        if ( (B(1) == 'I') &&
             (B(2) == 'G') &&
             (B(3) == 'I') )
        {
          if ( WANT_MUSIC && pw_test_DigiBooster17(c) != PW_BAD )
          {
            pw_rip_DigiBooster17(c);
          }
        }
        break;

      case 'E': /* 0x45 */
          /* not in the original: "EXO4" / "EXO8" : StarTrekker (Exolon) */
        if ( NEW_FORMATS &&
             (B(1) == 'X') &&
             (B(2) == 'O') &&
             (B(3) == '4') )
        {
          if ( WANT_MUSIC && pw_test_MOD(c, 4) != PW_BAD )
          {
            pw_rip_MOD(c, 4);
            pw_new_StartrekkerAM_companion(c);
          }
        }
        if ( NEW_FORMATS &&
             (B(1) == 'X') &&
             (B(2) == 'O') &&
             (B(3) == '8') )
        {
          if ( WANT_MUSIC )
            pw_new_Startrekker8(c);
        }
          /* "EMOD" : ID of Quadra Composer */
        if ( (B(1) == 'M') &&
             (B(2) == 'O') &&
             (B(3) == 'D') )
        {
          if ( WANT_MUSIC && pw_test_QuadraComposer(c) != PW_BAD )
          {
            pw_rip_QuadraComposer(c);
            /*pw_depack_QuadraComposer(c);*/
          }
        }
          /* "Extended Module" : ID of FastTracker 2 XM */
        if ( (B(1) == 'x') &&
             (B(2) == 't') &&
             (B(3) == 'e') &&
             (B(4) == 'n') &&
             (B(5) == 'd') &&
             (B(6) == 'e') &&
             (B(7) == 'd') &&
             (B(8) == ' ') &&
             (B(9) == 'M') &&
             (B(10)== 'o') )
        {
          if ( WANT_MUSIC && pw_test_XM(c) != PW_BAD )
          {
            pw_rip_XM(c);
          }
        }
        break;

      case 'F': /* 0x46 */
          /* "FC-M" : ID of FC-M packer */
        if ( (B(1) == 'C') &&
             (B(2) == '-') &&
             (B(3) == 'M') )
        {
          if ( WANT_MUSIC && pw_test_FC_M(c) != PW_BAD )
          {
            pw_rip_FC_M(c);
            pw_depack_FC_M(c);
          }
        }
          /* "FLT4" : ID of StarTrekker */
        if ( (B(1) == 'L') &&
             (B(2) == 'T') &&
             (B(3) == '4') )
        {
          if ( WANT_MUSIC && pw_test_MOD(c, 4) != PW_BAD )
          {
            pw_rip_MOD(c, 4);
            /* not in the original: its Startrekker AM synth file (.nt) */
            if ( NEW_FORMATS )
              pw_new_StartrekkerAM_companion(c);
          }
        }
          /* not in the original: "FLT8" : StarTrekker 8 channels */
        if ( NEW_FORMATS &&
             (B(1) == 'L') &&
             (B(2) == 'T') &&
             (B(3) == '8') )
        {
          if ( WANT_MUSIC )
            pw_new_Startrekker8(c);
        }
          /* "FC14" : Future Composer 1.4 */
        if ( (B(1) == 'C') &&
             (B(2) == '1') &&
             (B(3) == '4') )
        {
          if ( WANT_MUSIC && pw_test_FC14(c) != PW_BAD )
          {
            pw_rip_FC14(c);
          }
        }
          /* "FORM" : EA-IFF */
        if ( (B(1) == 'O') &&
             (B(2) == 'R') &&
             (B(3) == 'M') )
        {
          if ( WANT_MUSIC && pw_test_IFF(c) != PW_BAD )
          {
            pw_rip_IFF(c);
          }
        }
          /* "FUCO" : ID of BSI Future Composer */
        if ( (B(1) == 'U') &&
             (B(2) == 'C') &&
             (B(3) == 'O') )
        {
          if ( WANT_MUSIC && pw_test_BSIFutureComposer(c) != PW_BAD )
          {
            pw_rip_BSIFutureComposer(c);
          }
        }
         /* "Fuck" : ID of Noise From Heaven chiptunes */
        if ( (B(1) == 'u') &&
             (B(2) == 'c') &&
             (B(3) == 'k') )
        {
          if ( WANT_MUSIC && pw_test_NFH(c) != PW_BAD )
          {
            pw_rip_NFH(c);
            pw_depack_NFH(c);
          }
        }
              /* "FAST" : ID of Stone Arts Player */
        if ( (B(1) == 'A') &&
             (B(2) == 'S') &&
             (B(3) == 'T') )
        {
          if ( WANT_MUSIC && pw_test_StoneArtsPlayer(c) != PW_BAD )
          {
            pw_rip_StoneArtsPlayer(c);
                  pw_depack_StoneArtsPlayer(c);
          }
        }
          /* FIRE (RNC clone) Cruncher */
        if ( (B(1) == 'I') &&
             (B(2) == 'R') &&
             (B(3) == 'E') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 4, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "FIRE (RNC Clone) data Cruncher", 0, PWF_FIRE);
          }
        }
        break;

      case 'G': /* 0x47 */
        /* Mega Cruncher 1.0 */
        if ( (B(1)  == 0xFA ) &&
             (B(2)  == 0x01 ) &&
             (B(3)  == 0x2E ) &&
             (B(4)  == 0x20 ) &&
             (B(5)  == 0x0B ) &&
             (B(6)  == 0x22 ) &&
             (B(7)  == 0x2B ) &&
             (B(8)  == 0x00 ) &&
             (B(9)  == 0x08 ) &&
             (B(10) == 0x41 ) &&
             (B(11) == 0xFA ) &&
             (B(12) == 0x01 ) &&
             (B(13) == 0x30 ) &&
             (B(14) == 0xD1 ) &&
             (B(15) == 0xC1 ) )
        {
          if ( WANT_DATA && pw_test_MegaCruncher10(c) != PW_BAD )
          {
            pw_rip_MegaCruncher(c);
          }
        }

        /* Mega Cruncher 1.2 */
        if ( (B(1)  == 0xFA ) &&
             (B(2)  == 0x01 ) &&
             (B(3)  == 0x32 ) &&
             (B(4)  == 0x20 ) &&
             (B(5)  == 0x0B ) &&
             (B(6)  == 0x22 ) &&
             (B(7)  == 0x2B ) &&
             (B(8)  == 0x00 ) &&
             (B(9)  == 0x08 ) &&
             (B(10) == 0x41 ) &&
             (B(11) == 0xFA ) &&
             (B(12) == 0x01 ) &&
             (B(13) == 0x34 ) &&
             (B(14) == 0xD1 ) &&
             (B(15) == 0xC1 ) )
        {
          if ( WANT_DATA && pw_test_MegaCruncher12(c) != PW_BAD )
              {
            pw_rip_MegaCruncher(c);
          }
        }

        /* Double Action v1.0 */
        if ( (B(1)  == 0xF9 ) &&
             (B(2)  == 0x00 ) &&
             (B(3)  == 0xDF ) &&
             (B(137)== 0xAB ) &&
             (B(138)== 0xD1 ) &&
             (B(139)== 0xC0 ) &&
             (B(140)== 0xD3 ) &&
             (B(141)== 0xC0 ) &&
             (B(142)== 0x23 ) &&
             (B(143)== 0x20 ) )
        {
          if ( WANT_DATA && pw_test_DoubleAction10(c) != PW_BAD )
          {
            pw_rip_DoubleAction10(c);
          }
        }
        /* GPMO (crunch player ?)*/
        if ( (B(1) == 'P') &&
           (B(2) == 'M') &&
           (B(3) == 'O') )
        {
          if ( WANT_MUSIC && pw_test_GPMO(c) != PW_BAD )
          {
            pw_rip_GPMO(c);
            pw_depack_GPMO(c);
          }
        }

        /* Gnu player */
        if ( (B(1) == 'n') &&
           (B(2) == 'P') &&
           (B(3) == 'l') )
        {
          if ( WANT_MUSIC && pw_test_GnuPlayer(c) != PW_BAD )
          {
            pw_rip_GnuPlayer(c);
            pw_depack_GnuPlayer(c);
          }
        }

        break;

      case 'H': /* 0x48 */
          /* "HRT!" : ID of Hornet packer */
        if ( (B(1) == 'R') &&
           (B(2) == 'T') &&
           (B(3) == '!') )
        {
          if ( WANT_MUSIC && pw_test_HRT(c) != PW_BAD )
          {
            pw_rip_HRT(c);
            pw_depack_HRT(c);
          }
        }

        /* Master Cruncher 3.0 Address */
        if ( (B(1)  == 0xE7) &&
             (B(2)  == 0xFF) &&
             (B(3)  == 0xFE) &&
             (B(4)  == 0x4B) &&
             (B(5)  == 0xFA) &&
             (B(6)  == 0x01) &&
             (B(7)  == 0x80) &&
             (B(8)  == 0x41) &&
             (B(9)  == 0xFA) &&
             (B(10) == 0xFF) &&
             (B(11) == 0xF2) &&
             (B(12) == 0x22) &&
             (B(13) == 0x50) &&
             (B(14) == 0xD3) &&
             (B(15) == 0xC9) )
        {
          if ( WANT_DATA && pw_test_MasterCruncher30addr(c) != PW_BAD )
          {
            pw_rip_MasterCruncher30addr(c);
          }
        }

        /* Powerpacker 4.0 library */
        if ( (B(1)  == 0x7A) &&
             (B(2)  == 0x00) &&
             (B(3)  == 0x58) &&
             (B(4)  == 0x48) &&
             (B(5)  == 0xE7) &&
             (B(6)  == 0xFF) &&
             (B(7)  == 0xFE) &&
             (B(8)  == 0x70) &&
             (B(9)  == 0x23) &&
             (B(10) == 0x43) &&
             (B(11) == 0xFA) &&
             (B(12) == 0x00) &&
             (B(13) == 0x50) &&
             (B(14) == 0x2C) &&
             (B(15) == 0x78) )
        {
          if ( WANT_DATA && pw_test_Powerpacker4lib(c) != PW_BAD )
          {
            pw_rip_Powerpacker4lib(c);
          }
        }
        /* StoneCracker 2.70 */
        if ( (B(1)  == 0xE7) &&
             (B(2)  == 0xFF) &&
             (B(3)  == 0xFE) &&
             (B(4)  == 0x4D) &&
             (B(5)  == 0xF9) &&
             (B(6)  == 0x00) &&
             (B(7)  == 0xDF) &&
             (B(8)  == 0xF0) &&
             (B(9)  == 0x06) &&
             (B(10) == 0x7E) &&
             (B(11) == 0x00) &&
             (B(12) == 0x7C) &&
             (B(13) == 0x00) &&
             (B(14) == 0x7A) &&
             (B(15) == 0x00) )
        {
          if ( WANT_DATA && pw_test_StoneCracker270(c) != PW_BAD )
          {
            pw_rip_StoneCracker270(c);
          }
        }

        /* ByteKiller 3.0 */
        if ( (B(1)  == 0xE7) &&
             (B(2)  == 0xFF) &&
             (B(3)  == 0xFE) &&
             (B(4)  == 0x41) &&
             (B(5)  == 0xFA) &&
             (B(6)  == 0x00) &&
             (B(7)  == 0xB6) &&
             (B(8)  == 0x43) &&
             (B(9)  == 0xF9) &&
             (B(14) == 0x4D) &&
             (B(15) == 0xF9) )
        {
          if ( WANT_DATA && pw_test_ByteKiller30(c) != PW_BAD )
          {
            pw_rip_ByteKiller30(c);
          }
        }

        /* Powerpacker 2.3 */
        if ( (B(1)  == 0xE7) &&
             (B(2)  == 0xFF) &&
             (B(3)  == 0xFE) &&
             (B(4)  == 0x41) &&
             (B(5)  == 0xFA) &&
             (B(6)  == 0xFF) &&
             (B(7)  == 0xF6) &&
             (B(8)  == 0x20) &&
             (B(9)  == 0x50) &&
             (B(10) == 0xD1) &&
             (B(11) == 0xC8) &&
             (B(12) == 0xD1) &&
             (B(13) == 0xC8) &&
             (B(14) == 0x4A) &&
             (B(15) == 0x98) )
        {
          if ( WANT_DATA && pw_test_Powerpacker23(c) != PW_BAD )
          {
            pw_rip_Powerpacker23(c);
          }
        }

        /* Powerpacker 3.0 */
        if ( (B(1)  == 0x7A) &&
             (B(2)  == 0x01) &&
             (B(3)  == 0x78) &&
             (B(4)  == 0x48) &&
             (B(5)  == 0xE7) &&
             (B(6)  == 0xFF) &&
             (B(7)  == 0xFE) &&
             (B(8)  == 0x49) &&
             (B(9)  == 0xFA) &&
             (B(10) == 0xFF) &&
             (B(11) == 0xF2) &&
             (B(12) == 0x20) &&
             (B(13) == 0x54) &&
             (B(14) == 0xD1) &&
             (B(15) == 0xC8) )
        {
          if ( WANT_DATA && pw_test_Powerpacker30(c) != PW_BAD )
          {
            pw_rip_Powerpacker30(c);
          }
        }

        /* Powerpacker 4.0 */
        if ( (B(1)  == 0x7A) &&
             (B(2)  == 0x01) &&
             (B(3)  == 0xC8) &&
             (B(4)  == 0x48) &&
             (B(5)  == 0xE7) &&
             (B(6)  == 0xFF) &&
             (B(7)  == 0xFE) &&
             (B(8)  == 0x49) &&
             (B(9)  == 0xFA) &&
             (B(10) == 0xFF) &&
             (B(11) == 0xF2) &&
             (B(12) == 0x20) &&
             (B(13) == 0x54) &&
             (B(14) == 0xD1) &&
             (B(15) == 0xC8) )
        {
          if ( WANT_DATA && pw_test_Powerpacker40(c) != PW_BAD )
          {
            pw_rip_Powerpacker40(c);
          }
        }

        /* Super Cruncher 2.7 */
        if ( (B(1)  == 0xE7) &&
             (B(2)  == 0xFF) &&
             (B(3)  == 0xFE) &&
             (B(4)  == 0x2C) &&
             (B(5)  == 0x79) &&
             (B(10) == 0x4E) &&
             (B(11) == 0xAE) &&
             (B(12) == 0xFF) &&
             (B(13) == 0x7C) &&
             (B(14) == 0x41) &&
             (B(15) == 0xFA) )
        {
          if ( WANT_DATA && pw_test_SuperCruncher27(c) != PW_BAD )
          {
            pw_rip_SuperCruncher27(c);
          }
        }

        /* Crunchmania Address */
        if ((B(1) == 0xe7) &&
           (B(14) == 0x22) &&
           (B(15) == 0x1A) &&
           (B(16) == 0x24) &&
           (B(17) == 0x1A) &&
           (B(18) == 0x47) &&
           (B(19) == 0xEA) &&
           (B(24) == 0x6F) &&
           (B(25) == 0x1C) &&
           (B(26) == 0x26) &&
           (B(27) == 0x49) &&
           (B(28) == 0xD7) &&
           (B(29) == 0xC1) &&
           (B(30) == 0xB7) &&
           (B(31) == 0xCA) )
        {
          if ( WANT_DATA && pw_test_crunchmaniaAddr(c, 1) != PW_BAD )
          {
            pw_rip_CrunchmaniaAddr(c);
          }
        }

        /* Crunchmania Address (another)*/
        if ((B(1) == 0xe7) &&
           (B(14) == 0x20) &&
           (B(15) == 0x4C) &&
           (B(16) == 0x47) &&
           (B(17) == 0xFA) &&
           (B(18) == 0x00) &&
           (B(19) == 0x0C) &&
           (B(24) == 0x51) &&
           (B(25) == 0xCF) &&
           (B(26) == 0xFF) &&
           (B(27) == 0xFC) &&
           (B(28) == 0x4E) &&
           (B(29) == 0xD0) &&
           (B(30) == 0x43) &&
           (B(31) == 0xF9) )
        {
          if ( WANT_DATA && pw_test_crunchmaniaAddr(c, 1) != PW_BAD )
          {
            pw_rip_CrunchmaniaAddr(c);
          }
        }

        /* Crunchmania Address (another again)*/
        if ((B(1) == 0xe7) &&
           (B(14) == 0x1a) &&
           (B(15) == 0xbc) &&
           (B(16) == 0x00) &&
           (B(17) == 0xb9) &&
           (B(18) == 0x1a) &&
           (B(19) == 0xbc) &&
           (B(24) == 0x00) &&
           (B(25) == 0xe9) &&
           (B(26) == 0x1a) &&
           (B(27) == 0xbc) &&
           (B(28) == 0x00) &&
           (B(29) == 0xf1) &&
           (B(30) == 0x45) &&
           (B(31) == 0xfa) )
        {
          if ( WANT_DATA && pw_test_crunchmaniaAddr(c, 2) != PW_BAD )
          {
            pw_rip_CrunchmaniaAddr(c);
          }
        }

        /* Crunchmania Simple */
        if ((B(1) == 0xE7) &&
            (B(2) == 0xFF) &&
            (B(3) == 0xFF) &&
            (B(4) == 0x45) &&
            (B(5) == 0xFA) &&
            (B(6) == 0x01) &&
            (B(7) == 0x66) &&
            (B(8) == 0x22) &&
            (B(9) == 0x1A) &&
            (B(10) == 0x24) &&
            (B(11) == 0x1A) &&
            (B(12) == 0x22) &&
            (B(13) == 0x4A) &&
            (B(14) == 0x28) &&
            (B(15) == 0x7A) )
        {
          if ( WANT_DATA && pw_test_crunchmaniaSimple(c) != PW_BAD )
          {
            pw_rip_CrunchmaniaSimple(c);
          }
        }

        /* RelokIt 1.0 */
        if ( (B(1)  == 0xE7) &&
             (B(2)  == 0xFF) &&
             (B(3)  == 0xFE) &&
             (B(4)  == 0x41) &&
             (B(5)  == 0xFA) &&
             (B(6)  == 0x02) &&
             (B(7)  == 0xC6) &&
             (B(8)  == 0x70) &&
             (B(9)  == 0x00) &&
             (B(10) == 0x30) &&
             (B(11) == 0x28) &&
             (B(12) == 0x00) &&
             (B(13) == 0x04) &&
             (B(14) == 0x23) &&
             (B(15) == 0xC0) )
        {
          if ( WANT_DATA && pw_test_RelokIt10(c) != PW_BAD )
          {
            pw_rip_RelokIt10(c);
          }
        }

        /* Mega Cruncher Obj */
        if ( (B(1)  == 0xE7) &&
             (B(2)  == 0xFF) &&
             (B(3)  == 0xFE) &&
             (B(4)  == 0x2C) &&
             (B(5)  == 0x78) &&
             (B(6)  == 0x00) &&
             (B(7)  == 0x04) &&
             (B(8)  == 0x4B) &&
             (B(9)  == 0xFA) &&
             (B(10) == 0x01) &&
             (B(11) == 0xC0) )
        {
          if ( WANT_DATA && pw_test_MegaCruncherObj(c) != PW_BAD )
          {
            pw_rip_MegaCruncherObj(c);
          }
        }

        /* Turbo Squeezer 6.1 */
        if ( (B(1)  == 0xE7) &&
             (B(2)  == 0xFF) &&
             (B(3)  == 0xFE) &&
             (B(4)  == 0x2C) &&
             (B(5)  == 0x79) &&
             (B(6)  == 0x00) &&
             (B(7)  == 0x00) &&
             (B(8)  == 0x00) &&
             (B(9)  == 0x04) &&
             (B(10) == 0x20) &&
             (B(11) == 0x7A) )
        {
          if ( WANT_DATA && pw_test_TurboSqueezer61(c) != PW_BAD )
          {
            pw_rip_TurboSqueezer61(c);
          }
        }

        /* DragPack 2.52 */
        if ( (B(1)  == 0x7A) &&
             (B(2)  == 0x00) &&
             (B(3)  == 0x46) &&
             (B(4)  == 0x48) &&
             (B(5)  == 0xE7) &&
             (B(6)  == 0xFF) &&
             (B(7)  == 0xFE) &&
             (B(8)  == 0x49) &&
             (B(9)  == 0xFA) &&
             (B(10) == 0xFF) &&
             (B(11) == 0xEE) &&
             (B(12) == 0x28) &&
             (B(13) == 0xFC) &&
             (B(14) == 0x00) &&
             (B(15) == 0x00) )
        {
          if ( WANT_DATA && pw_test_Dragpack252(c) != PW_BAD )
          {
            pw_rip_Dragpack252(c);
          }
        }
        /* DragPack 1.00 */
        if ( (B(1)  == 0xE7) &&
             (B(2)  == 0xFF) &&
             (B(3)  == 0xFE) &&
             (B(4)  == 0x41) &&
             (B(5)  == 0xF9) &&
             (B(6)  == 0x00) &&
             (B(7)  == 0x00) &&
             (B(8)  == 0x00) &&
             (B(9)  == 0x00) &&
             (B(10) == 0x43) &&
             (B(11) == 0xF9) &&
             (B(12) == 0x00) &&
             (B(13) == 0x00) &&
             (B(14) == 0x00) &&
             (B(15) == 0x00) )
        {
          if ( WANT_DATA && pw_test_Dragpack100(c) != PW_BAD )
          {
            pw_rip_Dragpack100(c);
          }
        }
        /* GNU Packer 1.2 */
        if ( (B(1)  == 0xE7) &&
             (B(2)  == 0xFF) &&
             (B(3)  == 0xFE) &&
             (B(4)  == 0x4B) &&
             (B(5)  == 0xFA) &&
             (B(6)  == 0x02) &&
             (B(7)  == 0x32) &&
             (B(8)  == 0x4D) &&
             (B(9)  == 0xFA) &&
             (B(10) == 0x02) &&
             (B(11) == 0x46) &&
             (B(12) == 0x20) &&
             (B(13) == 0x6D) &&
             (B(14) == 0x00) &&
             (B(15) == 0x0C) )
        {
          if ( WANT_DATA && pw_test_GNUPacker12(c) != PW_BAD )
          {
            pw_rip_GNUPacker12(c);
          }
        }
        /* "HVL" - Hively tracker */
        if ( ( B(1) == 'V' ) &&
             ( B(2) == 'L' ) &&
             (( B(3) == 0x00 )||( B(3) == 0x01 )) )
        {
          if ( WANT_MUSIC && pw_test_THX(c) != PW_BAD )
          {
            pw_rip_THX(c);
          }
        }
        break;

      case 'I': /* 0x48 */
          /* "ICE!" : ID of IAM packer 1.0 */
        if ( (B(1) == 'C') &&
             (B(2) == 'E') &&
             (B(3) == '!') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 4, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "IAM Packer 1.0 (ICE!) data", 0, PWF_ICE);
          }
        }
          /* "Ice!" : ID of Ice! Cruncher */
        if ( (B(1) == 'c') &&
             (B(2) == 'e') &&
             (B(3) == '!') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 4, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Ice! Cruncher (data)", 0, PWF_ICE);
          }
        }
          /* "IMP!" */
        if ( (B(1) == 'M') &&
             (B(2) == 'P') &&
             (B(3) == '!') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Imploder data", 50, PWF_IMP);
          }
        }
          /* "IMPM" : ID of Impulse Tracker */
        if ( (B(1) == 'M') &&
             (B(2) == 'P') &&
             (B(3) == 'M') )
        {
          if ( WANT_MUSIC && pw_test_IT(c) != PW_BAD )
          {
            pw_rip_IT(c);
          }
        }
        if ( (B(1) == 'T') &&
             (B(2) == '1') &&
             (B(3) == '0') )
        {
          /* Ice Tracker 1.0 */
          if ( WANT_MUSIC && pw_test_STK26(c) != PW_BAD )
          {
            pw_rip_STK26(c);
            pw_depack_STK26(c);
          }
        }
        break;

      case 'K': /* 0x4B */
          /* "KRIS" : ID of Chip Tracker */
        if ( (B(1) == 'R') &&
             (B(2) == 'I') &&
             (B(3) == 'S') )
        {
          if ( WANT_MUSIC && pw_test_KRIS(c) != PW_BAD )
          {
            pw_rip_KRIS(c);
            pw_depack_KRIS(c);
          }
        }
        /* Try-It Cruncher 1.01 */
        if ( (B(1)  == 0xFA) &&
             (B(2)  == 0x01) &&
             (B(3)  == 0x54) &&
             (B(4)  == 0x24) &&
             (B(5)  == 0x6D) &&
             (B(6)  == 0x00) &&
             (B(7)  == 0x18) &&
             (B(8)  == 0xB3) &&
             (B(9)  == 0xED) &&
             (B(10) == 0x00) &&
             (B(11) == 0x18) &&
             (B(12) == 0x6F) &&
             (B(13) == 0x0E) &&
             (B(14) == 0x20) &&
             (B(15) == 0x4A) )
        {
          if ( WANT_DATA && pw_test_TryIt101(c) != PW_BAD )
          {
            pw_rip_TryIt101(c);
          }
        }
        break;

      case 'L': /* 0x4C */
          /* "LSD!" : ID of Automation 2.3r packer (Atari ST) */
        if ( (B(1) == 'S') &&
             (B(2) == 'D') &&
             (B(3) == '!') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Automation 2.3r Data Cruncher", 4, PWF_LSDDataCruncher);
          }
        }

          /* "LZH!" : ID of Jam Packer (LZH! compression) (Atari ST) */
        if ( (B(1) == 'Z') &&
             (B(2) == 'H') &&
             (B(3) == '!') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Jam Packer Data Cruncher", 12, PWF_JamDataCruncher);
          }
        }
        break;

      case 'M': /* 0x4D */
        /* MASM data cruncher */
        if ( (B(1) == 'A') &&
             (B(2) == 'S') &&
             (B(3) == 'M') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 4, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "MASM Data Cruncher", 0, PWF_MASMDataCruncher);
          }
        }

        if ( (B(1) == '.') &&
             (B(2) == 'K') &&
             (B(3) == '.') )
        {
          /* protracker ? */
          if ( WANT_MUSIC && pw_test_MOD(c, 4) != PW_BAD )
          {
            pw_rip_MOD(c, 4);
          }

          /* Unic tracker v1 ? */
          if ( WANT_MUSIC && pw_test_UNIC_withID(c) != PW_BAD )
          {
            pw_rip_UNIC_withID(c);
            pw_depack_UNIC(c);
          }

          /* Noiserunner ? */
          if ( WANT_MUSIC && pw_test_Noiserunner(c) != PW_BAD )
          {
            pw_rip_Noiserunner(c);
            pw_depack_Noiserunner(c);
          }

          /* Mosh packer ? */
          if ( WANT_MUSIC && pw_test_MOSH(c) != PW_BAD )
          {
            pw_rip_MOSH(c);
            pw_depack_MOSH(c);
          }
          /* HCD-protector ? */
          if ( WANT_MUSIC && pw_test_HCD(c) != PW_BAD )
          {
            pw_rip_HCD(c);
            pw_depack_HCD(c);
          }
        }

        if ( (B(1) == '1') &&
             (B(2) == '.') &&
             (B(3) == '0') )
        {
          /* Fuzzac packer */
          if ( WANT_MUSIC && pw_test_FUZZAC(c) != PW_BAD )
          {
            pw_rip_Fuzzac(c);
            pw_depack_Fuzzac(c);
          }
        }

        if ( (B(1) == 'E') &&
             (B(2) == 'X') &&
             (B(3) == 'X') )
        {
          /* tracker packer v2 */
          if ( WANT_MUSIC && pw_test_TP2(c) != PW_BAD )
          {
            pw_rip_TP2(c);
            pw_depack_TP2(c);
          }
          /* tracker packer v1 */
          if ( WANT_MUSIC && pw_test_TP1(c) != PW_BAD )
          {
            pw_rip_TP1(c);
            pw_depack_TP1(c);
          }
        }

        if ( B(1) == '.' )
        {
          /* Kefrens sound machine ? */
          if ( WANT_MUSIC && pw_test_KSM(c) != PW_BAD )
          {
            pw_rip_KSM(c);
            pw_depack_KSM(c);
          }
        }

        if ( (B(1) == 'O') &&
             (B(2) == 'D') &&
             (B(3) == 'U') )
        {
          /* NovoTrade */
          if ( WANT_MUSIC && pw_test_NovoTrade(c) != PW_BAD )
          {
            pw_rip_NovoTrade(c);
            pw_depack_NovoTrade(c);
            continue;
          }
        }

        if ( (B(1) == 'T') &&
             (B(2) == 'N') &&
             (B(3) == 0x00) )
        {
          /* SoundTracker 2.6 */
          if ( WANT_MUSIC && pw_test_STK26(c) != PW_BAD )
          {
            pw_rip_STK26(c);
            pw_depack_STK26(c);
          }
        }

        if ( (B(1) == 'M') &&
             (B(2) == 'D') &&
             ((B(3) == '0') ||
             (B(3) == '1')) )/* ||
             (B(3) == '2') ||
             (B(3) == '3')) )*/
        {
          /* MED (MMD0) */
          if ( WANT_MUSIC && pw_test_MMD0(c) != PW_BAD )
          {
            pw_rip_MMD0(c);
          }
          /* not in the original: an MMD0/1 a player has relocated in memory */
          else if ( NEW_FORMATS && WANT_MUSIC )
            pw_new_MMD_relocated(c);
        }
        /* not in the original: MMD2 / MMD3 (OctaMED Pro, Soundstudio) */
        else if ( NEW_FORMATS &&
             (B(1) == 'M') &&
             (B(2) == 'D') &&
             ((B(3) == '2') ||
              (B(3) == '3')) )
        {
          if ( WANT_MUSIC && !pw_new_MMD23(c) )
            pw_new_MMD_relocated(c);
        }

         /* Defjam Cruncher 3.2 pro */
        if ( (B(1)  == 0xF9 ) &&
             (B(2)  == 0x00 ) &&
             (B(3)  == 0xDF ) &&
             (B(4)  == 0xF0 ) &&
             (B(5)  == 0x00 ) &&
             (B(6)  == 0x7E ) &&
             (B(7)  == 0x00 ) &&
             (B(8)  == 0x30 ) &&
             (B(9)  == 0x3C ) &&
             (B(10) == 0x7F ) &&
             (B(11) == 0xFF ) &&
             (B(12) == 0x3D ) &&
             (B(13) == 0x40 ) &&
             (B(14) == 0x00 ) &&
             (B(15) == 0x96 ) )
        {
          if ( WANT_DATA && pw_test_Defjam32pro(c) != PW_BAD )
          {
            pw_rip_Defjam32(c);
          }
        }

         /* StoneCracker 2.99d */
        if ( (B(1)  == 0xF9 ) &&
             (B(2)  == 0x00 ) &&
             (B(3)  == 0xDF ) &&
             (B(4)  == 0xF0 ) &&
             (B(5)  == 0x00 ) &&
             (B(6)  == 0x4B ) &&
             (B(7)  == 0xfa ) &&
             (B(8)  == 0x01 ) &&
             (B(9)  == 0x54 ) &&
             (B(10) == 0x49 ) &&
             (B(11) == 0xf9 ) &&
             (B(12) == 0x00 ) &&
             (B(13) == 0xbf ) &&
             (B(14) == 0xd1 ) &&
             (B(15) == 0x00 ) )
        {
          if ( WANT_DATA && pw_test_STC299d(c) != PW_BAD )
          {
            pw_rip_STC299d(c);
          }
        }

         /* StoneCracker 2.99b */
        if ( (B(1)  == 0xF9 ) &&
             (B(2)  == 0x00 ) &&
             (B(3)  == 0xDF ) &&
             (B(4)  == 0xF0 ) &&
             (B(5)  == 0x00 ) &&
             (B(6)  == 0x4B ) &&
             (B(7)  == 0xfa ) &&
             (B(8)  == 0x01 ) &&
             (B(9)  == 0x58 ) &&
             (B(10) == 0x49 ) &&
             (B(11) == 0xf9 ) &&
             (B(12) == 0x00 ) &&
             (B(13) == 0xbf ) &&
             (B(14) == 0xd1 ) &&
             (B(15) == 0x00 ) )
        {
          if ( WANT_DATA && pw_test_STC299b(c) != PW_BAD )
          {
            pw_rip_STC299b(c);
          }
        }

         /* StoneCracker 2.99 */
        if ( (B(1)  == 0xF9 ) &&
             (B(2)  == 0x00 ) &&
             (B(3)  == 0xDF ) &&
             (B(4)  == 0xF0 ) &&
             (B(5)  == 0x00 ) &&
             (B(6)  == 0x4B ) &&
             (B(7)  == 0xfa ) &&
             (B(8)  == 0x01 ) &&
             (B(9)  == 0x5c ) &&
             (B(10) == 0x49 ) &&
             (B(11) == 0xf9 ) &&
             (B(12) == 0x00 ) &&
             (B(13) == 0xbf ) &&
             (B(14) == 0xd1 ) &&
             (B(15) == 0x00 ) )
        {
          if ( WANT_DATA && pw_test_STC299(c) != PW_BAD )
          {
            pw_rip_STC299(c);
          }
        }

         /* StoneCracker 3.00 */
        if ( (B(1)  == 0xF9 ) &&
             (B(2)  == 0x00 ) &&
             (B(3)  == 0xDF ) &&
             (B(4)  == 0xF0 ) &&
             (B(5)  == 0x96 ) &&
             (B(6)  == 0x4b ) &&
             (B(7)  == 0xfa ) &&
             (B(8)  == 0x01 ) &&
             (B(9)  == 0x5c ) &&
             (B(10) == 0x49 ) &&
             (B(11) == 0xf9 ) &&
             (B(12) == 0x00 ) &&
             (B(13) == 0xbf ) &&
             (B(14) == 0xd1 ) &&
             (B(15) == 0x00 ) )
        {
          if ( WANT_DATA && pw_test_STC300(c) != PW_BAD )
          {
            pw_rip_STC300(c);
          }
        }

         /* StoneCracker 3.10 */
        if ( (B(1)  == 0xF9 ) &&
             (B(2)  == 0x00 ) &&
             (B(3)  == 0xDF ) &&
             (B(4)  == 0xF1 ) &&
             (B(5)  == 0x80 ) &&
             (B(6)  == 0x47 ) &&
             (B(7)  == 0xf9 ) &&
             (B(8)  == 0x00 ) &&
             (B(9)  == 0xbf ) &&
             (B(10) == 0xd1 ) &&
             (B(11) == 0x00 ) &&
             (B(12) == 0x41 ) &&
             (B(13) == 0xfa ) &&
             (B(14) == 0x00 ) &&
             (B(15) == 0x62 ) )
        {
          if ( WANT_DATA && pw_test_STC310(c) != PW_BAD )
          {
            pw_rip_STC310(c);
          }
        }

          /* Mental Image Packer (data) */
        if ( (B(1) == 'I') &&
             (B(2) == '1') &&
             (B(3) == '0') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 12, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Mental Image Packer", 18, PWF_MentalImage);
          }
        }
        break;

      case 'N': /* Sonic Arranger (no hunk) */
        if ( (B(1) == 0xFA) &&
             (B(2) == 0x00) &&
             (B(4) == 'N') &&
             (B(5) == 0xFA) &&
             (B(8) == 'N') &&
             (B(9) == 0xFA) &&
             (B(12) == 'N') &&
             (B(13) == 0xFA) &&
             (B(16) == 'N') &&
             (B(17) == 0xFA) &&
             (B(20) == 'N') &&
             (B(21) == 0xFA) &&
             (B(24) == 'N') &&
             (B(25) == 0xFA))
        {
          if ( WANT_MUSIC && pw_test_SA(c) != PW_BAD )
          {
            pw_rip_SA(c);
          }
        }
        break;

      case 'O': /* 0x4F */
          /* "OKTASONG" : ID of Oktalizer */
        if ( (B(1) == 'K') &&
             (B(2) == 'T') &&
             (B(3) == 'A') &&
             (B(4) == 'S') &&
             (B(5) == 'O') &&
             (B(6) == 'N') &&
             (B(7) == 'G') &&
             (B(8) == 'C') &&
             (B(9) == 'M') &&
             (B(10) == 'O') &&
             (B(11) == 'D') )
        {
          if ( WANT_MUSIC && pw_test_Okta(c) != PW_BAD )
          {
            pw_rip_Okta(c);
          }
        }
        break;

      case 'P': /* 0x50 */
          /* "PP20" : ID of PowerPacker */
        if ( WANT_DATA &&
             (B(1) == 'P') &&
             (B(2) == '2') &&
             (B(3) == '0') )
        {
          pw_printf(c,  "PowerPacker ID (PP20) found at %u ... cant rip it!\n" , c->i );
          pw_report_unrippable(c, PWF_PP20, "PowerPacker (PP20)", c->i);
        }
          /* "P30A" : ID of The Player */
        if ( (B(1) == '3') &&
             (B(2) == '0') &&
             (B(3) == 'A') )
        {
          if ( WANT_MUSIC && pw_test_P40A(c) != PW_BAD ) /* yep same tests apply */
          {
            pw_rip_P30A(c);
            pw_depack_P30(c);
          }
        }

          /* "P22A" : ID of The Player */
        if ( (B(1) == '2') &&
             (B(2) == '2') &&
             (B(3) == 'A') )
        {
          if ( WANT_MUSIC && pw_test_P40A(c) != PW_BAD ) /* yep, same tests apply */
              {
            pw_rip_P22A(c);
            pw_depack_P22(c);
          }
        }

          /* "P40A" : ID of The Player */
        if ( (B(1) == '4') &&
             (B(2) == '0') &&
             (B(3) == 'A') )
        {
          if ( WANT_MUSIC && pw_test_P40A(c) != PW_BAD )
          {
            pw_rip_P40A(c);
            pw_depack_P40(c);
          }
        }

          /* "P40B" : ID of The Player */
        if ( (B(1) == '4') &&
             (B(2) == '0') &&
             (B(3) == 'B') )
        {
          if ( WANT_MUSIC && pw_test_P40A(c) != PW_BAD )
          {
            pw_rip_P40B(c);
            pw_depack_P40(c);
          }
        }

          /* "P41A" : ID of The Player */
        if ( (B(1) == '4') &&
             (B(2) == '1') &&
             (B(3) == 'A') )
        {
          if ( WANT_MUSIC && pw_test_P41A(c) != PW_BAD )
          {
            pw_rip_P41A(c);
            pw_depack_P41A(c);
          }
        }

          /* "PMd3/PMD3 : ID of ?!? TSCC format - 8CHN */
        if ( (B(1) == 'M') &&
             ((B(2) == 'd') || (B(2) == 'D')) &&
             (B(3) == '3') )
        {
          if ( WANT_MUSIC && pw_test_PMD3(c) != PW_BAD ) /* yep, same tests apply */
              {
            pw_rip_PMD3(c);
            pw_depack_PMD3(c);
          }
        }

          /* "PM40" : ID of Promizer 4 */
        if ( (B(1) == 'M') &&
             (B(2) == '4') &&
             (B(3) == '0') )
        {
          if ( WANT_MUSIC && pw_test_PM40(c) != PW_BAD )
          {
            pw_rip_PM40(c);
            pw_depack_PM40(c);
          }
        }

          /* "PPbk" : ID of AMOS PowerPacker Bank */
        if ( (B(1) == 'P') &&
             (B(2) == 'b') &&
             (B(3) == 'k') )
        {
          if ( WANT_DATA && pw_test_PPbk(c) != PW_BAD )
          {
            pw_rip_PPbk(c);
          }
        }

          /* PARA data Cruncher */
        if ( (B(1) == 'A') &&
             (B(2) == 'R') &&
             (B(3) == 'A') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "PARA Data Cruncher", 46, PWF_PARA);
          }
        }

          /* Pac1 data Cruncher */
        if ( (B(1) == 'a') &&
             (B(2) == 'c') &&
             (B(3) == '1') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Pac1 Data Cruncher", 12, PWF_Pac1);
          }
        }

          /* Master cruncher 3.0 data */
        if ( (B(1) == 'A') &&
             (B(2) == 'C') &&
             (B(3) == 'K') &&
             (B(4) == 'V') &&
             (B(5) == '1') &&
             (B(6) == '.') &&
             (B(7) == '2') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 12, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Master Cruncher 3.0 data", 8, PWF_MasterCruncher3data);
          }
        }
          /* POLKA Packer */
        if ( ((B(1) == 'W') &&
             (B(2) == 'R') &&
             (B(3) == '.')) ||
             ((B(1) == 'S') &&
             (B(2) == 'U') &&
             (B(3) == 'X')))
        {
          if ( WANT_MUSIC && pw_test_Polka(c) != PW_BAD )
          {
            pw_rip_Polka(c);
            pw_depack_Polka(c);
          }
        }

          /* PERFSONG Packer */
        if ( (B(1) == 'E') &&
             (B(2) == 'R') &&
             (B(3) == 'F') &&
             (B(4) == 'S') &&
             (B(5) == 'O') &&
             (B(6) == 'N') &&
             (B(7) == 'G'))
        {
          if ( WANT_MUSIC && pw_test_PERFSONG(c) != PW_BAD )
          {
            pw_rip_PERFSONG(c);
            pw_depack_PERFSONG(c);
          }
        }
        break;

      case 'R':  /* RNC */
        if ( (B(1) == 'N') &&
             (B(2) == 'C') )
        {
          /* RNC */
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Propack (RNC) data", 18, PWF_RNC);
          }
        }
          /* RLE Data Cruncher */
        if ( (B(1) == 'L') &&
             (B(2) == 'E') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "RLE Data Cruncher", 11, PWF_RLE);
          }
        }
        break;

      case 'S':  /* 0x53 */
          /* SpeedPacker (SP20) Data Cruncher */
        if ( (B(1) == 'P') &&
             (B(2) == '2') &&
             (B(3) == '0') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 12) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "SpeedPacker (SP20) Data Cruncher", 16, PWF_SP20);
          }
        }
          /* "SNT!" ProRunner 2 */
        if ( (B(1) == 'N') &&
             (B(2) == 'T') &&
             (B(3) == '!') )
        {
          if ( WANT_MUSIC && pw_test_PRUN2(c) != PW_BAD )
          {
            pw_rip_PRUN2(c);
            pw_depack_PRUN2(c);
          }
        }
          /* "SNT." ProRunner 1 */
        if ( (B(1) == 'N') &&
             (B(2) == 'T') &&
             (B(3) == '.') )
        {
          if ( WANT_MUSIC && pw_test_PRUN1(c) != PW_BAD )
          {
            pw_rip_PRUN1(c);
            pw_depack_PRUN1(c);
          }
        }

          /* SKYT packer */
        if ( (B(1) == 'K') &&
             (B(2) == 'Y') &&
             (B(3) == 'T') )
        {
          if ( WANT_MUSIC && pw_test_SKYT(c) != PW_BAD )
          {
            pw_rip_SKYT(c);
            pw_depack_SKYT(c);
          }
        }

          /* SMOD Future Composer 1.0 - 1.3 */
        if ( (B(1) == 'M') &&
             (B(2) == 'O') &&
             (B(3) == 'D') )
        {
          if ( WANT_MUSIC && pw_test_FC13(c) != PW_BAD )
          {
            pw_rip_FC13(c);
          }
        }

         /* SIDMON 2 */

          /* S404 StoneCracker 4.04 data */
        if ( (B(1) == '4') &&
             (B(2) == '0') &&
             (B(3) == '4') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 12, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "StoneCracker 4.04 data", 18, PWF_S404);
          }
        }

          /* S403 StoneCracker 4.03 data */
        if ( (B(1) == '4') &&
             (B(2) == '0') &&
             (B(3) == '3') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 12, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "StoneCracker 4.03 data", 20, PWF_S404);
          }
        }

          /* S401 StoneCracker 4.01 data */
        if ( (B(1) == '4') &&
             (B(2) == '0') &&
             (B(3) == '1') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "StoneCracker 4.01 data", 12, PWF_S404);
          }
        }
          /* S400 StoneCracker 4.00 data */
        if ( (B(1) == '4') &&
             (B(2) == '0') &&
             (B(3) == '0') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "StoneCracker 4.00 data", 12, PWF_S404);
          }
        }

          /* S310 StoneCracker 3.10 data */
        if ( (B(1) == '3') &&
             (B(2) == '1') &&
             (B(3) == '0') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "StoneCracker 3.10 data", 16, PWF_S404);
          }
        }

          /* S300 StoneCracker 3.00 data */
        if ( (B(1) == '3') &&
             (B(2) == '0') &&
             (B(3) == '0') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 12, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "StoneCracker 3.00 data", 16, PWF_S404);
          }
        }

          /* "SCRM" : ID of ScreamTracker 3 S3M */
        if ( (B(1) == 'C') &&
             (B(2) == 'R') &&
             (B(3) == 'M') )
        {
          if ( WANT_MUSIC && pw_test_S3M(c) != PW_BAD )
          {
            pw_rip_S3M(c);
          }
        }

          /* SC data cruncher */

          /* SC data cruncher */

          /* SF data cruncher */

          /* SQ data cruncher */

          /* SP data cruncher */

          /* STIM Slamtilt */
        if ( (B(1) == 'T') &&
             (B(2) == 'I') &&
             (B(3) == 'M') )
        {
          if ( WANT_MUSIC && pw_test_STIM(c) != PW_BAD )
          {
            pw_rip_STIM(c);
            pw_depack_STIM(c);
          }
        }

        /* SA hunk */
        if ( (B(1) == 'O') &&
             (B(2) == 'A') &&
             (B(3) == 'R') &&
             (B(4) == 'V') &&
             (B(5) == '1') &&
             (B(6) == '.') &&
             (B(7) == '0') )
        {
          if ( WANT_MUSIC && pw_test_SAhunk(c) != PW_BAD )
          {
            pw_rip_SA(c);
          }
        }

          /* SPv3 - Speed Packer 3 (Atari ST) */
        if ( (B(1) == 'P') &&
             (B(2) == 'v') &&
             (B(3) == '3') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 12) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Speed Packer 3 data", 0, PWF_SpeedPacker3Data);
          }
        }
          /* SONG Fuchs Tracker */
        if ( (B(1) == 'O') &&
             (B(2) == 'N') &&
             (B(3) == 'G') )
        {
          if ( WANT_MUSIC && pw_test_FuchsTracker(c) != PW_BAD )
          {
            pw_rip_FuchsTracker(c);
            pw_depack_FuchsTracker(c);
          }
          /* Sound FX */
          if ( WANT_MUSIC && pw_test_SoundFX13(c) != PW_BAD )
          {
            pw_rip_SoundFX13(c);
            pw_depack_SoundFX13(c);
          }
        }
        break;

      case 'T': /* "THX" - AHX */
        if ( ( B(1) == 'H' ) &&
             ( B(2) == 'X' ) &&
             (( B(3) == 0x00 )||( B(3) == 0x01 )) )
        {
          if ( WANT_MUSIC && pw_test_THX(c) != PW_BAD )
          {
            pw_rip_THX(c);
          }
        }

          /* "TRK1" Module Protector */
        if ( ( B(1) == 'R' ) &&
             ( B(2) == 'K' ) &&
             ( B(3) == '1' ) )
        {
          /* Module Protector */
          if ( WANT_MUSIC && pw_test_MP_withID(c) != PW_BAD )
          {
            pw_rip_MP_withID(c);
            pw_depack_MP(c);
          }
        }

          /* "TMK. Timetracker ?!? */
        if ( ( B(1) == 'M' ) &&
             ( B(2) == 'K' ) &&
             ( B(3) == 0x01 ) )
        {
          if ( WANT_MUSIC && pw_test_TMK(c) != PW_BAD )
          {
            pw_rip_TMK(c);
            pw_depack_TMK(c);
          }
        }
        break;

      case 'U': /* "UNIC" */
        if ( ( B(1) == 'N' ) &&
             ( B(2) == 'I' ) &&
             ( B(3) == 'C' ) )
        {
          /* Unic tracker v1 ? */
          if ( WANT_MUSIC && pw_test_UNIC_withID(c) != PW_BAD )
          {
            pw_rip_UNIC_withID(c);
            pw_depack_UNIC(c);
          }
        }
        /* Mugician */
        if ( ( B(1) == 'G' ) &&
             ( B(2) == 'I' ) &&
             ( B(3) == 'C' ) )
        {
          /*  */
          if ( WANT_MUSIC && pw_test_MUGICIAN(c) != PW_BAD )
          {
            pw_rip_MUGICIAN(c);
          }
        }
        break;

      case 'V': /* "V.2" */
        if ( (( B(1) == '.' ) &&
              ( B(2) == '2' )) ||
             (( B(1) == '.' ) &&
              ( B(2) == '3' )) )
        {
          /* Sound Monitor v2/v3 */
          if ( WANT_MUSIC && pw_test_BP(c) != PW_BAD )
          {
            pw_rip_BP(c);
          }
        }
          /* Virtual Dreams VDCO data cruncher */
        if ( (B(1) == 'D') &&
             (B(2) == 'C') &&
             (B(3) == 'O') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 12, 8) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "Virtual Dreams (VDCO) data cruncher", 13, PWF_VDCO);
          }
        }
        break;

      case 'W': /* 0x57 */
          /* "WN" Wanton Packer */
        if ( (B(1) == 'N') &&
             (B(2) == 0x00 ) )
        {
          if ( WANT_MUSIC && pw_test_WN(c) != PW_BAD )
          {
            pw_rip_WN(c);
            pw_depack_WN(c);
          }
        }
        break;

      case 'X': /* XPKF */
          /* xpk'ed file */
        if ( (B(1) == 'P') &&
             (B(2) == 'K') &&
             (B(3) == 'F') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 4, 12) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "XPK", 8, PWF_XPK);
          }
        }
        break;

      case 0x60:
          /* promizer 1.8a ? */
        if ( (B(1) == 0x38) &&
             (B(2) == 0x60) &&
             (B(3) == 0x00) &&
             (B(4) == 0x00) &&
             (B(5) == 0xa0) &&
             (B(6) == 0x60) &&
             (B(7) == 0x00) &&
             (B(8) == 0x01) &&
             (B(9) == 0x3e) &&
             (B(10)== 0x60) &&
             (B(11)== 0x00) &&
             (B(12)== 0x01) &&
             (B(13)== 0x0c) &&
             (B(14)== 0x48) &&
             (B(15)== 0xe7) )   /* gosh !, should be enough :) */
        {
          if ( WANT_MUSIC && pw_test_PMZ(c) != PW_BAD )
          {
            pw_rip_PM18a(c);
            pw_depack_PM18a(c);
          }

          /* Promizer 1.0c */
          if ( WANT_MUSIC && pw_test_PM10c(c) != PW_BAD )
          {
            pw_rip_PM10c(c);
            pw_depack_PM10c(c);
          }
        }

          /* promizer 2.0 ? */
        if ( (B(1) == 0x00) &&
             (B(2) == 0x00) &&
             (B(3) == 0x16) &&
             (B(4) == 0x60) &&
             (B(5) == 0x00) &&
             (B(6) == 0x01) &&
             (B(7) == 0x40) &&
             (B(8) == 0x60) &&
             (B(9) == 0x00) &&
             (B(10)== 0x00) &&
             (B(11)== 0xf0) &&
             (B(12)== 0x3f) &&
             (B(13)== 0x00) &&
             (B(14)== 0x10) &&
             (B(15)== 0x3a) )   /* gosh !, should be enough :) */
        {
          if ( WANT_MUSIC && pw_test_PM2(c) != PW_BAD )
          {
            pw_rip_PM20(c);
            pw_depack_PM20(c);
          }
        }

          /* Spike Cruncher */
        if ( (B(1)  == 0x16) &&
             (B(24) == 0x48) &&
             (B(25) == 0xE7) &&
             (B(26) == 0xFF) &&
             (B(27) == 0xFE) &&
             (B(28) == 0x26) &&
             (B(29) == 0x7A) &&
             (B(30) == 0xFF) &&
             (B(31) == 0xDE) &&
             (B(32) == 0xD7) &&
             (B(33) == 0xCB) &&
             (B(34) == 0xD7) &&
             (B(35) == 0xCB) &&
             (B(36) == 0x58) &&
             (B(37) == 0x8B) )
        {
          if ( WANT_DATA && pw_test_SpikeCruncher(c) != PW_BAD )
          {
            pw_rip_SpikeCruncher(c);
          }
        }
        break;

      case 0x61: /* "a" */
         /* TNM Cruncher 1.1 */
        if ( (B(1)  == 0x06) &&
             (B(2)  == 0x4E) &&
             (B(3)  == 0xF9) &&
             (B(4)  == 0x00) &&
             (B(5)  == 0x00) &&
             (B(6)  == 0x00) &&
             (B(7)  == 0x00) &&
             (B(8)  == 0x48) &&
             (B(9)  == 0xE7) &&
             (B(10) == 0xFF) &&
             (B(11) == 0xFE) &&
             (B(12) == 0x2C) &&
             (B(13) == 0x78) &&
             (B(14) == 0x00) &&
             (B(15) == 0x04) )
        {
          if ( WANT_DATA && pw_test_TNMCruncher11(c) != PW_BAD )
          {
            pw_rip_TNMCruncher11(c);
          }
        }

          /* "arcD" data cruncher */
        if ( (B(1)  == 'r') &&
             (B(2)  == 'c') &&
             (B(3)  == 'D'))
        {
          if ( WANT_DATA && pw_test_ArcDDataCruncher(c) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "arcD data Cruncher", 0, PWF_arcD);
          }
        }

        /* Tetrapack 2.1 */
        if ( (B(1)  == 0x00) &&
             (B(2)  == 0x41) &&
             (B(3)  == 0xFA) &&
             (B(4)  == 0x00) &&
             (B(5)  == 0xE4) &&
             (B(6)  == 0x4B) &&
             (B(7)  == 0xF9) &&
             (B(8)  == 0x00) &&
             (B(9)  == 0xDF) &&
             (B(12) == 0xD1) &&
             (B(13) == 0xFC) )
        {
          if ( WANT_DATA && pw_test_Tetrapack_2_1(c) != PW_BAD )
          {
            pw_rip_Tetrapack_2_1(c);
          }
        }

        /* Tetrapack 2.2 */
        if ( (B(1)  == 0x00) &&
             (B(2)  == 0x43) &&
             (B(3)  == 0xFA) &&
             (B(4)  == 0x00) &&
             (B(5)  == 0xFC) &&
             (B(12) == 0x28) &&
             (B(13) == 0x7A) )
        {
          if ( WANT_DATA && pw_test_Tetrapack_2_2(c) != PW_BAD )
            pw_rip_Tetrapack_2_2(c);
        }
        /* High Pressure Cruncher */
        if ( (B(1)  == 0x00) &&
             (B(2)  == 0x00) &&
             (B(3)  == 0xA8) &&
             (B(4)  == 0x20) &&
             (B(5)  == 0x7C) &&
             (B(10) == 0x22) &&
             (B(11) == 0x7c) &&
             (B(16) == 0x24) &&
             (B(17) == 0x48) &&
             (B(18) == 0x26) &&
             (B(19) == 0x49) &&
             (B(20) == 0x61))
        {
          if ( WANT_DATA && pw_test_HighPressureCruncher(c) != PW_BAD )
          {
            pw_rip_HighPressureCruncher(c);
          }
        }
        break;

      case 'x': /* xVdg */
          /* AMOS sub file */
        if ( (B(1) == 'V') &&
             (B(2) == 'd') &&
             (B(3) == 'g') )
        {
          if ( WANT_DATA && pw_test_special_cruncher_data(c, 8, 4) != PW_BAD )
          {
            pw_rip_special_cruncher_data(c, "xVdg", 12, PWF_xVdg);
          }
        }
        break;

      case 0x7E:
        /* Tetrapack 2.2 case #2 */
        if ( (B(1)  == 0x00) &&
             (B(2)  == 0x43) &&
             (B(3)  == 0xFA) &&
             (B(4)  == 0x00) &&
             (B(5)  == 0xFC) &&
             (B(12) == 0x28) &&
             (B(13) == 0x7A) )
        {
          if ( WANT_DATA && pw_test_Tetrapack_2_2(c) != PW_BAD )
            pw_rip_Tetrapack_2_2(c);
        }

        /* Tetrapack 2.1 case #2*/
        if ( (B(1)  == 0x00) &&
             (B(2)  == 0x41) &&
             (B(3)  == 0xFA) &&
             (B(4)  == 0x00) &&
             (B(5)  == 0xE4) &&
             (B(6)  == 0x4B) &&
             (B(7)  == 0xF9) &&
             (B(8)  == 0x00))
        {
          if ( WANT_DATA && pw_test_Tetrapack_2_1(c) != PW_BAD )
          {
            pw_rip_Tetrapack_2_1(c);
          }
        }

        /* Defjam Cruncher 3.2T */
        if ( (B(1)  == 0x00) &&
             (B(2)  == 0x43) &&
             (B(3)  == 0xFA) &&
             (B(4)  == 0x02) &&
             (B(5)  == 0x8C) &&
             (B(6)  == 0x4B) &&
             (B(7)  == 0xF9) &&
             (B(8)  == 0x00) &&
             (B(9)  == 0xDF) &&
             (B(10) == 0xF1))
        {
          if ( WANT_DATA && pw_test_Defjam32t(c) != PW_BAD )
          {
            pw_rip_Defjam32(c);
          }
        }
       break;
     case 0x80:
          /* Viruz2 8000 */

       break;

      case 0xAC:
          /* AC1D packer ?!? */
        if ( B(1) == 0x1D )
        {
          if ( WANT_MUSIC && pw_test_AC1D(c) != PW_BAD )
          {
            pw_rip_AC1D(c);
            pw_depack_AC1D(c);
          }
        }
        break;

      case 0xB4:
          /* DIET PC data packer */
        if ( (B(1)  == 0x4C) &&
             (B(2)  == 0xCD) &&
             (B(3)  == 0x21) &&
             (B(4)  == 0x9D) &&
             (B(5)  == 0x89) &&
             ( ( (B(6)  == 0x64) &&
             (B(7)  == 0x6C) &&
             (B(8)  == 0x7A) ) ||
             ( (B(6)  == 0x45) &&
             (B(7)  == 0x4F) &&
             (B(8)  == 0x53) ) ) )
        {
          if ( WANT_DATA && pw_test_DietDataPacker(c) != PW_BAD )
          {
            pw_rip_DietDataPacker(c);
          }
        }
        break;

      case 0xC0:
          /* Pha Packer */
        if ( (c->i >= 1) && (B(-1) == 0x03) )
        {
          if ( WANT_MUSIC && pw_test_PHA(c) != PW_BAD )
          {
            pw_rip_PHA(c);
            pw_depack_PHA(c);
          }
        }
        break;

      case 0xE0:

        break;

      default: /* do nothing ... save continuing :) */
        break;

    } /* end of switch */

    /* not in the original: the formats it never had, at the position the
     * checks above leave (the original's handlers always come first) */
    if ( NEW_FORMATS && !c->fatal )
      pw_scan_new(c);
  }
}
