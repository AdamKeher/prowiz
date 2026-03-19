# Pro-Wizard

Version: v1.70b based on v1.70a

Pro-Wizard is a tool designed to identify, rip, and depack various Amiga music module formats and data-crunched files. Originally based on the famous Pro-Wizard 2 for Amiga, this version brings its capabilities to modern systems, allowing for the recovery of music from packed executables and data blocks.

**Note:** This is a modified version (demoscene.au) tailored for personal use. It introduces command-line flags for better control and a machine-readable JSON output mode for integration with other tools.

## Command Line Help

```text
Usage: prowiz <flag> <input file> <output file>

Flags:
  -r   : Rip Mode (Identify and depack modules to .mod)
  -d   : Data Mode (Identify and rebuild executables for data crunchers)
  -rd  : Both modes
  -j   : JSON mode (clean stdout, machine readable JSON)
```

## Credits

Pro-Wizard is the result of contributions from many individuals in the demoscene and Amiga community:

*   **Sylvain "asle" Chipaux**: Primary author and developer.
*   **Nicolas Franck (Gryzor)**: Author of the original Pro-Wizard 2 and provider of numerous packed files.
*   **Mickael Doering**: Linux port and constructive feedback.
*   **Philippe Anel (Xigh)**: Mingwin32/Windows port and technical tips.
*   **Thomas Neumann**: Extensive bug hunting and reporting.
*   **Markus Jaegermeister**: Bug reports and information on "exotic" formats.
*   **Laurent Clévy**: Descriptions for numerous synth formats and C coding tips.
*   **Michael Trier, XtC**: Provided various files for testing and development.
*   **Slight**: Early project supporter.
*   **Adam Keher**: Quick hack / mangling / slop for flags, JSON output, and output file.

## Description

Pro-Wizard specializes in "ripping" music modules that have been packed or embedded within other files (like intros, games, or cracktros). It can recognize over 60 different music formats and nearly 60 data/executable cruncher formats.

### Key Modifications
*   **Option Flags**: Modernized CLI interface with flags (`-r`, `-d`, `-rd`) to specify processing modes.
*   **JSON Output**: A new `-j` flag allows the tool to output clean, machine-readable JSON to `stdout`, making it easier to use Pro-Wizard as a backend for other applications.

In **Rip Mode (`-r`)**, it attempts to identify music modules and convert them back to a standard ProTracker-compatible `.mod` format where possible.
In **Data Mode (`-d`)**, it identifies data blocks packed by various Amiga crunchers and can help rebuild them.

## Supported Music Formats

Pro-Wizard detects and handles a wide variety of Amiga music formats:

*   **Tracker Formats**
    *   ProTracker (MOD)
    *   SoundTracker
    *   StarTrekker
    *   NoisePacker (v1, v2, v3)
    *   Tracker Packer (v1, v2, v3)
    *   ProPacker (v1.0, v2.1, v3.0)
    *   ProRunner (v1, v2)
    *   Promizer (0.1, 1.0c, 1.8a, 2.0, 4.0)
    *   UNIC Tracker (v1, v2)
*   **Player Formats**
    *   The Player (2.2A, 3.3A, 4.0A, 4.0B, 4.1A, 5.0A, 6.0A, 6.1A)
    *   Game Music Creator (GMC)
    *   Fuchs Tracker
    *   Kefrens Sound Machine (KSM)
    *   MED (MMD0/MMD1)
    *   Sound FX
    *   Quadra Composer
*   **Packers & Exotic**
    *   AC1D packer
    *   Heatseeker mc1.0
    *   Digital Illusion
    *   Eureka packer
    *   FC-M packer
    *   Fuzzac packer
    *   GnuPlayer
    *   GPMO (Crunch Player)
    *   Hornet packer
    *   Kris Tracker
    *   Module protector
    *   Newtron packer
    *   Noiserunner
    *   PHA Packer
    *   Polka packer
    *   SKYT packer
    *   STIM (Slamtilt)
    *   The Dark Demon
    *   Wanton Packer
    *   Xann Packer
    *   Zen Packer
*   **Synthetic Formats**
    *   Sound Monitor (v2/v3)
    *   BSI Future Composer
    *   Future Composer (1.3, 1.4)
    *   JamCracker / Pro

## Supported Depackers / Data Crunchers

Pro-Wizard can identify and process files crunched by the following tools:

*   **Executable Crunchers**
    *   ByteKiller (1.3, 2.0, 3.0, Pro 1.0)
    *   Crunchmania
    *   Double Action 1.0
    *   Defjam Cruncher 3.2
    *   HQC Cruncher 2.0
    *   Max Packer 1.2
    *   Master Cruncher 3.0
    *   Mega Cruncher (1.0, 1.2, Obj)
    *   PowerPacker (2.3, 3.0, 4.0)
    *   RelokIt 1.0
    *   Spike Cruncher
    *   StoneCracker (2.70, 2.99, 3.00, 3.10)
    *   Syncro Packer 4.6
    *   Super Cruncher 2.7
    *   Tetrapack (1.02, 2.1, 2.2)
    *   Try-It Cruncher 1.01
    *   Time Cruncher 1.7
    *   TNM Cruncher 1.1
    *   TurboCruncher 6.1
*   **Data Crunchers**
    *   Amnesty Design Data Packer
    *   $08090A08 Data Cruncher
    *   arcD
    *   Crunchmania Data
    *   CRND
    *   FIRE (RNC clone)
    *   Ice!
    *   Imploder (ATN!, IMP!, CHFI)
    *   PARA
    *   AMOS Powerpacker Bank "PPbk"
    *   Propack (Rob Northern)
    *   =SB=
    *   SF
    *   Virtual Dreams (VDCO)
    *   XPK

---
*Note: Some formats may have limited support for full depacking depending on the complexity of the original compression.*
