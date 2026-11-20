/* format_table.c - built-in extensions and kinds (generated from Pro-Wizard 1.70a misc.c). */
#include "pw_internal.h"

/* Support_Types_FileDefault(): one entry per format plus the trailing "---". */
const char *const pw_default_ext[PWF_COUNT + 1] =
{
    "ac1d",                  /*   0 AC1D_packer */
    "bp",                    /*   1 SoundMonitor */
    "fc-m",                  /*   2 FC_M_packer */
    "hrt",                   /*   3 Hornet_packer */
    "kris",                  /*   4 KRIS_tracker */
    "PowerMusic",            /*   5 Power_Music */
    "Promizer10c",           /*   6 Promizer_10c */
    "Promizer18a",           /*   7 Promizer_18a */
    "Promizer20",            /*   8 Promizer_20 */
    "ProRunner1",            /*   9 ProRunner_v1 */
    "ProRunner2",            /*  10 ProRunner_v2 */
    "skyt",                  /*  11 SKYT_packer */
    "WantonPacker",          /*  12 Wanton_packer */
    "xann",                  /*  13 XANN_packer */
    "ModuleProtector",       /*  14 Module_protector */
    "DigitalIllusion",       /*  15 Digital_illusion */
    "PhaPacker",             /*  16 Pha_packer */
    "Promizer01",            /*  17 Promizer_01 */
    "ProPacker21",           /*  18 Propacker_21 */
    "ProPacker30",           /*  19 Propacker_30 */
    "Eureka",                /*  20 Eureka_packer */
    "StarTrekkerPack",       /*  21 Star_pack */
    "mod",                   /*  22 Protracker */
    "unic1",                 /*  23 UNIC_v1 */
    "unic2",                 /*  24 UNIC_v2 */
    "Fuzzac",                /*  25 Fuzzac */
    "gmc",                   /*  26 GMC */
    "crb",                   /*  27 Heatseeker */
    "ksm",                   /*  28 KSM */
    "Noiserunner",           /*  29 Noiserunner */
    "NoisePacker1",          /*  30 Noisepacker1 */
    "NoisePacker2",          /*  31 Noisepacker2 */
    "NoisePacker3",          /*  32 Noisepacker3 */
    "P40A",                  /*  33 P40A */
    "P40B",                  /*  34 P40B */
    "P41A",                  /*  35 P41A */
    "Promizer4",             /*  36 PM40 */
    "ProPacker1",            /*  37 PP10 */
    "TrackerPacker1",        /*  38 TP1 */
    "TrackerPacker2",        /*  39 TP2 */
    "TrackerPacker3",        /*  40 TP3 */
    "ZenPacker",             /*  41 ZEN */
    "P50A",                  /*  42 P50A */
    "P60A",                  /*  43 P60A */
    "mod",                   /*  44 StarTrekker */
    "StoneCrackerData",      /*  45 S404 */
    "StoneCracker270 ",      /*  46 StoneCracker270 */
    "P61A",                  /*  47 P61A */
    "stim",                  /*  48 STIM */
    "mod",                   /*  49 SoundTracker */
    "TetraPack22",           /*  50 TPACK22 */
    "CrunchmaniaData",       /*  51 CRM1 */
    "DefjamCruncher",        /*  52 Defjam_32 */
    "Tetrapack21",           /*  53 TPACK21 */
    "ice",                   /*  54 ICE */
    "ByteKiller",            /*  55 ByteKiller */
    "xpk",                   /*  56 XPK */
    "Imploder",              /*  57 IMP */
    "rnc",                   /*  58 RNC */
    "DoubleAction",          /*  59 Double_Action */
    "PowerPacker3",          /*  60 Powerpacker3 */
    "PowerPacker4",          /*  61 Powerpacker4 */
    "PowerPacker23",         /*  62 Powerpacker23 */
    "SpikeCruncher",         /*  63 SpikeCruncher */
    "Tetrapack102",          /*  64 TPACK102 */
    "TimeCruncher17",        /*  65 TimeCruncher */
    "MasterCruncher",        /*  66 MasterCruncher */
    "MegaCruncher",          /*  67 MegaCruncher */
    "jam",                   /*  68 JamCracker */
    "BSI-FC",                /*  69 BSIFC */
    "digi",                  /*  70 DigiBooster */
    "qc",                    /*  71 QuadraComposer */
    "TheDarkDemon",          /*  72 TDD */
    "FuchsTracker",          /*  73 FuchsTracker */
    "SynchroPacker46",       /*  74 SyncroPacker */
    "TNMCruncher11",         /*  75 TNMCruncher */
    "SuperCruncher27",       /*  76 SuperCruncher */
    "PPbk",                  /*  77 PP20 */
    "RelokIt1",              /*  78 RelokIt */
    "StoneCracker292data",   /*  79 STC292data */
    "fire",                  /*  80 FIRE */
    "MacPacker12",           /*  81 MaxPacker */
    "SoundFX13",             /*  82 SoundFX */
    "arcD",                  /*  83 arcD */
    "para",                  /*  84 PARA */
    "crnd",                  /*  85 CRND */
    "-sb-",                  /*  86 SB_DataCruncher */
    "sf",                    /*  87 SF */
    "RLE",                   /*  88 RLE */
    "VDC0",                  /*  89 VDCO */
    "sq",                    /*  90 SQ */
    "sp",                    /*  91 SP */
    "ST26",                  /*  92 STK26 */
    "IT10",                  /*  93 IceTracker */
    "HQCCruncher2",          /*  94 HQC */
    "TtyItCruncher101",      /*  95 TryIt */
    "FC13",                  /*  96 FC13 */
    "FC14",                  /*  97 FC14 */
    "1AM",                   /*  98 AmnestyDesign1 */
    "2AM",                   /*  99 AmnestyDesign2 */
    "med",                   /* 100 MED */
    "AceCruncherData",       /* 101 ACECruncherData */
    "Newtron",               /* 102 Newtron */
    "GPMO",                  /* 103 GPMO */
    "PolkaPacker",           /* 104 PolkaPacker */
    "GnuPlayer",             /* 105 GnuPlayer */
    "CJ_DataCruncher",       /* 106 CJ_DataCruncher */
    "AmBk",                  /* 107 AmBk */
    "MasterCruncher3data",   /* 108 MasterCruncher3data */
    "xm",                    /* 109 XM */
    "MegaCruncherObj",       /* 110 MegaCruncherObj */
    "TurboSqueezer61",       /* 111 TurboSqueezer61 */
    "StoneCracker299d",      /* 112 STC299d */
    "StoneCracker310",       /* 113 STC310 */
    "StoneCracker299b",      /* 114 STC299b */
    "StoneCracker299",       /* 115 STC299 */
    "StoneCracker300",       /* 116 STC300 */
    "ThePlayer30a",          /* 117 ThePlayer30a */
    "ThePlayer22a",          /* 118 ThePlayer22a */
    "NoiseFromHeaven",       /* 119 NoiseFromHeaven */
    "TMK",                   /* 120 TMK */
    "DragPack252",           /* 121 DragPack252 */
    "DragPack100",           /* 122 DragPack100 */
    "SPv3",                  /* 123 SpeedPacker3Data */
    "AtomikPackerData",      /* 124 AtomikPackerData */
    "AutomationPackerData",  /* 125 AutomationPackerData */
    "SGTPacker",             /* 126 SGTPacker */
    "GNUPacker12",           /* 127 GNUPacker12 */
    "CrunchmaniaSimple",     /* 128 CrunchmaniaSimple */
    "dmu",                   /* 129 dmu */
    "TitanicsPlayer",        /* 130 TitanicsPlayer */
    "NewtronOld",            /* 131 NewtronOld */
    "NovoTrade",             /* 132 NovoTrade */
    "GnoiPacker",            /* 133 GnoiPacker */
    "StoneArtsPlayer",       /* 134 StoneArtsPlayer */
    "SLAM",                  /* 135 SLAM */
    "S3M",                   /* 136 S3M */
    "MOSH",                  /* 137 MOSH */
    "BinaryPacker",          /* 138 BNR */
    "HCD",                   /* 139 HCD */
    "Tetrapack101",          /* 140 TPACK101 */
    "StruggleGame",          /* 141 STRUGGLE */
    "BKCloneFLT",            /* 142 BKCloneFLT */
    "Oktalizer",             /* 143 Oktalizer */
    "AHX",                   /* 144 AHX */
    "Sidmon1",               /* 145 Sidmon1 */
    "Sidmon2",               /* 146 Sidmon2 */
    "PerfSong",              /* 147 PerfSong */
    "MASMDataCruncher",      /* 148 MASMDataCruncher */
    "ImpulseTracker",        /* 149 ImpulseTracker */
    "HighPresCruncher",      /* 150 HighPresCruncher */
    "SP20",                  /* 151 SP20 */
    "B9AB",                  /* 152 B9AB */
    "HVL",                   /* 153 HVL */
    "BKClone5",              /* 154 BKClone5 */
    "xVdg",                  /* 155 xVdg */
    "DAT",                   /* 156 Diet */
    "LSDDataCruncher",       /* 157 LSDDataCruncher */
    "JamDataCruncher",       /* 158 JamDataCruncher */
    "MI10",                  /* 159 MentalImage */
    "BHC3CruncherData",      /* 160 BHC3CruncherData */
    "IFF",                   /* 161 IFF */
    "SA",                    /* 162 SA */
    "DM1",                   /* 163 DM1 */
    "PMd3",                  /* 164 PMd3 */
    "PMD3",                  /* 165 PMD3 */
    "BHC2CruncherData",      /* 166 BHC2CruncherData */
    "Pac1",                  /* 167 Pac1 */
    "---",                   /* 168 (_KNOWN_FORMATS) */
};

const unsigned char pw_format_kinds[PWF_COUNT] =
{
    PW_KIND_MUSIC, /* AC1D_packer */
    PW_KIND_MUSIC, /* SoundMonitor */
    PW_KIND_MUSIC, /* FC_M_packer */
    PW_KIND_MUSIC, /* Hornet_packer */
    PW_KIND_MUSIC, /* KRIS_tracker */
    PW_KIND_MUSIC, /* Power_Music */
    PW_KIND_MUSIC, /* Promizer_10c */
    PW_KIND_MUSIC, /* Promizer_18a */
    PW_KIND_MUSIC, /* Promizer_20 */
    PW_KIND_MUSIC, /* ProRunner_v1 */
    PW_KIND_MUSIC, /* ProRunner_v2 */
    PW_KIND_MUSIC, /* SKYT_packer */
    PW_KIND_MUSIC, /* Wanton_packer */
    PW_KIND_MUSIC, /* XANN_packer */
    PW_KIND_MUSIC, /* Module_protector */
    PW_KIND_MUSIC, /* Digital_illusion */
    PW_KIND_MUSIC, /* Pha_packer */
    PW_KIND_MUSIC, /* Promizer_01 */
    PW_KIND_MUSIC, /* Propacker_21 */
    PW_KIND_MUSIC, /* Propacker_30 */
    PW_KIND_MUSIC, /* Eureka_packer */
    PW_KIND_MUSIC, /* Star_pack */
    PW_KIND_MUSIC, /* Protracker */
    PW_KIND_MUSIC, /* UNIC_v1 */
    PW_KIND_MUSIC, /* UNIC_v2 */
    PW_KIND_MUSIC, /* Fuzzac */
    PW_KIND_MUSIC, /* GMC */
    PW_KIND_MUSIC, /* Heatseeker */
    PW_KIND_MUSIC, /* KSM */
    PW_KIND_MUSIC, /* Noiserunner */
    PW_KIND_MUSIC, /* Noisepacker1 */
    PW_KIND_MUSIC, /* Noisepacker2 */
    PW_KIND_MUSIC, /* Noisepacker3 */
    PW_KIND_MUSIC, /* P40A */
    PW_KIND_MUSIC, /* P40B */
    PW_KIND_MUSIC, /* P41A */
    PW_KIND_MUSIC, /* PM40 */
    PW_KIND_MUSIC, /* PP10 */
    PW_KIND_MUSIC, /* TP1 */
    PW_KIND_MUSIC, /* TP2 */
    PW_KIND_MUSIC, /* TP3 */
    PW_KIND_MUSIC, /* ZEN */
    PW_KIND_MUSIC, /* P50A */
    PW_KIND_MUSIC, /* P60A */
    PW_KIND_MUSIC, /* StarTrekker */
    PW_KIND_DATA , /* S404 */
    PW_KIND_DATA , /* StoneCracker270 */
    PW_KIND_MUSIC, /* P61A */
    PW_KIND_MUSIC, /* STIM */
    PW_KIND_MUSIC, /* SoundTracker */
    PW_KIND_DATA , /* TPACK22 */
    PW_KIND_DATA , /* CRM1 */
    PW_KIND_DATA , /* Defjam_32 */
    PW_KIND_DATA , /* TPACK21 */
    PW_KIND_DATA , /* ICE */
    PW_KIND_DATA , /* ByteKiller */
    PW_KIND_DATA , /* XPK */
    PW_KIND_DATA , /* IMP */
    PW_KIND_DATA , /* RNC */
    PW_KIND_DATA , /* Double_Action */
    PW_KIND_DATA , /* Powerpacker3 */
    PW_KIND_DATA , /* Powerpacker4 */
    PW_KIND_DATA , /* Powerpacker23 */
    PW_KIND_DATA , /* SpikeCruncher */
    PW_KIND_DATA , /* TPACK102 */
    PW_KIND_DATA , /* TimeCruncher */
    PW_KIND_DATA , /* MasterCruncher */
    PW_KIND_DATA , /* MegaCruncher */
    PW_KIND_MUSIC, /* JamCracker */
    PW_KIND_MUSIC, /* BSIFC */
    PW_KIND_MUSIC, /* DigiBooster */
    PW_KIND_MUSIC, /* QuadraComposer */
    PW_KIND_MUSIC, /* TDD */
    PW_KIND_MUSIC, /* FuchsTracker */
    PW_KIND_DATA , /* SyncroPacker */
    PW_KIND_DATA , /* TNMCruncher */
    PW_KIND_DATA , /* SuperCruncher */
    PW_KIND_DATA , /* PP20 */
    PW_KIND_DATA , /* RelokIt */
    PW_KIND_DATA , /* STC292data */
    PW_KIND_DATA , /* FIRE */
    PW_KIND_DATA , /* MaxPacker */
    PW_KIND_MUSIC, /* SoundFX */
    PW_KIND_DATA , /* arcD */
    PW_KIND_DATA , /* PARA */
    PW_KIND_DATA , /* CRND */
    PW_KIND_DATA , /* SB_DataCruncher */
    PW_KIND_DATA , /* SF */
    PW_KIND_DATA , /* RLE */
    PW_KIND_DATA , /* VDCO */
    PW_KIND_DATA , /* SQ */
    PW_KIND_DATA , /* SP */
    PW_KIND_MUSIC, /* STK26 */
    PW_KIND_MUSIC, /* IceTracker */
    PW_KIND_DATA , /* HQC */
    PW_KIND_DATA , /* TryIt */
    PW_KIND_MUSIC, /* FC13 */
    PW_KIND_MUSIC, /* FC14 */
    PW_KIND_DATA , /* AmnestyDesign1 */
    PW_KIND_DATA , /* AmnestyDesign2 */
    PW_KIND_MUSIC, /* MED */
    PW_KIND_DATA , /* ACECruncherData */
    PW_KIND_MUSIC, /* Newtron */
    PW_KIND_MUSIC, /* GPMO */
    PW_KIND_MUSIC, /* PolkaPacker */
    PW_KIND_MUSIC, /* GnuPlayer */
    PW_KIND_DATA , /* CJ_DataCruncher */
    PW_KIND_MUSIC, /* AmBk */
    PW_KIND_DATA , /* MasterCruncher3data */
    PW_KIND_MUSIC, /* XM */
    PW_KIND_DATA , /* MegaCruncherObj */
    PW_KIND_DATA , /* TurboSqueezer61 */
    PW_KIND_DATA , /* STC299d */
    PW_KIND_DATA , /* STC310 */
    PW_KIND_DATA , /* STC299b */
    PW_KIND_DATA , /* STC299 */
    PW_KIND_DATA , /* STC300 */
    PW_KIND_MUSIC, /* ThePlayer30a */
    PW_KIND_MUSIC, /* ThePlayer22a */
    PW_KIND_MUSIC, /* NoiseFromHeaven */
    PW_KIND_MUSIC, /* TMK */
    PW_KIND_DATA , /* DragPack252 */
    PW_KIND_DATA , /* DragPack100 */
    PW_KIND_DATA , /* SpeedPacker3Data */
    PW_KIND_DATA , /* AtomikPackerData */
    PW_KIND_DATA , /* AutomationPackerData */
    PW_KIND_MUSIC, /* SGTPacker */
    PW_KIND_DATA , /* GNUPacker12 */
    PW_KIND_DATA , /* CrunchmaniaSimple */
    PW_KIND_MUSIC, /* dmu */
    PW_KIND_MUSIC, /* TitanicsPlayer */
    PW_KIND_MUSIC, /* NewtronOld */
    PW_KIND_MUSIC, /* NovoTrade */
    PW_KIND_MUSIC, /* GnoiPacker */
    PW_KIND_MUSIC, /* StoneArtsPlayer */
    PW_KIND_MUSIC, /* SLAM */
    PW_KIND_MUSIC, /* S3M */
    PW_KIND_MUSIC, /* MOSH */
    PW_KIND_MUSIC, /* BNR */
    PW_KIND_MUSIC, /* HCD */
    PW_KIND_DATA , /* TPACK101 */
    PW_KIND_MUSIC, /* STRUGGLE */
    PW_KIND_DATA , /* BKCloneFLT */
    PW_KIND_MUSIC, /* Oktalizer */
    PW_KIND_MUSIC, /* AHX */
    PW_KIND_MUSIC, /* Sidmon1 */
    PW_KIND_MUSIC, /* Sidmon2 */
    PW_KIND_MUSIC, /* PerfSong */
    PW_KIND_DATA , /* MASMDataCruncher */
    PW_KIND_MUSIC, /* ImpulseTracker */
    PW_KIND_DATA , /* HighPresCruncher */
    PW_KIND_DATA , /* SP20 */
    PW_KIND_DATA , /* B9AB */
    PW_KIND_MUSIC, /* HVL */
    PW_KIND_DATA , /* BKClone5 */
    PW_KIND_DATA , /* xVdg */
    PW_KIND_DATA , /* Diet */
    PW_KIND_DATA , /* LSDDataCruncher */
    PW_KIND_DATA , /* JamDataCruncher */
    PW_KIND_DATA , /* MentalImage */
    PW_KIND_DATA , /* BHC3CruncherData */
    PW_KIND_MUSIC, /* IFF */
    PW_KIND_MUSIC, /* SA */
    PW_KIND_MUSIC, /* DM1 */
    PW_KIND_MUSIC, /* PMd3 */
    PW_KIND_MUSIC, /* PMD3 */
    PW_KIND_DATA , /* BHC2CruncherData */
    PW_KIND_DATA , /* Pac1 */
};
