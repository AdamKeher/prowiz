/* formats.h - format handlers called by the scanner (one file per original r/ source file). */
#ifndef PW_FORMATS_H
#define PW_FORMATS_H

#include <stdio.h>

#include "../pw_internal.h"

/* DietDataPacker.c  (upstream misc/misc.c) */
void pw_rip_DietDataPacker(pw_ctx *c);
int16_t pw_test_DietDataPacker(pw_ctx *c);

/* AC1D_packer.c  (upstream r/AC1D_packer.c) */
void pw_depack_AC1D(pw_ctx *c);
void pw_rip_AC1D(pw_ctx *c);
int16_t pw_test_AC1D(pw_ctx *c);

/* AMOS-MusicBank.c  (upstream r/AMOS-MusicBank.c) */
void pw_depack_AmBk(pw_ctx *c);
void pw_rip_AmBk(pw_ctx *c);
int16_t pw_test_AmBk(pw_ctx *c);

/* AMOS-PPBk.c  (upstream r/AMOS-PPBk.c) */
void pw_rip_PPbk(pw_ctx *c);
int16_t pw_test_PPbk(pw_ctx *c);

/* bnr.c  (upstream r/bnr.c) */
void pw_depack_BNR(pw_ctx *c);
void pw_rip_BNR(pw_ctx *c);
int16_t pw_test_BNR(pw_ctx *c);

/* BP.c  (upstream r/BP.c) */
void pw_rip_BP(pw_ctx *c);
int16_t pw_test_BP(pw_ctx *c);

/* BSI-FutureComposer.c  (upstream r/BSI-FutureComposer.c) */
void pw_rip_BSIFutureComposer(pw_ctx *c);
int16_t pw_test_BSIFutureComposer(pw_ctx *c);

/* Bytekiller.c  (upstream r/Bytekiller.c) */
void pw_rip_BKClone5(pw_ctx *c);
void pw_rip_BKCloneFLT(pw_ctx *c);
void pw_rip_ByteKiller(pw_ctx *c);
void pw_rip_ByteKiller30(pw_ctx *c);
void pw_rip_bytekillerpro10(pw_ctx *c);
int16_t pw_test_BKClone5(pw_ctx *c);
int16_t pw_test_BKCloneFLT(pw_ctx *c);
int16_t pw_test_ByteKiller30(pw_ctx *c);
int16_t pw_test_ByteKiller_13(pw_ctx *c);
int16_t pw_test_ByteKiller_20(pw_ctx *c);
int16_t pw_test_bytekillerpro10(pw_ctx *c);

/* ChipTracker.c  (upstream r/ChipTracker.c) */
void pw_depack_KRIS(pw_ctx *c);
void pw_rip_KRIS(pw_ctx *c);
int16_t pw_test_KRIS(pw_ctx *c);

/* Crunchmania-Address.c  (upstream r/Crunchmania-Address.c) */
void pw_rip_CrunchmaniaAddr(pw_ctx *c);
int16_t pw_test_crunchmaniaAddr(pw_ctx *c, int arg);

/* Crunchmania-Simple.c  (upstream r/Crunchmania-Simple.c) */
void pw_rip_CrunchmaniaSimple(pw_ctx *c);
int16_t pw_test_crunchmaniaSimple(pw_ctx *c);

/* datacrunchers.c  (upstream r/datacrunchers.c) */
int16_t pw_test_ArcDDataCruncher(pw_ctx *c);
int16_t pw_test_B9AB(pw_ctx *c);
int16_t pw_test_CRND(pw_ctx *c);

/* DefjamCr.c  (upstream r/DefjamCr.c) */
void pw_rip_Defjam32(pw_ctx *c);
int16_t pw_test_Defjam32(pw_ctx *c);
int16_t pw_test_Defjam32pro(pw_ctx *c);
int16_t pw_test_Defjam32t(pw_ctx *c);

/* DigiBooster17.c  (upstream r/DigiBooster17.c) */
void pw_rip_DigiBooster17(pw_ctx *c);
int16_t pw_test_DigiBooster17(pw_ctx *c);

/* DigitalIllusion.c  (upstream r/DigitalIllusion.c) */
void pw_depack_DI(pw_ctx *c);
void pw_rip_DI(pw_ctx *c);
int16_t pw_test_DI(pw_ctx *c);

/* DM1.c  (upstream r/DM1.c) */
void pw_rip_DM1(pw_ctx *c);
int16_t pw_test_DM1(pw_ctx *c);

/* DoubleAction.c  (upstream r/DoubleAction.c) */
void pw_rip_DoubleAction10(pw_ctx *c);
int16_t pw_test_DoubleAction10(pw_ctx *c);

/* Dragpack100.c  (upstream r/Dragpack100.c) */
void pw_rip_Dragpack100(pw_ctx *c);
int16_t pw_test_Dragpack100(pw_ctx *c);

/* Dragpack252.c  (upstream r/Dragpack252.c) */
void pw_rip_Dragpack252(pw_ctx *c);
int16_t pw_test_Dragpack252(pw_ctx *c);

/* EurekaPacker.c  (upstream r/EurekaPacker.c) */
void pw_depack_EUREKA(pw_ctx *c);
void pw_rip_EUREKA(pw_ctx *c);
int16_t pw_test_EUREKA(pw_ctx *c);

/* FC-M-Packer.c  (upstream r/FC-M-Packer.c) */
void pw_depack_FC_M(pw_ctx *c);
void pw_rip_FC_M(pw_ctx *c);
int16_t pw_test_FC_M(pw_ctx *c);

/* FC13.c  (upstream r/FC13.c) */
void pw_rip_FC13(pw_ctx *c);
int16_t pw_test_FC13(pw_ctx *c);

/* FC14.c  (upstream r/FC14.c) */
void pw_rip_FC14(pw_ctx *c);
int16_t pw_test_FC14(pw_ctx *c);

/* FT2.c  (upstream r/FT2.c) */
void pw_rip_XM(pw_ctx *c);
int16_t pw_test_XM(pw_ctx *c);

/* FuchsTracker.c  (upstream r/FuchsTracker.c) */
void pw_depack_FuchsTracker(pw_ctx *c);
void pw_rip_FuchsTracker(pw_ctx *c);
int16_t pw_test_FuchsTracker(pw_ctx *c);

/* FuzzacPacker.c  (upstream r/FuzzacPacker.c) */
void pw_depack_Fuzzac(pw_ctx *c);
void pw_rip_Fuzzac(pw_ctx *c);
int16_t pw_test_FUZZAC(pw_ctx *c);

/* GMC.c  (upstream r/GMC.c) */
void pw_depack_GMC(pw_ctx *c);
void pw_rip_GMC(pw_ctx *c);
int16_t pw_test_GMC(pw_ctx *c);

/* GNUPacker12.c  (upstream r/GNUPacker12.c) */
void pw_rip_GNUPacker12(pw_ctx *c);
int16_t pw_test_GNUPacker12(pw_ctx *c);

/* GnuPlayer.c  (upstream r/GnuPlayer.c) */
void pw_depack_GnuPlayer(pw_ctx *c);
void pw_rip_GnuPlayer(pw_ctx *c);
int16_t pw_test_GnuPlayer(pw_ctx *c);

/* GPMO.c  (upstream r/GPMO.c) */
void pw_depack_GPMO(pw_ctx *c);
void pw_rip_GPMO(pw_ctx *c);
int16_t pw_test_GPMO(pw_ctx *c);

/* HCD.c  (upstream r/HCD.c) */
void pw_depack_HCD(pw_ctx *c);
void pw_rip_HCD(pw_ctx *c);
int16_t pw_test_HCD(pw_ctx *c);

/* HeatseekerMC10.c  (upstream r/HeatseekerMC10.c) */
void pw_depack_HEATSEEKER(pw_ctx *c);
void pw_rip_HEATSEEKER(pw_ctx *c);
int16_t pw_test_HEATSEEKER(pw_ctx *c);

/* HighPresCr.c  (upstream r/HighPresCr.c) */
void pw_rip_HighPressureCruncher(pw_ctx *c);
int16_t pw_test_HighPressureCruncher(pw_ctx *c);

/* HornetPacker.c  (upstream r/HornetPacker.c) */
void pw_depack_HRT(pw_ctx *c);
void pw_rip_HRT(pw_ctx *c);
int16_t pw_test_HRT(pw_ctx *c);

/* HQC2.c  (upstream r/HQC2.c) */
void pw_rip_HQCCruncher2(pw_ctx *c);
int16_t pw_test_HQCCruncher2(pw_ctx *c);

/* IFF.c  (upstream r/IFF.c) */
void pw_rip_IFF(pw_ctx *c);
int16_t pw_test_IFF(pw_ctx *c);

/* IT.c  (upstream r/IT.c) */
void pw_rip_IT(pw_ctx *c);
int16_t pw_test_IT(pw_ctx *c);

/* JamCracker.c  (upstream r/JamCracker.c) */
void pw_rip_JamCracker(pw_ctx *c);
int16_t pw_test_JamCracker(pw_ctx *c);

/* KSM.c  (upstream r/KSM.c) */
void pw_depack_KSM(pw_ctx *c);
void pw_rip_KSM(pw_ctx *c);
int16_t pw_test_KSM(pw_ctx *c);

/* MastCrunch30-Add.c  (upstream r/MastCrunch30-Add.c) */
void pw_rip_MasterCruncher30addr(pw_ctx *c);
int16_t pw_test_MasterCruncher30addr(pw_ctx *c);

/* MaxPacker12.c  (upstream r/MaxPacker12.c) */
void pw_rip_MaxPacker12(pw_ctx *c);
int16_t pw_test_MaxPacker12(pw_ctx *c);

/* MED-Octamed.c  (upstream r/MED-Octamed.c) */
void pw_rip_MMD0(pw_ctx *c);
int16_t pw_test_MMD0(pw_ctx *c);

/* MegaCruncher-Obj.c  (upstream r/MegaCruncher-Obj.c) */
void pw_rip_MegaCruncherObj(pw_ctx *c);
int16_t pw_test_MegaCruncherObj(pw_ctx *c);

/* MegaCruncher.c  (upstream r/MegaCruncher.c) */
void pw_rip_MegaCruncher(pw_ctx *c);
int16_t pw_test_MegaCruncher10(pw_ctx *c);
int16_t pw_test_MegaCruncher12(pw_ctx *c);

/* MOD-compatible.c  (upstream r/MOD-compatible.c) */
void pw_rip_MOD(pw_ctx *c, int nbrchannels);
int16_t pw_test_MOD(pw_ctx *c, int arg);

/* ModuleProtector.c  (upstream r/ModuleProtector.c) */
void pw_depack_MP(pw_ctx *c);
void pw_rip_MP_noID(pw_ctx *c);
void pw_rip_MP_withID(pw_ctx *c);
int16_t pw_test_MP_noID(pw_ctx *c);
int16_t pw_test_MP_withID(pw_ctx *c);

/* mosh.c  (upstream r/mosh.c) */
void pw_depack_MOSH(pw_ctx *c);
void pw_rip_MOSH(pw_ctx *c);
int16_t pw_test_MOSH(pw_ctx *c);

/* Mugician.c  (upstream r/Mugician.c) */
void pw_rip_MUGICIAN(pw_ctx *c);
int16_t pw_test_MUGICIAN(pw_ctx *c);

/* Newtron.c  (upstream r/Newtron.c) */
void pw_depack_Newtron(pw_ctx *c);
void pw_rip_Newtron(pw_ctx *c);
int16_t pw_test_Newtron(pw_ctx *c);

/* NewtronOld.c  (upstream r/NewtronOld.c) */
void pw_depack_NewtronOld(pw_ctx *c);
void pw_rip_NewtronOld(pw_ctx *c);
int16_t pw_test_NewtronOld(pw_ctx *c);

/* NoiseFromHeaven.c  (upstream r/NoiseFromHeaven.c) */
void pw_depack_NFH(pw_ctx *c);
void pw_rip_NFH(pw_ctx *c);
int16_t pw_test_NFH(pw_ctx *c);

/* NoiseRunner.c  (upstream r/NoiseRunner.c) */
void pw_depack_Noiserunner(pw_ctx *c);
void pw_rip_Noiserunner(pw_ctx *c);
int16_t pw_test_Noiserunner(pw_ctx *c);

/* NovoTrade.c  (upstream r/NovoTrade.c) */
void pw_depack_NovoTrade(pw_ctx *c);
void pw_rip_NovoTrade(pw_ctx *c);
int16_t pw_test_NovoTrade(pw_ctx *c);

/* NP1.c  (upstream r/NP1.c) */
void pw_depack_Noisepacker1(pw_ctx *c);
void pw_rip_Noisepacker1(pw_ctx *c);
int16_t pw_test_Noisepacker1(pw_ctx *c);

/* NP2.c  (upstream r/NP2.c) */
void pw_depack_Noisepacker2(pw_ctx *c);
void pw_rip_Noisepacker2(pw_ctx *c);
int16_t pw_test_Noisepacker2(pw_ctx *c);

/* NP3.c  (upstream r/NP3.c) */
void pw_depack_Noisepacker3(pw_ctx *c);
void pw_rip_Noisepacker3(pw_ctx *c);
int16_t pw_test_Noisepacker3(pw_ctx *c);

/* okta.c  (upstream r/okta.c) */
void pw_rip_Okta(pw_ctx *c);
int16_t pw_test_Okta(pw_ctx *c);

/* Perfsong.c  (upstream r/Perfsong.c) */
void pw_depack_PERFSONG(pw_ctx *c);
void pw_rip_PERFSONG(pw_ctx *c);
int16_t pw_test_PERFSONG(pw_ctx *c);

/* PhaPacker.c  (upstream r/PhaPacker.c) */
void pw_depack_PHA(pw_ctx *c);
void pw_rip_PHA(pw_ctx *c);
int16_t pw_test_PHA(pw_ctx *c);

/* pmd3.c  (upstream r/pmd3.c) */
void pw_depack_PMD3(pw_ctx *c);
void pw_rip_PMD3(pw_ctx *c);
int16_t pw_test_PMD3(pw_ctx *c);

/* PolkaPacker.c  (upstream r/PolkaPacker.c) */
void pw_depack_Polka(pw_ctx *c);
void pw_rip_Polka(pw_ctx *c);
int16_t pw_test_Polka(pw_ctx *c);

/* PowerMusic.c  (upstream r/PowerMusic.c) */
void pw_depack_PM(pw_ctx *c);
void pw_rip_PM(pw_ctx *c);
int16_t pw_test_PM(pw_ctx *c);

/* PowerPacker23.c  (upstream r/PowerPacker23.c) */
void pw_rip_Powerpacker23(pw_ctx *c);
int16_t pw_test_Powerpacker23(pw_ctx *c);

/* PowerPacker30.c  (upstream r/PowerPacker30.c) */
void pw_rip_Powerpacker30(pw_ctx *c);
int16_t pw_test_Powerpacker30(pw_ctx *c);

/* PowerPacker40-lib.c  (upstream r/PowerPacker40-lib.c) */
void pw_rip_Powerpacker4lib(pw_ctx *c);
int16_t pw_test_Powerpacker4lib(pw_ctx *c);

/* PowerPacker40.c  (upstream r/PowerPacker40.c) */
void pw_rip_Powerpacker40(pw_ctx *c);
int16_t pw_test_Powerpacker40(pw_ctx *c);

/* Promizer01.c  (upstream r/Promizer01.c) */
void pw_depack_PM01(pw_ctx *c);
void pw_rip_PM01(pw_ctx *c);
int16_t pw_test_PM01(pw_ctx *c);

/* Promizer10c.c  (upstream r/Promizer10c.c) */
void pw_depack_PM10c(pw_ctx *c);
void pw_rip_PM10c(pw_ctx *c);
int16_t pw_test_PM10c(pw_ctx *c);

/* Promizer18a.c  (upstream r/Promizer18a.c) */
void pw_depack_PM18a(pw_ctx *c);
void pw_rip_PM18a(pw_ctx *c);
int16_t pw_test_PMZ(pw_ctx *c);

/* Promizer20.c  (upstream r/Promizer20.c) */
void pw_depack_PM20(pw_ctx *c);
void pw_rip_PM20(pw_ctx *c);
int16_t pw_test_PM2(pw_ctx *c);

/* Promizer40.c  (upstream r/Promizer40.c) */
void pw_depack_PM40(pw_ctx *c);
void pw_rip_PM40(pw_ctx *c);
int16_t pw_test_PM40(pw_ctx *c);

/* ProPacker10.c  (upstream r/ProPacker10.c) */
void pw_depack_PP10(pw_ctx *c);
void pw_rip_PP10(pw_ctx *c);
int16_t pw_test_PP10(pw_ctx *c);

/* ProPacker21.c  (upstream r/ProPacker21.c) */
void pw_depack_PP21(pw_ctx *c);
void pw_rip_PP21(pw_ctx *c);
int16_t pw_test_PP21(pw_ctx *c);

/* ProPacker30.c  (upstream r/ProPacker30.c) */
void pw_depack_PP30(pw_ctx *c);
void pw_rip_PP30(pw_ctx *c);
int16_t pw_test_PP30(pw_ctx *c);

/* Prorunner10.c  (upstream r/Prorunner10.c) */
void pw_depack_PRUN1(pw_ctx *c);
void pw_rip_PRUN1(pw_ctx *c);
int16_t pw_test_PRUN1(pw_ctx *c);

/* Prorunner20.c  (upstream r/Prorunner20.c) */
void pw_depack_PRUN2(pw_ctx *c);
void pw_rip_PRUN2(pw_ctx *c);
int16_t pw_test_PRUN2(pw_ctx *c);

/* QC.c  (upstream r/QC.c) */
void pw_depack_QuadraComposer(pw_ctx *c);
void pw_rip_QuadraComposer(pw_ctx *c);
int16_t pw_test_QuadraComposer(pw_ctx *c);

/* RelokIt10.c  (upstream r/RelokIt10.c) */
void pw_rip_RelokIt10(pw_ctx *c);
int16_t pw_test_RelokIt10(pw_ctx *c);

/* S3M.c  (upstream r/S3M.c) */
void pw_rip_S3M(pw_ctx *c);
int16_t pw_test_S3M(pw_ctx *c);

/* SA.c  (upstream r/SA.c) */
void pw_rip_SA(pw_ctx *c);
int16_t pw_test_SA(pw_ctx *c);
int16_t pw_test_SAhunk(pw_ctx *c);

/* sidmon1.c  (upstream r/sidmon1.c) */
void pw_rip_SIDMON1(pw_ctx *c);
int16_t pw_test_SIDMON1(pw_ctx *c);

/* skizzo.c  (upstream r/skizzo.c) */
void pw_depack_Skizzo(pw_ctx *c);
void pw_rip_Skizzo(pw_ctx *c);
int16_t pw_test_Skizzo(pw_ctx *c);

/* SkytPacker.c  (upstream r/SkytPacker.c) */
void pw_depack_SKYT(pw_ctx *c);
void pw_rip_SKYT(pw_ctx *c);
int16_t pw_test_SKYT(pw_ctx *c);

/* SoundFX.c  (upstream r/SoundFX.c) */
void pw_depack_SoundFX13(pw_ctx *c);
void pw_rip_SoundFX13(pw_ctx *c);
int16_t pw_test_SoundFX13(pw_ctx *c);

/* SoundTracker.c  (upstream r/SoundTracker.c) */
void pw_rip_SoundTracker(pw_ctx *c);
int16_t pw_test_SoundTracker(pw_ctx *c);

/* SoundTracker26.c  (upstream r/SoundTracker26.c) */
void pw_depack_STK26(pw_ctx *c);
void pw_rip_STK26(pw_ctx *c);
int16_t pw_test_STK26(pw_ctx *c);

/* SpikeCruncher.c  (upstream r/SpikeCruncher.c) */
void pw_rip_SpikeCruncher(pw_ctx *c);
int16_t pw_test_SpikeCruncher(pw_ctx *c);

/* StartrekkerPack.c  (upstream r/StartrekkerPack.c) */
void pw_depack_STARPACK(pw_ctx *c);
void pw_rip_STARPACK(pw_ctx *c);
int16_t pw_test_STARPACK(pw_ctx *c);

/* StimPacker.c  (upstream r/StimPacker.c) */
void pw_depack_STIM(pw_ctx *c);
void pw_rip_STIM(pw_ctx *c);
int16_t pw_test_STIM(pw_ctx *c);

/* StoneArtsPlayer.c  (upstream r/StoneArtsPlayer.c) */
void pw_depack_StoneArtsPlayer(pw_ctx *c);
void pw_rip_StoneArtsPlayer(pw_ctx *c);
int16_t pw_test_StoneArtsPlayer(pw_ctx *c);

/* StoneCracker270.c  (upstream r/StoneCracker270.c) */
void pw_rip_StoneCracker270(pw_ctx *c);
int16_t pw_test_StoneCracker270(pw_ctx *c);

/* StoneCracker299.c  (upstream r/StoneCracker299.c) */
void pw_rip_STC299(pw_ctx *c);
int16_t pw_test_STC299(pw_ctx *c);

/* StoneCracker299b.c  (upstream r/StoneCracker299b.c) */
void pw_rip_STC299b(pw_ctx *c);
int16_t pw_test_STC299b(pw_ctx *c);

/* StoneCracker299d.c  (upstream r/StoneCracker299d.c) */
void pw_rip_STC299d(pw_ctx *c);
int16_t pw_test_STC299d(pw_ctx *c);

/* StoneCracker300.c  (upstream r/StoneCracker300.c) */
void pw_rip_STC300(pw_ctx *c);
int16_t pw_test_STC300(pw_ctx *c);

/* StoneCracker310.c  (upstream r/StoneCracker310.c) */
void pw_rip_STC310(pw_ctx *c);
int16_t pw_test_STC310(pw_ctx *c);

/* SuperCruncher27.c  (upstream r/SuperCruncher27.c) */
void pw_rip_SuperCruncher27(pw_ctx *c);
int16_t pw_test_SuperCruncher27(pw_ctx *c);

/* SyncroPacker46.c  (upstream r/SyncroPacker46.c) */
void pw_rip_SyncroPacker(pw_ctx *c);
int16_t pw_test_SyncroPacker(pw_ctx *c);

/* TetraPack1.c  (upstream r/TetraPack1.c) */
void pw_rip_Tetrapack101(pw_ctx *c);
void pw_rip_Tetrapack102(pw_ctx *c);
int16_t pw_test_Tetrapack101(pw_ctx *c);
int16_t pw_test_Tetrapack102(pw_ctx *c);

/* TetraPack21.c  (upstream r/TetraPack21.c) */
void pw_rip_Tetrapack_2_1(pw_ctx *c);
int16_t pw_test_Tetrapack_2_1(pw_ctx *c);

/* TetraPack22.c  (upstream r/TetraPack22.c) */
void pw_rip_Tetrapack_2_2(pw_ctx *c);
int16_t pw_test_Tetrapack_2_2(pw_ctx *c);

/* TheDarkDemon.c  (upstream r/TheDarkDemon.c) */
void pw_depack_TheDarkDemon(pw_ctx *c);
void pw_rip_TheDarkDemon(pw_ctx *c);
int16_t pw_test_TheDarkDemon(pw_ctx *c);

/* THX.c  (upstream r/THX.c) */
void pw_rip_THX(pw_ctx *c);
int16_t pw_test_THX(pw_ctx *c);

/* TimeCruncher17.c  (upstream r/TimeCruncher17.c) */
void pw_rip_TimeCruncher17(pw_ctx *c);
int16_t pw_test_TimeCruncher17(pw_ctx *c);

/* TitanicsPlayer.c  (upstream r/TitanicsPlayer.c) */
void pw_depack_TitanicsPlayer(pw_ctx *c);
void pw_rip_TitanicsPlayer(pw_ctx *c);
int16_t pw_test_TitanicsPlayer(pw_ctx *c);

/* TMK.c  (upstream r/TMK.c) */
void pw_depack_TMK(pw_ctx *c);
void pw_rip_TMK(pw_ctx *c);
int16_t pw_test_TMK(pw_ctx *c);

/* TNMCruncher.c  (upstream r/TNMCruncher.c) */
void pw_rip_TNMCruncher11(pw_ctx *c);
int16_t pw_test_TNMCruncher11(pw_ctx *c);

/* TP22a.c  (upstream r/TP22a.c) */
void pw_depack_P22(pw_ctx *c);
void pw_rip_P22A(pw_ctx *c);

/* TP30a.c  (upstream r/TP30a.c) */
void pw_depack_P30(pw_ctx *c);
void pw_rip_P30A(pw_ctx *c);

/* TP40.c  (upstream r/TP40.c) */
void pw_depack_P40(pw_ctx *c);
void pw_rip_P40A(pw_ctx *c);
void pw_rip_P40B(pw_ctx *c);
int16_t pw_test_P40A(pw_ctx *c);

/* TP41a.c  (upstream r/TP41a.c) */
void pw_depack_P41A(pw_ctx *c);
void pw_rip_P41A(pw_ctx *c);
int16_t pw_test_P41A(pw_ctx *c);

/* TP50a.c  (upstream r/TP50a.c) */
void pw_depack_P50A(pw_ctx *c);
void pw_rip_P50A(pw_ctx *c);
int16_t pw_test_P50A(pw_ctx *c);

/* TP60a.c  (upstream r/TP60a.c) */
void pw_depack_P60A(pw_ctx *c);
void pw_rip_P60A(pw_ctx *c);
int16_t pw_test_P60A_nopack(pw_ctx *c);
int16_t pw_test_P60A_pack(pw_ctx *c);

/* TP61a.c  (upstream r/TP61a.c) */
void pw_depack_P61A(pw_ctx *c);
void pw_rip_P61A(pw_ctx *c);
int16_t pw_test_P61A_nopack(pw_ctx *c);
int16_t pw_test_P61A_pack(pw_ctx *c);

/* TrackerPacker1.c  (upstream r/TrackerPacker1.c) */
void pw_depack_TP1(pw_ctx *c);
void pw_rip_TP1(pw_ctx *c);
int16_t pw_test_TP1(pw_ctx *c);

/* TrackerPacker2.c  (upstream r/TrackerPacker2.c) */
void pw_depack_TP2(pw_ctx *c);
void pw_rip_TP2(pw_ctx *c);
int16_t pw_test_TP2(pw_ctx *c);

/* TrackerPacker3.c  (upstream r/TrackerPacker3.c) */
void pw_depack_TP3(pw_ctx *c);
void pw_rip_TP3(pw_ctx *c);
int16_t pw_test_TP3(pw_ctx *c);

/* TryItCruncher101.c  (upstream r/TryItCruncher101.c) */
void pw_rip_TryIt101(pw_ctx *c);
int16_t pw_test_TryIt101(pw_ctx *c);

/* TurboSqueezer61.c  (upstream r/TurboSqueezer61.c) */
void pw_rip_TurboSqueezer61(pw_ctx *c);
int16_t pw_test_TurboSqueezer61(pw_ctx *c);

/* UnicTracker.c  (upstream r/UnicTracker.c) */
void pw_depack_UNIC(pw_ctx *c);
void pw_rip_UNIC_noID(pw_ctx *c);
void pw_rip_UNIC_withID(pw_ctx *c);
int16_t pw_test_UNIC_noID(pw_ctx *c);
int16_t pw_test_UNIC_withID(pw_ctx *c);
int16_t pw_test_UNIC_withemptyID(pw_ctx *c);

/* UnicTracker2.c  (upstream r/UnicTracker2.c) */
void pw_depack_UNIC2(pw_ctx *c);
void pw_rip_UNIC2(pw_ctx *c);
int16_t pw_test_UNIC2(pw_ctx *c);

/* WantonPacker.c  (upstream r/WantonPacker.c) */
void pw_depack_WN(pw_ctx *c);
void pw_rip_WN(pw_ctx *c);
int16_t pw_test_WN(pw_ctx *c);

/* XannPlayer.c  (upstream r/XannPlayer.c) */
void pw_depack_XANN(pw_ctx *c);
void pw_rip_XANN(pw_ctx *c);
int16_t pw_test_XANN(pw_ctx *c);

/* ZenPacker.c  (upstream r/ZenPacker.c) */
void pw_depack_ZEN(pw_ctx *c);
void pw_rip_ZEN(pw_ctx *c);
int16_t pw_test_ZEN(pw_ctx *c);

#endif
