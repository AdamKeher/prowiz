# Pro-Wizard

A portable, re-entrant C99 port of **Pro-Wizard for PC 1.70a** by Sylvain
"Asle" Chipaux: the Amiga music ripper that finds modules, packed modules and
crunched executables or data inside any binary (disk images, memory dumps,
intros, games), saves each one, and converts packed modules back to
Protracker.

With `-c` the port prints the same text and writes the same files as the
original Windows build, byte for byte, for every input. It is tested that
way: see [Accuracy](#accuracy). By default it also finds formats the original
never knew, such as PreTracker, LightSpeedPlayer, TFMX, OctaMED MMD2/MMD3 and
SidMon 2 (see [Formats](#formats)). Underneath it is a library with no global
state, bounds checked memory access and a callback API, plus a command line
tool with a few optional extras (mode selection, an output directory, a
scan-only listing and JSON output).

## Downloads

Ready-made command line binaries are on the
[releases page](https://github.com/AdamKeher/prowiz/releases): Windows
(x86-64), Linux (x86-64 and arm64, statically linked) and macOS (universal,
11 or later). The macOS binary is not notarised; if macOS refuses to open it,
clear the download flag with `xattr -d com.apple.quarantine prowiz`.

## Building

Any C99 compiler. GNU make:

```sh
make            # build/prowiz (CLI) and build/libprowiz.a
make test       # unit tests
```

or CMake (also for MSVC):

```sh
cmake -S . -B build && cmake --build build --config Release
ctest --test-dir build -C Release
```

Tested with MinGW-w64 gcc 16 (Windows) and gcc 13 (Linux).

## Using it

```text
prowiz [options] <input file>

  -c          only the formats of Pro-Wizard 1.70a (exactly its output)
  -r          music only: modules, packed modules, synth formats
  -d          data only: executable and data crunchers
  -rd         both (default)
  -s          scan only: list what is found, write no files
  -j          print a JSON report on stdout instead of the classic text
  -o <dir>    write files into <dir> (created if needed; default: current dir)
  -t <file>   extension table to use (default: "_types_" in the current dir)
  -q          quiet: no console text
  -v, -h      version, help
```

It prints the original's classic console text and writes, for each hit, a
raw rip `<n>.<ext>` plus, for packed modules, a converted `<n>.mod` in the
current directory. Some of the formats the original never had need a second
file next to the rip (a sample bank, a sample file, a synth file): it is
saved as `<n>.<ext2>`, e.g. `3.lsmusic` with `3.lsbank`. With `-c` it behaves
exactly like the original. An optional `_types_` file in the current
directory renames the extensions of the original's formats, one line per
format, as in the original.

```text
$ prowiz -s demo.bin
offset     size       status     format
0x0007a120 111180     saved      NoisePacker v2 module  -> 0.NoisePacker2, 0.mod
0x0010f48c 5002       saved      The Player 6.1A module  -> 1.P61A, 1.mod
0x0018a936 256232     saved      STIM (Slamtilt) module  -> 2.stim, 2.mod
0x0024333e 14080      saved      Kefrens Sound Machine module  -> 3.ksm, 3.mod
0x0036aa7a 107008     saved      Digital Illusion Packed music  -> 4.DigitalIllusion, 4.mod
```

`-j` gives one JSON document (shortened here to the first finding):

```json
{
  "program": "prowiz", "version": "2.0.0", "engine": "Pro-Wizard 1.70a",
  "input": {"path": "demo.bin", "size": 4805776},
  "modes": ["music", "data"], "formats": "all", "scan_only": false, "output_dir": "out",
  "findings": [
    {"index": 0, "format": "NoisePacker v2 module", "format_id": 31, "kind": "music",
     "status": "saved", "offset": 500000, "size": 111180, "ext": "NoisePacker2",
     "header_rebuilt": false, "restored": false,
     "files": [{"name": "0.NoisePacker2", "path": "out/0.NoisePacker2", "role": "rip", "size": 111180, "written": true},
               {"name": "0.mod", "path": "out/0.mod", "role": "converted", "size": 123914, "written": true}]}
  ],
  "summary": {"found": 5, "saved": 5, "files": 10},
  "error": null
}
```

`status` is `saved`, `truncated` (the data runs past the end of the input:
nothing is written, `missing` says how many bytes are absent) or `unrippable`
(recognised, but Pro-Wizard cannot rip it, e.g. The Player 6.1A with packed
samples). A file's `role` is `rip`, `converted` or `companion` (the second file
some formats need). `restored` is true when the module was found as a player
had prepared it in memory (pointers relocated and so on) and the rip is the
module as it was before; `formats` is `"1.70a"` with `-c`.

## Library

```c
#include "prowiz.h"

static int on_file(void *user, const char *name, const uint8_t *data, size_t size,
                   pw_file_role role, const pw_found *f)
{
    /* store or inspect the file; return non-zero to abort the scan */
    return 0;
}

pw_callbacks cb = {0};
pw_result    res;
cb.file = on_file;              /* also: cb.found (detections), cb.log (classic text) */
int rc = pw_scan(data, size, NULL, &cb, &res);
```

`pw_scan()` copies the input and keeps all state in a private context, so
scans can run concurrently. See `include/prowiz.h` for options (modes, custom
extension table) and the format table functions.

## Formats

About 90 music formats and 80 cruncher variants (158 format IDs), the full
set of the original's scanner:

* **Modules:** Protracker / Startrekker / FastTracker 1 (any channel count),
  SoundTracker (15 samples), SoundTracker 2.6, IceTracker, Sound FX, Fuchs
  Tracker, Quadra Composer, DigiBooster 1.7, MED MMD0 / OctaMED MMD1,
  Oktalyzer, FastTracker 2 XM, Impulse Tracker, ScreamTracker 3, AMOS music
  banks.
* **Packed modules (converted back to Protracker):** AC1D, Binary Packer,
  Digital Illusion, Eureka, FC-M, Fuzzac, Game Music Creator, GnoiPacker,
  GnuPlayer, GPMO, HCD-Protector, Heatseeker, Hornet, Kefrens Sound Machine,
  KRIS, Module Protector, Mosh, Newtron (old and new), NoisePacker 1-3,
  Noiserunner, Noise From Heaven, NovoTrade, Perfect Song, PHA, PMD3/PMd3,
  Polka, Power Music, ProPacker 1.0/2.1/3.0, Promizer 0.1/1.0c/1.8a/2.0/4.0,
  ProRunner 1/2, SKYT, StarTrekker Pack, STIM, Stone Arts Player, The Dark
  Demon, The Player 2.2A/3.0A/4.0A/4.0B/4.1A/5.0A/6.0A/6.1A, Titanics, TMK,
  Tracker Packer 1-3, UNIC Tracker 1/2, Wanton, Xann, ZEN.
* **Synth formats (ripped):** AHX/THX, Hively Tracker, Sound Monitor 2/3,
  Future Composer 1.3/1.4, BSI Future Composer, JamCracker, Digital Mugician,
  SidMon 1, Sonic Arranger, Delta Music 1.
* **Executable crunchers (missing hunk headers rebuilt):** ByteKiller (1.3,
  2.0, 3.0, Pro 1.0 and clones), Crunchmania, Defjam 3.2, Double Action, DragPack,
  GNU Packer, HQC, High Pressure, Master Cruncher, Max Packer, Mega Cruncher,
  PowerPacker 2.3/3.0/4.0, RelokIt, Spike, StoneCracker 2.70-3.10, Super
  Cruncher, Syncro Packer, TetraPack, Time Cruncher, TNM, Try-It, Turbo
  Squeezer.
* **Data crunchers:** Imploder, RNC ProPack, XPK, Crunchmania data, Ice!,
  StoneCracker 2.92-4.04 data, PowerPacker banks, Atomik, Automation,
  Speed Packer, Diet and a dozen more.

`docs/upstream/KNOWN_FORMATS.txt` is the original's own list.

## Accuracy

The original is the specification. `tests/reference/` holds its last source
release (`prowiz-1.70a-src.zip`, checksummed) and a script that builds it as a
reference binary. Two tools compare the port with it:

* `tests/difftest.py` runs both on the same inputs, each in an empty
  directory, and compares the console text and every output file byte for
  byte;
* `tests/fuzzdiff.py` does the same for thousands of mutants of seed files.

The inputs are the libxmp test suite's real modules (`tests/fetch_corpus.py`,
285 files) and `tests/corpus/synthetic/` (165 small hand-built files for the
many formats without public samples, each confirmed to be detected by the
original). `sh tests/run_difftest.sh` builds everything and runs the full
comparison.

Current results (Windows, against the original built from its source):

* 458 of the 460 inputs are byte-identical in console text and every file.
  The original never finishes on one (it rips the same Quadra Composer data
  forever). The other one, a Digital Illusion module, differs in 10 bytes of
  its converted `.mod`, where the original read note periods from beyond the
  end of its period table.
* Well over 100,000 fuzzed mutants (per format during the port, plus 14,000
  more across all formats at once): console text and rip files always
  identical; every difference in a converted file was traced to the original
  reading or writing outside one of its local arrays.
* Built for Linux, the port produces the same output as on Windows (the one
  exception is inherited: the original's StoneCracker 2.70 extension ends in
  a space, which Windows drops from file names). An AddressSanitizer/UBSan
  build runs the corpus and 5,800 fresh mutants without a single report.

Where the original misbehaves, the port does something defined instead (all
documented in [docs/PORTING.md](docs/PORTING.md)):

* it read past the end of small local tables on corrupt data (e.g. a note
  number beyond the 37-entry period table): its output then depended on stack
  contents; the port reads 0;
* a few inputs make it rip the same data forever or loop in a test; the port
  saves once and carries on;
* it trusted sizes from the data and could crash; the port bounds every
  access.

## Origins and credits

Pro-Wizard for PC is by Sylvain "Asle" Chipaux (1997-2016,
<http://asle.free.fr/prowiz/>), after Nicolas "Gryzor" Franck's Pro-Wizard 2 on
the Amiga; see `docs/upstream/CREDITS.txt` for everyone who contributed to
it. 1.70a (last changed 2017) is the latest release. WinUAE's integrated
ripper carries the same 1.70a code, and libxmp an older subset of its
converters.

Licensed under the GNU General Public License, version 2 (`COPYING`), like
the original.
