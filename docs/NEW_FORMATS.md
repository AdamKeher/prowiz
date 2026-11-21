# Formats Pro-Wizard 1.70a never had

prowiz finds these formats on top of the original's. They were researched
and implemented on 2026-11-21. They are on by default; `-c`
(`PW_FORMATS_170A`) turns them off, and the engine is then exactly the 1.70a
one (difftest and fuzzdiff run the port that way).

Each section below gives the format's layout, the size rule, the checks,
what players change in memory, the reference code, where the test files come
from, and implementation notes. Offsets are hex and relative to the module
start; all fields are big-endian.

**Exact** counts the real files (mostly from Modland, plus each tool's own
examples) whose rip has exactly the file's size. The others are listed with
the reason (data after the module, files cut short, variants not handled) in
the format's test list, `tests/newformats/<Format>.txt`.

## Summary

| Format | ID | Signature | Files saved | Exact | In memory dumps |
|---|---|---|---|---|---|
| OctaMED MMD2/MMD3 | 168, 169 | `MMD2`/`MMD3` | `.mmd2` / `.mmd3` | 41/41 MMD2, 51/51 MMD3 | relocated MMD0-MMD3 restored (164/164 test relocations exact) |
| PreTracker | 170 | `PRT` + version | `.prt` | 83/83 | verbatim; Raspberry Casket patch undone |
| SidMon 2 | 146 | `SIDMON II - THE MIDI VERSION` at 3A | `.Sidmon2` | 101/107 | intact; sample bytes may be negated |
| Delta Music 2 | 171 | `.FNL` at BC6 | `.dm2` | 93/98 | player globals, waveform 0 change |
| Art of Noise | 172 | `AON4`/`AON8` + known chunks | `.aon` | 209/211 | verbatim |
| DigiBooster Pro | 176 | `DBM0` + version 2/3 | `.dbm` | 58/58 (+3 libxmp) | rare (loaded into structures) |
| Musicline Editor | 177 | `MLEDMODL` | `.ml` | 102/104 | rare (loaded into structures) |
| Symphonie | 178 | `SymM` + version 1 | `.symmod` | 29/29 | rare |
| LightSpeedPlayer | 173 | `LSP1` / `LSPm` | `.lsmusic` + `.lsbank` | 38/38 music, 32/32 banks | relocation undone; ADPCM bank packed again |
| Startrekker AM | 22 + 174 | `.nt`: `ST1.x ModuleINFO`, `AudioSculpture10` | `.mod` + `.mod.nt` | every `.nt` exact | `.nt` paired with its mod by position |
| Startrekker 8 ch. | 175 | `FLT8` / `EXO8` | `.mod` (+ `.mod.nt`) | 1/1 | - |
| TFMX | 179, 180 | `TFMX-SONG ` and others; TFHD, TFMXPAK | `.mdat` + `.smpl`; `.tfx` | mdat 346/368, smpl 277/368 | smpl found only right after its mdat |
| Rob Hubbard | 181 | 5 × `bra.w` + code | `.rh` | 12/12 (2 effect banks not handled) | - |
| Ben Daglish | 182 | `bra.w` table + init code | `.bd` | 28/44 (15 have data after the module) | offsets, intact |
| David Whittaker | 183 | 3 code prologues | `.dw` | 92/121 (7 variants not handled) | initialised copies |

Also new: Startrekker `EXO4` modules (the original found only `FLT4`).

Packed productions: LSP and PreTracker songs in released demos and intros are
almost always inside crunched executables (Shrinkler, Cranker, LDOS packers).
They will mostly be found in memory dumps, decrunched files and plain data
files.

## How they fit in

- **Code:** `src/formats/new/`. `new_formats.h` states the conventions.
  `pw_scan_new()` runs after the original's checks at each scan position,
  and `scan.c` has a few hooks inside the original's MMD and Startrekker
  checks (grep `NEW_FORMATS`).
- **IDs:** 168-183 come after the 1.70a table, so `_types_` files keep their
  meaning; their extensions are fixed (`pw_new_ext`). SidMon 2 reuses ID 146,
  which 1.70a reserved but never used.
- **Second files:** saved as `<n>.<ext>` with the role `companion`
  (`pw_save_companion*()`), and claimed so they are not ripped again.
- **Restored copies:** a rip rebuilt from a copy a player prepared in memory
  is flagged `restored` (`pw_save_rip_data()`).
- **Tests:**
  - `tests/newformats.py` runs the lists in `tests/newformats/`. Each module
    is checked alone and embedded in noise, for its exact rip and companions,
    and the run fails on any false positive in the noise.
  - Real files are referenced by URL and sha256 and downloaded into a
    git-ignored cache. The small synthetic files under
    `tests/newformats/files/` are committed, with the scripts that make them,
    and CI runs those (`--offline`).
  - `tests/fuzznew.py` hunts crashes and hangs with a sanitizer build.

---

## OctaMED MMD2 / MMD3

- **Origin:** Teijo Kinnunen / RBF Software.
  - MMD2 comes from OctaMED Pro 5-6 (1993-95).
  - MMD3 comes from OctaMED Soundstudio (1996 onwards), and also from MED
    Soundstudio for Windows.
  - Both share one layout. The MMD3 ID means "uses Soundstudio mixing".
- **Size:** `modlen` (u32 at 04) is the whole file, exactly as for MMD0/1.
  - **Multi-song files** (`extra_songs` at 33 > 0) chain further headers
    through MMD0exp+00 `nextmod`, with IDs `MCN2`/`MCN3`.
  - Their `modlen` is 0, and their pointers are offsets from the first
    header. The first header's `modlen` covers everything.
  - **Exceptions:** 1 of 322 Modland MMD2/MMD3 headers probed has a
    `modlen` 4 bytes short of the file. The extra bytes are an Amiga
    HUNK_END (`000003F2`): the module was cut out of an executable and
    `modlen` is right. libxmp's `med_hold_1exx_ss2.med` (MED Soundstudio 2)
    has `modlen` 0.
- **Differences from MMD0/1 that matter for a test:**
  - The song structure (MMD2song, 314 bytes) is not at 34. The playseq,
    section and track-volume tables come first; the song was found between
    6E and F8.
  - MMD2song fields: +1F8 numblocks, +1FA songlen, and pointers at +1FC,
    +200, +204 and +20C.
  - Blocks use the MMD1 layout.
  - `mmdflags` (14) bit 0 is set in MMD3 files.
  - MED 3.00 did not align its MMD0 structures (the spec admits it):
    libxmp has MMD0 files with odd `blockarr`, `smplarr` and `expdata`.
- **Upstream support was started:** the original has `'2'`/`'3'` commented
  out of its `MMD` check (`src/scan.c` ~1693). `pw_rip_MMD0` saves only IDs
  ending in `0`/`1` (`src/formats/MED-Octamed.c` ~59). `pw_test_MMD0`
  already passes every MMD2/MMD3 file examined.
- **In memory:** `RelocModule` adds the load address to every non-null
  pointer.
  - Pointers covered:
    - header 08, 10, 18 and 20;
    - smplarr and blockarr entries, BlockInfo and pages;
    - the MMD2song table pointers and each PlaySeq;
    - expdata and its sub-pointers;
    - each `nextmod` header.
  - Exception: synth waveform pointers get the instrument's own address
    added, not the base.
  - The player also writes 0C, 0E, 28-2E and 32 while playing.
  - `modlen`, the ID and every count are untouched.
- **References:**
  - The spec is `mmd3_r6.txt` (libxmp `docs/formats/`).
  - Reusable code: libxmp `mmd3_load.c` and `mmd_common.c` (MIT), OpenMPT
    `Load_med.cpp` (BSD), NostalgicPlayer OctaMed (MIT).
  - The official `reloc.a`/`proplayer.a` licence allows only free
    programs, so read them as a spec only.
- **Test files:**
  - Modland: 305 MMD2 and 660 MMD3.
  - Multi-song examples: Olek/love story blues.mmd2, Spoochy/freedom-title
    (remix).mmd2, Elapse/pyro-electricity.mmd3.

### Implementation (`src/formats/new/MMD23.c`)

- **MMD2 / MMD3 files** (`pw_new_MMD23`, at "MMD2"/"MMD3" in `scan.c`):
  `PWF_MMD2` "OctaMED Pro (MMD2) module" and `PWF_MMD3` "OctaMED
  Soundstudio (MMD3) module". The size is `modlen`.
  - **Checks:** the structures are walked, and every pointer must land
    inside [34, modlen) on an even offset:
    - `modlen` ≥ 348 (header + song);
    - `mmdflags` ≤ 1 and the reserved bytes 15-17 zero (as `pw_test_MMD0`);
    - the song fits; numblocks ≥ 1; numsamples ≤ 63;
    - MMD2song: numpseqs ≥ 1 and each PlaySeq fits; songlen ≥ 1 and every
      section number < numpseqs; numtracks ≤ 64 and the track volume / pan
      tables fit;
    - every block has 1-64 tracks, and its notes and BlockInfo fit;
    - every instrument has a type from -4 to -1, or a sample type 0-8 plus
      the 16-bit / stereo / delta / packed flags, and its data fits (a
      stereo sample is twice its length);
    - MMD0exp: its pointers, and the InstrExt, annotation, InstrInfo and
      song name tables, fit;
    - each further song of a multi-module is an "MCN" header after the
      previous one (so there are no cycles), with a valid song and MMD0exp.
  - **Deliberately not tested:** `modlen` being even (it need not be), and
    the word at 30 being FFFF (a player field, not a structure).
  - **`modlen` 0** is accepted when every structure is in the input. The
    size is then the end of the furthest structure, rounded up to even:
    blocks with their BlockInfo data, instruments with their waveforms, and
    MMD0exp with its tables.
  - **Truncated modules:** the data that only part of the input holds is
    not checked, so a module cut by the end of the input is still reported
    (TRUNCATED).
- **Relocated MMD0-MMD3** (`pw_new_MMD_relocated`):
  - **When it runs:** `scan.c` calls it at "MMD0"/"MMD1" when 1.70a's
    `pw_test_MMD0` rejects the header, and at "MMD2"/"MMD3" when
    `pw_new_MMD23` finds no file. 1.70a's own rips stay as they were, even
    of a module relocated to a low address.
  - **Load address:** every non-null header pointer p was an offset in
    [34, modlen). So the address B is in [max(p) - modlen + 1,
    min(p) - 34], and B ≥ 400 (the 68k vectors are below).
    - Candidates are tried from the top down: the song at 34 first, then
      further in, since MMD2/MMD3 songs come after their play sequences.
    - At most 64 Ki candidates are tried. Each gets a quick look at the
      song, then the full checks of a file, reading pointers minus B.
  - **Restore:** a copy of the module is walked as `reloc.a` walks it, and
    every pointer it relocated gets B taken off. Synth waveform pointers get
    the instrument's address taken off instead.
    - The walk covers the header, the instruments of the first song, the
      block table and BlockInfo, the MMD2song tables and play sequences,
      and MMD0exp with its MIDI dumps, first ARexx trigger and command 3x
      tables. Then it does each further song while `extra_songs` lasts.
    - `reloc.a`'s loop quirks are repeated:
      - byte counters for the sample and block tables: 0 means 256, and a
        multiple of 256 blocks walks 255 more;
      - 65536 rounds for a zero waveform, page or play sequence count;
      - no BlockInfo walk above 32768 blocks.
    - The player fields of every header are reset: 0C-0F, 28-2F and 32 to
      0, and 30 to FFFF.
    - The result must pass the checks of a file.
  - **Saved** with `pw_save_rip_data(..., restored=1)`:
    - MMD0/MMD1 under 1.70a's `PWF_MED` ("MED (MMD0) module", "OctaMED
      (MMD1) module");
    - MMD2/MMD3 under their own IDs.

    A relocated module cut by the end of the input is only reported
    (TRUNCATED), since there is nothing to restore.
- **Bounds:** the walks have a work budget: 4 Mi structures per finding,
  and for the undo, half the module size more. Tables that repeat one
  another cannot make a scan quadratic.
- **Tests** (`tests/newformats/MMD.txt`):
  - 50 MMD2 and 52 MMD3 files, 7 of them multi-song;
  - synthetic MMD2 and multi-song MMD3 files;
  - relocated MMD0, MMD1, MMD2 and multi-song MMD3 memory images, made by
    `tests/newformats/files/MMD/make_mmd.py`, whose `reloc()` follows
    `reloc.a`.

  Relocating 164 real MMD0-MMD3 files (Modland and libxmp) the same way and
  scanning the images restores every one of them exactly.
- **Limitations:**
  - Modules relocated by other routines (game players, MED packers) are
    only found if they follow `reloc.a`.
  - A structure reached twice is fixed once, where `reloc.a` would add B
    twice.
  - ARexx trigger commands after the first one are left alone: `reloc.a`
    follows their unrelocated `next` pointers out of the module.
  - The trackinfo and effectinfo tag lists of Soundstudio MMD3s
    (MMD0exp+44/48) are neither relocated by `reloc.a` nor checked here.
  - A relocated module is not found if its `modlen` is 0 or its song is
    more than 128 KiB into the module.

## PreTracker

- **Origin:** Pink/Abyss (Manfred Linzner). 1.0 appeared at Revision 2019,
  and 1.5 in April 2022 with up to 31 subsongs and SFX. The extension is
  `.prt`.
- **Out of scope:** the 1.6 "packed register dump" player and PreTracker
  2.0 contain no `.prt`.
- **Contents:** a single file of synthesis parameters. The player renders
  the waves into a separate chip buffer.
- **Header:**

| Off | Size | Field |
|---|---|---|
| 00 | 3 | `PRT` |
| 03 | 1 | version: 18 (before 0.3), 19 (0.3-0.866), 1A (0.87-0.92), 1B (0.93-1.02), 1E (1.5); 1C/1D never seen |
| 04 | 4 | POSD offset (positions) |
| 08 | 4 | PATT offset (patterns) |
| 0C | 4 | INST offset (instrument names) |
| 10 | 4 | WAVE offset (wave names) |
| 14 | 20 | song name |
| 28 | 20 | author |
| 3C-3F | 4 | restart, npat, npos, nsteps (versions ≤ 1D; 0 in 1E) |
| 40 | 1 | ninst (≤ 32; always 40 in 1E) |
| 41 | 1 | nwav (1-24) |
| 42 | 24 | wave order (only when version > 19) |
| 5A | 1 | nsub (versions ≥ 1D) |

- **Layout choice** (as the official 1.5 player does it):
  - versions ≤ 1C use the old layout;
  - 1D and 1E have subsong headers;
  - only 1E has 64 instead of 32 instrument names.
- **Version 1E:**
  - POSD starts with nsub × 8-byte headers
    `{restart, npat, nsteps, npos, u32 pattern offset from PATT}`.
  - The positions of all subsongs follow, concatenated.
- **Size** (exact on 83/83; there is no end marker):

  ```
  p = WAVE;  24 times: skip a name (to after its NUL, at most 23 bytes)
  if (p & 1) p++                 /* even relative to the module start */
  size = p + 42 * nwav
  ```

- **Hard checks:**
  - POSD < PATT ≤ INST < WAVE, all inside the input.
  - The chain matches:
    - POSD + 8·npos (1E: + 8·nsub + 8·Σnpos) == PATT;
    - PATT + Σ npat·nsteps·3 == INST;
    - INST, then 32/64 names, then 8·ninst bytes of instrument info,
      then Σ 3·steps (the step count is byte 7 of each 8-byte record)
      == WAVE.
  - Every position's pattern number ≤ npat.
  - In every 42-byte wave record, the word at +0A == (byte +19 + 1) × 128.
    This held for 1027/1027 records.
- **Do not check:**
  - restart < npos;
  - instrument-info bytes ≤ 15.

  Real files break both.
- **Prototype result:** no false positives in 20,000 random `PRT` buffers or
  in the existing 460-file corpus.
- **In memory:** the official players only read the song. Raspberry Casket
  with V1.5 support patches the song in place:
  - $04 += 8;
  - $3C-$3F := the subsong-0 header bytes in the order [0],[1],[3],[2].

  Recognise POSD == 64 with those bytes, then restore 04 = 5C and clear
  3C-3F.
- **References** (reusable): Daniel Collin's `pretracker_parse.c` (MIT,
  github.com/RetrovertApp/playback-pretracker); Raspberry Casket (MIT,
  git.platon42.de/chrisly42/PretrackerRaspberryCasket).
- **Test files:** Modland has 64; the PreTracker 1.00 and 1.5 release zips
  add 18 more, including the multi-subsong files Tinyus and Wrong Way
  Driver.
- **Implementation** (`src/formats/new/PreTracker.c`, "PreTracker module"):
  - Signature: `PRT` and a version 18-1E. The layout is chosen as the
    official 1.5 player does (see above). POSD must not overlap the header
    (POSD ≥ 43 / 5B / 5C).
  - Checks: the hard checks above, plus: npat, nsteps and npos ≥ 1 for the
    song or every subsong; nsub 1-31; ninst ≤ 32 (≤ 64 in 1E); the
    subsongs' pattern offsets follow one another (true in every 1E file).
  - The header and the positions must be in the input. When the input ends
    before WAVE, the instrument chain cannot be checked and the module is
    reported TRUNCATED; its size then counts the missing names as empty, so
    it is a lower bound.
  - Raspberry Casket's patch is recognised as POSD = 64 with 3C-3F equal to
    the subsong-0 header bytes [0],[1],[3],[2] (a real 1E file has 0 there,
    and npat ≥ 1). The rip is restored, POSD = 5C and 3C-3F = 0, and
    flagged restored.
  - Not checked: the wave-order permutation (rePlayer reports a file,
    "wigged" by Curt Cool, with an odd order), the reserved bytes, the
    POSD constants.
  - Tests (`tests/newformats/PreTracker.txt`): the 64 Modland files and 5
    synthetic ones (versions 19, 1B, 1E with 1 and 2 subsongs, and the
    Raspberry Casket image), all exact alone and embedded. The 19 songs of
    the two release zips (8 of them not on Modland) also pass with a
    temporary list. No finding in the 460-file corpus.

## SidMon 2

- **Origin:** Michael Kleps (Unknown/DOC).
- **File:** a single file with no player code; Modland calls it `.sid2`.
- **Layout:**

| Off | Field |
|---|---|
| 00 | u16 MIDI mode (0) |
| 02 | u8 positions − 1 |
| 03 | u8 speed |
| 04 | u16 sample-info size (64 × samples) |
| 06 | u32 1C (ID length) |
| 0A | u32 4 |
| 0E-32 | u32 sizes of PT, NT, IT, I, WL, A, V, SI, TT, T |
| 36 | u32 ignored by every loader (0 to > FFFF) |
| 3A | `SIDMON II - THE MIDI VERSION` |
| 5A | the ten blocks, in the order above |

- **Size:**

  ```
  end = 5A + PT+NT+IT+I+WL+A+V+SI+TT+T
  if (end & 1) end++
  for each sample i < u16[04]/64:
      end += 2 * u16(SI_i + 4)
  ```

  This is exact on 101/107. NostalgicPlayer's pad is a no-op bug, and 52
  files need the pad.
- **Checks** (all hold on 107/107):
  - PT = NT = IT = 4·(byte 02 + 1);
  - SI a multiple of 64, I of 32, WL/A/V of 16;
  - track-table entries ascending from 0 and < T;
  - max position byte + 1 = TT/2.
- **The original's `sidmon2.c`** (never called by its scanner):
  - It always saves the 58-byte header only.
  - The sum it computes is not the module size.
  - It rejects 42/107 real files by treating the long at 36 as a block
    size.

  Format ID 146 `Sidmon2` already exists.
- **In memory:** sample pointers in SI+0 are absolute. The negation effect
  inverts sample bytes in place, so the size still computes but sample data
  may differ from the file.
- **References** (reusable): NostalgicPlayer `SidMon20` (MIT), UADE
  `amifilemagic.c` (GPL/PD).
- **Test files:** Modland has 107.

- **Implementation** (`src/formats/new/SidMon2.c`, ID 146 `Sidmon2`):
  - Triggered at the `S` of the ID; the module starts 3A bytes earlier.
  - Checks: 06 = 1C, 0A = 4, PT = NT = IT = 4·(byte 02 + 1), I a non-zero
    multiple of 32, WL/A/V multiples of 16, SI = u16[04] and a multiple of
    64, TT even and ≥ 2, T > 0, every block ≤ 1 MB, the track table
    ascending from 0 and below T, every position < TT/2, every
    instrument's waveform list < WL/16. The long at 36 is ignored.
  - Size as above (exact on 101/107 Modland files).
  - Tests (`tests/newformats/SidMon2.txt`): 2 synthetic files (with and
    without the pad byte) and 106 Modland files (101 exact, 5 with
    `size=` for bytes after the module); the truncated copy is not
    listed.

## Delta Music 2

- **Origin:** Bent Nielsen.
- **File:** a single file containing its own replayer code; `.dm2`.
- **Layout:**

| Off | Content |
|---|---|
| 000-877 | replayer code; first 16 bytes `02 00 00 03 4A 00 67 00 03 B4 0C 00 00 01 67 00` |
| 878-AE1 | channel structures |
| AE2 | period table, AA bytes, constant (`00 00 1A C0 19 40 17 D0 ...`) |
| B8C-BC5 | player globals; BBB = start speed |
| BC6 | `.FNL` |
| BCA | 64 × 16 arpeggios |
| FCA | 4 × (u16 loop, u16 length) |
| FDA | tracks, then u32 BL + blocks (64 bytes each) |
| then | 127 × u16 instrument offsets + u16 IL, then IL bytes of instruments (88 each) |
| then | u32 WL + waveforms (256 each) |
| then | 8 × u32 sample lengths + 32 zero bytes |
| then | 8 × u32 sample offsets, then the sample data |

- **Size:** (end of the offset table) + Σ of the 8 sample lengths. This is
  exact on 93/98.
- **Do not** use max(offset + length): the offsets are garbage in 4/98
  files.
- **Trigger:** trigger on `.FNL` with start = i − BC6, then check the period
  table and the code prefix. Check bytes 4-F of the code only: one Modland
  rip (`coutesy.dm2`) has bytes 0-3 cleared.
- **Other checks:**
  - BL a multiple of 64, IL of 88, WL of 256;
  - the 32 bytes after the length table are 0;
  - sample numbers ≤ 7.
- **In memory:**
  - The player rewrites the globals and channel structures.
  - It overwrites waveform 0 with noise every tick.
- **References** (reusable): NostalgicPlayer `DeltaMusic20` (MIT) and its
  format description.
- **Test files:** Modland has 98.

- **Implementation** (`src/formats/new/DeltaMusic2.c`, ID 171, `.dm2`):
  - Checks: `.FNL`; the 85-word period table at AE2; bytes 4-F of the
    replayer; track lengths even with the loop inside; every track step
    names a block < BL/64; BL a multiple of 64; IL a multiple of 88 and
    the used instrument offsets multiples of 88 below IL; sampled
    instruments use samples 0-7; WL a multiple of 256; the 32 bytes after
    the length table are 0; every part ≤ 1 MB.
  - Size: end of the offset table + the 8 sample lengths (93/98 exact).
  - Tests (`tests/newformats/DeltaMusic2.txt`): 2 synthetic files (their
    replayer area is zero apart from the 16 checked bytes) and 95 Modland
    files (93 exact, 2 with `size=`); the 3 truncated copies are not
    listed.

## Art of Noise

- **Origin:** Bastian Spiegel (Twice/Lego), 1993-96; `.aon`.
- **Variants:**
  - AON4: 4 channels, patterns 400 bytes.
  - AON8: 8 software-mixed channels, patterns 800 bytes.
- **Header:**
  - 00 `AON4`/`AON8`.
  - 04: 42 bytes of text. In the 211 Modland files:
    `artofnoise by bastian spiegel (twice/lego)` (163 files),
    `ArtOfNoise by Bastian Spiegel(twice/lego)` + NUL (36),
    `Chorus by Bastian Spiegel(twice/lego)` + NUL + `B-` + NUL + LF (11),
    and one with a corrupted letter. So it is not a usable signature.
  - The chunks start at 2E.
- **Chunks** (ID + u32 length, no padding), in order:
  - optional NAME, AUTH, DATE, RMRK;
  - INFO (4 bytes; byte 0 = 34, byte 1 = song length);
  - ARPG (40);
  - PLST;
  - PATT ((highest pattern + 1) × 400 or 800);
  - INST (n × 20, at most 61);
  - optional INAM (7A0);
  - WLEN (100: 64 u32 wave lengths);
  - WAVE (Σ WLEN).
- **Size:** walk the chunks to the end of WAVE. Then round up to a
  multiple of 4 if the next bytes are `?` (the editor's pad).
- **Checks:**
  - every chunk ID is known, and appears once;
  - INFO byte 0 = 34;
  - PLST length = (song length + 1) & ~1;
  - PATT a multiple of 400/800 and every PLST entry below the pattern
    count;
  - instrument type 0 (sample) or 1 (synth);
  - WAVE length = WLEN sum.
- **In memory:** used in place and not relocated, so a dump holds the file,
  often right after the player (the editor's "player+module image" is 7476
  bytes of `player4ch` followed by the module).
- **References:** NostalgicPlayer ArtOfNoise (MIT) is reusable. The
  original editor source (miditracker.org) has no licence, so it is spec
  only.
- **Test files:** Modland has 211 (178 AON4, 33 AON8).
- **Checked on all 211:** the chunk order is always NAME AUTH DATE RMRK
  INFO ARPG PLST PATT INST [INAM] WLEN WAVE (one file has only INFO to
  WAVE). INFO is always 4 bytes starting with 34, ARPG 40, WLEN 100, INAM
  7A0; INST is a multiple of 20 with types 0/1; WAVE = Σ WLEN. After WAVE
  come: nothing (141 files), the `?` pad to a multiple of 4 (68), or zero
  bytes that are not part of the module (2: Tivurr "herb", 4 bytes; Twice
  "soundfx", 2 bytes). 209 files are exact.
- **Implementation** (`src/formats/new/ArtOfNoise.c`, "Art of Noise
  (AON4) module" / "Art of Noise (AON8) module"):
  - Signature: `AON4`/`AON8` and a known chunk ID at 2E.
  - The walk takes the twelve IDs above, each at most once, and ends after
    WAVE. Text chunks are limited to 64 KB, any chunk to 16 MB.
  - Checks: INFO (4 bytes, 34, song length ≥ 1) before PLST before PATT;
    the PLST length; PATT a non-zero multiple of 400/800 with every played
    pattern below the count; INST 1-61 records of type 0/1; INAM a
    multiple of 20 (≤ 61 names); ARPG 40; WLEN 100; WAVE = Σ WLEN (when
    WLEN is in the input), after INST, PATT and ARPG.
  - The `?` pad is added only when all its bytes are `?`.
  - Truncated modules (INFO, PLST and PATT seen) are reported TRUNCATED.
    The size is exact when the input ends inside WAVE; otherwise it is a
    lower bound (the end of the chunk that runs past the input).
  - Tests (`tests/newformats/ArtOfNoise.txt`): the 211 Modland files (2
    with a size= for the stray zero bytes) and 2 synthetic files (AON4 with
    the pad, AON8 with INAM), all exact alone and embedded. No finding in
    the 460-file corpus.

## DigiBooster Pro (DBM0)

- **Origin:** DigiBooster Pro 2.x (Piasta) and 3.x (Kraszewski); `.dbm`.
  It is unrelated to DigiBooster 1.7.
- **Header:**
  - 00 `DBM0`;
  - 04 version 2 or 3;
  - 05 revision (BCD);
  - 06-07 reserved: not zero in practice, so do not test them.
- **Chunks** (ID + u32 length, from 08): NAME, INFO (5 × u16: instruments,
  samples, songs, patterns, tracks), SONG, INST, VENV, PENV, PATT, PNAM,
  SMPL, DSPE, MPEG.
- **Size:** walk the chunks until an unknown ID.
- **Checks:**
  - INFO comes before the content chunks;
  - instruments, samples and songs 1-255; patterns ≥ 1;
  - tracks even, 2-254;
  - sample flags & 7 in {1, 2, 4};
  - VENV/PENV = 2 + 136·n.
- **References** (reusable): libdigibooster3 (BSD-2, the official loader),
  libxmp `dbm_load.c` (MIT), OpenMPT `Load_dbm.cpp` (BSD), NostalgicPlayer
  (MIT).
- **Test files:** Modland has 846, and libxmp test-dev about 14 (3 real
  modules, the rest broken on purpose). All 42 Modland headers probed are
  version 2 (revisions 2.12-2.21); no real DigiBooster 3 file was found.

### Implementation (`src/formats/new/DigiBoosterPro.c`)

- `pw_new_DigiBoosterPro`, at 'D': `PWF_DigiBoosterPro`, "DigiBooster Pro
  module" (version 2) or "DigiBooster 3 module".
- **Signature:** `DBM0`, version 2 or 3, a BCD revision, and NAME or INFO
  as the first chunk.
- **Size:** the chunks are walked from 08 until an ID that is not a DBM0
  chunk (at most 64 chunks).
- **Checks:** as in libdigibooster3's `read_*()`:
  - each chunk appears once;
  - INFO comes before SONG, INST, VENV, PENV, PATT, SMPL and DSPE, and its
    counts are in range;
  - SONG, INST, PATT and SMPL are present;
  - SONG (46 + 2 per order, per song), INST (50 per instrument), PATT
    (rows ≥ 1 and a non-zero packed size, per pattern), SMPL (8 + frames ×
    bytes per sample) and VENV / PENV (≤ 255 envelopes of 136 bytes) all
    fit in their chunk;
  - DSPE has one mask byte per track.
- **Truncated modules:** a chunk running past the end of the input is
  reported (TRUNCATED). So is a chunk ID cut by it.
- **Tests** (`tests/newformats/DigiBoosterPro.txt`): 58 Modland files, the 3
  real libxmp modules, and synthetic version 2 and 3 files
  (`tests/newformats/files/DigiBoosterPro/make_dbm.py`; the version 3 one
  has PNAM and DSPE).

## Musicline Editor

- **Origin:** the Cyréus brothers and others, 1993-95; `.ml`.
- **Header:** 00 `MLED`, 04 `MODL`, 08 u32 length (4 to 28 hex in practice)
  and that many bytes, then `VERS`.
  Very old files have `VERS` at 08.
- **Chunks** (ID + u32 length, unaligned): TUNE, PART, ARPG, INST, SMPL,
  INFO. The player (`mlineplayer102.asm`, LoadHeader) reads them one after
  the other and stops at the first other ID.
  - **TUNE** is read by its contents, not by its length: 40 bytes (decimal),
    then channels × u32 sizes, then the sequences (no sizes when channels =
    0). Files up to version 1.06 store a length as if all 8 sizes were
    there: 16 bytes too much for 4 channels, 32 for none (6 of the 104 files
    checked).
  - **SMPL** takes 8 + 6 + length bytes: the stored length leaves out a
    6-byte prefix (u32 unpacked length, delta command, pad). The length
    covers the 50-byte (decimal) sample header and the data, packed or not.
- **Size:**
  1. Skip the MODL data and VERS.
  2. Walk the chunks until an unknown ID (or one with impossible fields).
- **Checks** (numbers in decimal):
  - the first chunk after VERS is TUNE;
  - TUNE: channels ≤ 8, play mode 0/1, sequence sizes even and ≤ 512 (the
    player's buffer), length = 40 + 4·channels + Σ sizes, or that plus
    4·(8 − channels);
  - PART: number < 1024, length 3 to 2 + 128·13 + 2;
  - ARPG: number < 256, (len − 2) % 6 == 0, len − 2 ≤ 768;
  - INST: length ≤ 206 (`inst_SIZEOF`; always 206 in practice);
  - SMPL: length ≥ 50, the data no longer than the unpacked length.
- **Packing:** samples can be delta-packed and parts are packed, but the
  stored lengths cover both.
- **References:** NostalgicPlayer MusiclineEditor (MIT) is reusable. The UADE
  deliplayer and format doc are LGPL-2.1 (68k assembly, a spec in practice).
- **Test files:** Modland has 817.
- **Checked:** 104 files (100 picked from Modland, up to 3 per artist, plus
  4 more): 102 are exact. Andrew "percubic" is truncated on Modland (its
  last SMPL misses 6261 bytes). Mark Sheeky "xenex-remix" has a PART whose
  length says 70 but that holds 69 bytes, so the next PART starts one byte
  early: the player stops loading there, and so does the rip (1872 of 43919
  bytes).
- **Implementation** (`src/formats/new/Musicline.c`, "Musicline Editor
  module"):
  - Signature: `MLEDMODL`, then MODL data (≤ 256 bytes) and `VERS` (≤ 256
    bytes), or `VERS` at 08. The first chunk must be a valid TUNE.
  - The walk follows the player (TUNE by its contents, SMPL + 6, the
    others by their length) with the checks above, for at most 4096
    chunks, and ends at the first ID that is not a valid chunk.
  - A chunk that runs past the input makes the module TRUNCATED, with a
    size up to the end of that chunk (a lower bound if more chunks follow).
  - In memory the player keeps no copy of the file, so only file images
    are found.
  - Tests (`tests/newformats/Musicline.txt`): 99 of the 100 Modland files
    (percubic left out; xenex-remix with size=1872) and 2 synthetic files
    (MODL data, VERS 1.15, two subsongs, a packed SMPL; VERS at 08 with old
    TUNE lengths), all exact alone and embedded. No finding in the 460-file
    corpus.

## Rob Hubbard

- **What a file is:** raw game player code followed by song data and
  samples; Modland `RH.`/`.rh`. Rob Hubbard's 1989-91 Amiga routine.
- **Signature** (even offsets only):
  - `6000` at 00, 04, 08, 0C and 10, each with a positive even
    displacement;
  - `41FA` at 14;
  - `4E75 41FA` at 1C.
- **Size** (exact on 12/12):
  1. The sample count is n+1 from the `72nn` (moveq #n,d1) right before the
     first `2418` within 8 words of 40.
  2. The samples start at the target of the first `41FA` within 5 words of
     36, plus the immediate of a `D1FC` (adda.l #imm,a0) right after it
     (always 40 in the known files).
  3. Walk the `[u32 len][u16 freq][data]` records. Reject len > 10000.
  4. Add 2 for the closing `4E71`, which must be present unless the module
     runs past the input (then it is reported as truncated).
- **Out of scope:**
  - sound-effect banks (4 × `bra.w`): `fxlarge.rh`, `fxsmall.rh`;
  - tunes with separate samples (Populous and the other Delitracker
    customs);
  - Rob Hubbard ST (`RHO.`), Steve Bak's routine, which has a heuristic end;
  - Hubbard's 1987-88 tunes made with David Whittaker's routine (Leviathan,
    QBall): see David Whittaker.
- **Implementation:** `src/formats/new/RobHubbard.c`, as above. Tests:
  `tests/newformats/RobHubbard.txt`, all 14 Modland files (12 exact; the 2
  sound-effect banks are listed as not detected) plus a synthetic file.
- **References:** the Wanted Team EagleRipper and player are not open
  source, so they are spec only. Flod is CC BY-NC-SA, also spec only.
- **Test files:** Modland has 14 + 3 RHO.

## Ben Daglish

- **What a file is:** raw, position-independent code + data; `BD.`/`.bd`.
  Colin Dooley's sound player.
- **Signature** (even offsets only):
  - `6000` at 00, 04 and 0A, each with a positive even displacement;
  - at the init target (00's): `3F00 6100`, `3D7C` at +6, `41FA` at +C.
- **Instrument tables.** A table is a list of longword offsets (from the
  table) to records: `00` sample offset, `04` loop offset (0: none), `08`
  length and `0A` loop length in words, `0C` volume (0-40 hex). Its length
  is the smallest offset. The sample and loop offsets of **both** tables
  count from table 1: with that base, the second table ends exactly at the
  file end in 10 files.
  1. Table 1, the music: after the first `D040 D040 D040 41FA` in the init
     code, the next `41FA`.
  2. Table 2, the sound effects (used by the third entry point): the first
     `D040 D040` there, if `41FA` follows and its target starts with a zero
     word.
- **Size:** the end of the last sample (or loop), over both tables.
  - Table 1 must fit in the input: if it does not, the module is reported
    as truncated.
  - Table 2's samples count only as far as they fit in the input. Some rips
    lack the last effects: Super Cars' table describes samples up to 126010,
    the file has 70404 bytes.
- **Checked:** 28/44 exact. Of the other 16:
  - 6 differ by padding of 1-34 bytes;
  - 8 have more data after the last sample (78 bytes to 40 KB; Axel's Magic
    Hammer carries 20 KB of unrelated data);
  - Super Cars (see above);
  - Rick Dangerous 2 "patched" is 150 bytes shorter than its effects table,
    so its size depends on what follows it in the input. It is not in the
    test list.
- **Implementation:** `src/formats/new/BenDaglish.c`. A table with more
  than 256 records, an odd record offset, a record outside the input, a
  volume above 64 or a zero sample offset is not a table.
- **References** (reusable): EaglePlayer 2.04's ripper
  `Src/Engines/Eaglerippers/Benn Daglish.s` (GPL-2+), and NostalgicPlayer
  BenDaglish (MIT).
- **Test files:** Modland has 44.

## LightSpeedPlayer (LSP)

- **Origin:** Arnaud Carré (Leonard/Oxygene), 2021 onwards, MIT. The
  repository is github.com/arnaud-carre/LSPlayer.
- **What LSPConvert outputs:**
  - `X.lsmusic`: the score, which can go in any RAM;
  - `X.lsbank`: the samples, in chip RAM.
- **Modes:**
  - `-insane` adds generated code but leaves both files byte-identical to a
    standard conversion.
  - `-micro` writes an `LSPm` music file.
- **Other writers:** LSPX (github.com/dansalvato/lspx, MIT) converts OpenMPT
  formats to LSP1 version 1.25. Its code table is padded with zeros rather
  than unique dummy values. Apocalypso-invite's `apocalypso.lsmusic` comes
  from a converter of that kind: it uses getpos markers 0-255, not MOD
  positions.
- **No PC player reads LSP,** and conversion back to MOD is not meaningful:
  it is a per-tick sound-chip register stream.
- **The link between the files:** a 4-byte ID at music+04 is repeated at
  bank+00. Treat it as a random tag. The bank has no other header: it is
  raw 8-bit PCM.
- **`LSP1` headers** by converter version (the word at 08 is
  `major<<8 | minor`):

| Off | A: 1.00-1.03 | B: 1.04-1.05 | C: 1.10 | D: 1.11 onwards |
|---|---|---|---|---|
| 0A | BPM | BPM | flags | flags |
| 0C | n | escRewind | BPM | BPM |
| 0E | instruments | escSetBPM | escRewind | escRewind |
| 10 | | frames (u32) | escSetBPM | escSetBPM |
| 12 | | | frames (u32) | escGetPos |
| 14 | | n | | frames (u32) |
| 16 | | instruments | n | |
| 18 | | | instruments | [ADPCM block, from 1.30 when flag bit 2 is set] then n |
| 1A | | | | instruments (when there is no ADPCM block) |

- **After the header:**
  1. n × 12 instruments: u32 start, u16 length (words), u32 repeat,
     u16 repeat length. Offsets are relative to the bank start, 4 or more.
  2. u16 T + T words of code table.
  3. Layouts C and D only: u16 S + S × 8 sequence table.
  4. u32 W (word-stream size).
  5. Two u32 loop offsets.
  6. The word stream, then the byte stream. The byte stream's size is not
     stored.
- **Size:** walk the byte stream as that version's player does:

  ```
  A:     b=rd(); idx=b; if b==0 { b=rd(); idx=0x100|b; if b==0 { b=rd(); idx=0x200|b } }
  B/C/D: idx=0; ext=0; b=rd(); while b==0 { idx+=0x100; ext=1; b=rd() }; idx+=b
  require idx < T; v = table[idx]
  A:   v==FFFF ends; v==F00F skips 1 byte (BPM)
  B/C: escapes tested on every non-zero code; D: only when ext
       v==escRewind ends; v==escSetBPM or escGetPos skips 1 byte
  A:   q += popc(v>>8&15); words += popc(v>>4&15) + popc(v&15)
  BCD: q += popc(v>>4&15); words += popc(v&15) + voices with (v>>(8+2k))&2
  ```

  The end is the end of the stream. Require words × 2 == W and, for
  layouts B-D, frames == the header's frame count.
- **Micro (`LSPm`):**
  - Header: 04 version, 06 BPM (from 1.24), then n, n × 12 instruments, and
    16 u32 stream offsets.
  - Size = D + off[15] + (bytes with bit 5 set in the v3 command stream),
    where D = (version ≥ 0118 ? 0A : 08) + 12n + 40.
- **Bank size** = max over instruments of (start + 2 × length). This is
  exact unless the file was made with `-nosampleoptim`.
- **Checks:**
  - version 01xx;
  - BPM 1-255;
  - n 1-2600 (micro ≤ 256);
  - T ≤ 768, with table[0], [256] and [512] all 0;
  - S ≤ 128;
  - volume bytes ≤ 64;
  - instrument deltas land on valid entries.

  Odd sample offsets, repeat length 0 (micro, Planartunnel) and period 0 do
  occur, so don't reject them. **Every LSP player contains `0C98 'LSP1'`**,
  so expect one rejected candidate per copy of the player.
- **In memory:** init adds the bank address B to each instrument's start
  and repeat. The exact changes by player version:
  - 1.04-1.06 also set bit 0 of the start offset;
  - from 1.10, 0A := FF, and sequence entries get the word-stream address
    added;
  - the generated "insane" players leave the `.lsmusic` alone up to 1.10,
    and from 1.11 only set 0A := FF (they relocate their own copy of the
    instrument table);
  - micro players relocate the instruments, with no flag and no guard;
  - ADPCM banks (from 1.30) are decoded in place.

  To undo:
  - B = min(pointer) − 4;
  - or search for the ID to find the bank, skipping hits preceded by
    `20 3C`, the insane code's immediate.
- **Test files:**
  - The repository's `Example/` at 15 git revisions (layouts A-D).
  - Demo repositories (licences vary; only URLs are committed):
    Ozzyboshi/Planartunnel (micro), nattfalk/ins-gerp24,
    danamini/aura-tunnel-amiga, asimilon/AmigaDemoTemplate,
    ericde45/mega_JAG (1.05) and 14 more, listed in `tests/newformats/LSP.txt`.
  - The pouet list 200 has about 69 productions.

### Implementation (`src/formats/new/LSP.c`)

- **Detection:** `LSP1` with 01 at 08, or `LSPm` with 01 at 04; that byte
  already rejects the copies of the signature inside player code.
- **Checks, in addition to the walk above:**
  - flags high byte 00 or FF, and no bits above 2;
  - bit 2 (ADPCM) only from version 1.30, with 1 to 31 sample counts;
  - the escapes are distinct;
  - the rewind escape is among the last 4 table entries, and from 1.11 its
    index is 256 or more;
  - from 1.11, T ≥ 258;
  - W is even, and the loop offsets are inside their streams;
  - the sequence table:
    - on disk it starts with (0, W);
    - it is monotonic;
    - it stays inside the streams.
  - the instrument pointers end within 2 MB (chip RAM);
  - the ADPCM block:
    - its sample sizes add up to the bank size;
    - the in-place offset equals total minus packed size;
    - only the top bits of the lossless mask are used.
  - Micro:
    - the 16 offsets are in order;
    - the four command streams have equal length;
    - every volume, period and instrument stream has exactly the length
      its voice's command bits need;
    - bits 2-0 of command bytes are 0;
    - loop marks are only on voice 3: at most one "save" (10), and
      "restore" (11) on the last frame;
    - volumes are ≤ 64 and instrument numbers < n.
  - The getpos parameter is not checked (see LSPX above).
- **Restoring an initialised music** (saved as `restored`):

| Layout | Seen as relocated when | B |
|---|---|---|
| C, D | flag byte FF (cleared on save) | min(pointer) − 4 if that is ≥ 0x400 and even, else 0 (an insane player) |
| B | every start is odd | min(start − 1, repeat) − 4, if ≥ 0x400 and even |
| A, micro | min(pointer) − 4 ≥ 0x400 and even | that |

  Sequence entries lose the address of the word stream: their first entry
  is (0, W) on disk.
- **The bank:**
  - **LSP1:** the first input position holding the music's ID wins. It must
    not be inside the music, after `LSP1` (another copy of a music) or after
    `20 3C`.
  - **An earlier copy of the same music,** met first, has already searched
    the whole input, so its claimed bank is reused. The input is then
    scanned about once per distinct music, not once per copy.
  - **Otherwise,** for a relocated music: at B, as in a dump of memory from
    address 0. For lack of an ID, every one-shot sample (repeat length 0
    or 1) must start with two zero bytes there. LSPConvert clears them from
    1.01 on; this holds for all 518 such entries in the test banks.
  - **An unrelocated micro music** cannot find its bank; it is saved alone.
  - **An ADPCM bank of an initialised music** has been decoded in place.
    - It is packed again, nibble by nibble. Every decoded step is exactly
      reachable, so this gives back the original file.
    - Bytes that do not decode that way are saved as found, with a
      message.
- **Truncation:**
  - A music cut inside its header (up to the word-stream size) is not
    reported.
  - A music cut in its streams is reported as truncated:
    - its size is a lower bound when the byte stream is cut (one byte per
      remaining frame plus the rewind);
    - for micro, the size is exact once the command streams are complete.
- **Limits:**
  - **`-nosampleoptim`:** the bank size is a lower bound (unused trailing
    samples are not visible), and an unused first sample makes
    min(pointer) > 4. With the flag FF from an insane player, a first
    offset ≥ 0x404 would then be taken for a relocation.
  - **LSPX's experimental mixer mode:** it appends raw event bytes that no
    standard player reads, so those files are rejected.
  - **Version 1.04-1.05:** an odd original offset is not relocated by its
    player (bit 0 is already set), so such a mixed table is not restored.
  - **A bank whose ID is 0** is not searched for.
- **Coverage** (`tests/newformats/LSP.txt`, 52 entries):
  - 38 real files: the example at 15 revisions and 23 from demo
    repositories, 4 of them micro;
  - their 32 LSP1 banks are found and exact;
  - 14 synthetic files: layouts A-D, ADPCM, micro 1.20 and 1.33, and 7 memory
    images, made by `tests/newformats/files/LSP/make_synth.py`.

## Symphonie / Symphonie Pro

- **Origin:** Patrick Meng; `.SymMOD`. A `.SymSONG` has no samples (its
  instrument names are file paths).
- **Header:** 00 `SymM`, 04 u32 1, then the hunks. Each hunk starts with a
  signed u32 type, in one of four shapes:
  - 8-byte value hunks: −1 to −7, 10, 11, 12;
  - −12: just the 4-byte type (an empty sample);
  - block hunks: type + u32 length + data (−10, −11, −13 to −21);
  - 0: end of file.

  Unknown positive types are value hunks, unknown negative ones are blocks.
  Symphonie's `.SymMACRO` files also start with `SymM`, followed by
  `MCRO`.
- **Size:** walk the hunks.
  - Exact end: stop after N sample hunks (−11, −12, −17, −18).
  - N = (low word of −4) − the number of −14 records that the saver skips:
    a name set, a type other than "kill" (−4 at +80), and +84 == 2 (the
    right half of a stereo pair).
  - The −14 block may be RLE-packed (`PACK FF FF` + u32 length): decode it
    first.
- **Checks:**
  - `FFFFFFFF` + a channel count of 1-256 at 08 (usually a power of two);
  - track length ≤ 1024;
  - −5 low word = 4;
  - block lengths multiples of their record size.
- **References:** OpenMPT `Load_symmod.cpp` (BSD) is reusable. The original
  source on Aminet has no formal licence, so it is spec only. libxmp's
  `sym_load.c` is a different format (Archimedes Digital Symphony).
- **Test files:** Modland has 174.

### Implementation (`src/formats/new/Symphonie.c`)

- `pw_new_Symphonie`, at 'S' (after SidMon 2): `PWF_Symphonie`,
  "Symphonie module".
- **Signature:** `SymM`, version 1, then hunk −1 with 1-256 channels (12
  fixed bytes and a range).
- **Walk:** the order `SaveActualModule` writes:
  1. the value hunks: −2 must be 1-1024, −4 1-256 slots, and −5 low word 4;
  2. the song blocks −14, −10, −13 and −15, each once (−16, −19, −20 and
     −21 are optional). Their unpacked sizes are multiples of 256, 32, 4
     and 16. The instrument list −14 is decoded when RLE-packed (≤ 64 KiB),
     to count the stereo right halves;
  3. the sample hunks, until there are N of them.

  A type 0 hunk (end of file) also ends it. Anything else that is not a hunk
  of the current stage ends the walk, and the module must be complete.
- **Packing and delta coding** never change the size: block lengths count
  the stored bytes.
- **`.SymSONG` (decision):** it is ripped too, under the same ID, as
  "Symphonie song (samples not included)".
  - It is recognised by its −7 hunk. It has no sample hunks, so it ends
    after −15, optionally followed by −16, and by −20 + −19, as the saver
    writes them.
  - Its samples are separate files named in its instrument list. Nothing
    links them to the song in a binary, so they are not looked for.
  - The rip keeps the ID's extension, `symmod`. Symphonie opens a song
    with Load Project, whose file requester lists `*.SymSONG`, and then
    loads the samples by name (`LoadActualProject`, `AutoLoadSamples`).
    The module loader just skips the −7 value, so through Load Module the
    song plays without its samples.
- **Truncated modules:** a module cut by the end of the input is reported
  (TRUNCATED) once its instrument list has been read. When it is cut
  between hunks, at least the next hunk's 4 bytes count as missing.
- **Tests** (`tests/newformats/Symphonie.txt`):
  - 29 Modland files: their end is exact, whether their −4 slot count is
    128 (27 files) or 256 (2) and their −14 is packed (18) or not (11);
    5 of them skip 1-12 stereo right halves;
  - synthetic files (`tests/newformats/files/Symphonie/make_sym.py`): a
    packed module with raw, 8-bit delta and 16-bit delta samples, a stereo
    pair, empty and killed slots; an unpacked 4-slot module; and a
    `.SymSONG`.

## Startrekker AM (mod + `.nt`)

- **Origin:** Björn Wesen (Exolon/Fairlight), 1990. Audio Sculpture is the
  successor and uses `.as` files with the same layout.
- **The mod:** a 31-instrument MOD with `FLT4` (or `EXO4`) at 438. AM
  instruments have sample length 0 and volume 64.
- **Existing gaps:** prowiz rips FLT4 today, but `EXO4`, `FLT8` and `EXO8`
  are never triggered. FLT8 needs 1024 × (max(order) + 2) bytes of patterns
  for even orders (libxmp `flt_load.c`).
- **`.nt` header:** 000 is `ST1.2 ModuleINFO`, `ST1.3 ModuleINFO` or
  `AudioSculpture10`. 010-08F are settings: in all 49 Modland files 32 ×
  u16 below 100 (`0018`), 32 bytes `04` and 32 × `"0"`.
- **`.nt` records:** 090 + 78·i holds instrument i+1 for i < 31. Each
  record is `AM` + 4 zero bytes + parameters (waveform ≤ 3).
- **ST1.3 and AS only:** 16 macro slots × 110 bytes at F90 (`ABCD` at +10
  means empty).
- **`.nt` size** (corrected): 1008 for every `ST1.2` file, 2090 for every
  `ST1.3` and `AudioSculpture10` one.
  - Exact for 33 of the 49 Modland `.nt`/`.as` files (ST1.2 17/27, ST1.3
    12/18, AS 4/4).
  - The 5 ST1.2 files of 8336 bytes hold a second `ST1.2 ModuleINFO`
    block or editor memory after 1008; one more ST1.2 file and two ST1.3
    ones have a few bytes more; 3 ST1.3 files are 2 bytes short and 5
    files are cut (264-1824 bytes). Players accept cut copies.
- **Pairing:** nothing links the two files. Corrected: the `.nt`'s AM
  instruments are *not* always empty with a volume in the module (some
  have volume 0, a few a length of up to 32 words).
- **References** (reusable): libxmp `flt_load.c` (MIT), OpenMPT
  `Load_mod.cpp` (BSD), NostalgicPlayer ModTracker (MIT).
- **Test files:** Modland has 47 mods (44 with `.nt`), plus 5 Audio
  Sculpture pairs.

- **Implementation** (`src/formats/new/StartrekkerAM.c`):
  - **Companion** (`pw_new_StartrekkerAM_companion`): after the 1.70a
    handler ripped a FLT4 or EXO4 module (EXO4 is new, through the same
    1.70a test), every unclaimed `.nt`/`.as` in the input is looked at. It
    fits if it has at least one AM instrument, all of them have at most 32
    words in the module and at least one has length 0. The AM sets of most
    songs look alike, so where the file is decides: the nearest fitting
    `.nt` starting within 4 KB after the module, else the nearest one
    ending within 4 KB before it, else the only fitting one in the input.
    With the Modland files concatenated in name order this gives 38 right
    companions and 1 wrong one (`kingkeldon.mod`, whose own `.nt` has no
    AM instrument, takes the only other one that fits); scoring by AM sets
    instead paired fewer than half right. It is saved as `N.mod.nt`
    (`<mod extension>.nt`), cut to the input end when every AM record is
    still there. The IDs are located with `memchr()` inside the input
    buffer, so a big input with many FLT4 modules stays fast.
  - **Standalone** (`pw_new_StartrekkerNT`, ID 174, `.nt`): an ID the
    companion search did not claim, with the settings check above (it
    keeps the ID string inside the Startrekker program out) and valid AM
    records (`AM`, 4 zero bytes, waveform ≤ 3). The scan does not skip
    the `.nt` after saving it: with cut copies the next module can start
    inside its editor size (this hid 5 FLT4 modules in the concatenated
    Modland files). A `.nt` stored before its module is saved twice: on its
    own, then as the module's companion.
  - **8 channels** (`pw_new_Startrekker8`, ID 175, `.mod`): FLT8 / EXO8 at
    438, 31 instruments checked like the 1.70a MOD test, played orders
    even, note data checked in the played patterns only (the one Modland
    FLT8 file has garbage in unused ones). Size = 1084 + 1024 × 2 ×
    ((max order >> 1) + 1) + samples, as libxmp and OpenMPT load it
    (exact on that file). Its `.nt` is saved as companion too.
  - Tests (`tests/newformats/Startrekker.txt`): 9 synthetic lines (FLT4 +
    ST1.2, EXO4 + ST1.3, FLT4 + AS, FLT8 + ST1.3, EXO8, and the `.nt`
    files alone), 52 Modland modules (id 22; 25 with their `.nt` as
    companion; the others have a `.nt` of another size, without an AM
    instrument, or extra bytes), 41 Modland `.nt`/`.as` files alone (8 with
    `size=`; the 8 cut or short ones are not listed) and the FLT8 file.

## TFMX

- **Origin:** Chris Hülsbeck and Peter Thierolf. The variants are 1.x, Pro
  and 7V. Each comes as an `mdat` (song) plus an `smpl` (headerless 8-bit
  PCM).
- **Containers:** single-file containers from the PC era are exact:
  - TFHD: u32 header size at 04, mdat size at 0A, smpl size at 0E;
  - TFMXPAK.
- **mdat header** (200 bytes):

| Off | Content |
|---|---|
| 000 | `TFMX-SONG ` (once `TFMX-SONG` + 00), `tfmxsong` or `TFMX ` + `00 00 01` |
| 010 | 6 × 40 characters of text |
| 100/140/180 | 32 × u16 song start, end, tempo |
| 1D0/1D4/1D8 | u32 trackstep, pattern table and macro table offsets; all 0 means 800/400/600 |

- **mdat size:**
  - **Unpacked:** the first `07000000` at or after u32[7FC], + 4.
    Exact on 25/29.
  - **Packed:** pattern count = (macro table − pattern table)/4; size = macro
    table + 4 × (macro entries that are ascending and < pattern table).
    Exact on 44/45.
- **smpl size:** not stored, and nothing in the mdat points at it.
  - Estimate it as the max over all 128 macros of SetBegin (02/22) +
    2 × SetLen (03), or + the low word of 23.
  - This is a lower bound: exact on 53/74, with misses up to 47 KB.
- **Checks:**
  - Unpacked: 200-3FF and 1DC-1FF are zero; the tables ascend and are even.
  - Packed: trackstep < patterns ≤ macros (equal in SFX-only files without
    patterns).
  - Unpacked: the macro table need not ascend (`mdat.z-out`).
  - Songs: start ≤ end ≤ 1FF.
  - Macro command bytes < 40. Higher values mean Atari ST TFMX, which is
    out of scope.
- **In memory:**
  - The player keeps the mdat and smpl addresses in its own variables. The
    data offsets themselves are not relocated.
  - The first 4 smpl bytes are cleared.
  - SID macros write waveforms into the smpl.
  - Games often overwrite or drop the mdat header.
- **References** (reusable): libtfmxaudiodecoder (GPL-2+), NostalgicPlayer
  Tfmx (MIT, derived from it), UADE `amifilemagic.c`.
  - The libxmp `tfmx-format.txt` gets the fixed offsets wrong.
- **Test files:** Modland has 368 mdat/smpl pairs.

- **Checked on all 368 Modland pairs:** mdat size exact for 226/230 packed
  and 120/138 unpacked files; 13 files have 4-8268 bytes after the module
  and 9 unpacked ones are cut before their last macro ends. The smpl
  bound is exact for 277/368; 365/368 smpl files start with 4 zero bytes.
- **Implementation** (`src/formats/new/TFMX.c`; mdat: ID 179 `.mdat`,
  smpl: companion `N.smpl`; TFHD / TFMXPAK: ID 180 `.tfx`):
  - mdat checks: a tag; first song start ≤ end ≤ 1FF. Unpacked: 200-3FF
    zero, the pattern table ascending, even, from 800, the macro entries
    even and ≥ 800, a `07000000` within 64 KB of macro 127. Packed: 200 ≤
    trackstep < patterns ≤ macros < 1 MB, at most 128 patterns at even
    ascending offsets between trackstep and pattern table; the macro
    table ends at the first entry that is 0, odd, smaller than the one
    before (or the last pattern), or not below the pattern table. Mixed
    zero / non-zero offsets (little-endian PC conversions) are rejected.
  - An mdat cut by the end of the input is reported as truncated (at
    least macro 127 + 8 bytes).
  - smpl: only right after the mdat, ending at the macro bound; it must
    start with 4 zero bytes, fit in the input and contain no valid mdat
    (Modland's `smpl.z-out level 4` contains a stray header copy, which
    does not count). The scan goes on right after the mdat (the bound can
    reach a little past the real samples).
  - Names: "TFMX 7V module" (trackstep uses `EFFE 0003`), "TFMX 1.5
    module" (old tag, or unpacked with an `(Empty)` text), "TFMX Pro
    module".
  - TFHD: header size 12-400, version hint ≤ 3 (bit 7 free), byte 9 = 0;
    TFMXPAK: two decimal sizes and `>>>` within 32 bytes. Both need a
    valid mdat after the header; size = header + mdat + smpl sizes.
  - Not supported: an smpl anywhere else (the player keeps its address in
    its own variables), mdats without header, TFMX-MOD files.
  - Tests (`tests/newformats/TFMX.txt`): 7 synthetic files (unpacked Pro,
    old 1.5 tag, 7V, packed, `tfmxsong`, TFHD, TFMXPAK) and 359 Modland
    mdats: 267 with their smpl as companion (`gap=0`), 92 alone (13 with
    `size=`; the smpl is longer or shorter than the macros reach in 86);
    the 9 cut files are not listed.

## David Whittaker

- **What a file is:** raw game player code, data and samples; `DW.`.
  - There are many player variants, plus the old 1987 format `DWOLD`.
  - Other composers used the same routine: Rob Hubbard (1987-88), J.C.
    Brooke, Paul Robotham.
- **Prologues** (even offsets only; the Wanted Team player accepts the same):
  - A `48E7 F1FE 6100`, where the bsr.w target has `47FA` (lea x(pc),a3)
    within 10 words: 99 of 121 files;
  - B `08F9 0001 00BF E001` (bset #1,$bfe001), then `47FA` at 08 or 0A
    pointing at or before 00: 8;
  - C `47FA` pointing at most 8 bytes back, i.e. at the module start, after
    up to 4 words such as `7000`: 12 (`47FA FFFE` itself, `7000 47FA FFFC`,
    `303C 0000 47FA FFFA`, `48E7 F0F0 4240 47FA FFF8`);
  - a module that starts with `47FA` pointing further back, followed by
    `6100` or `4A2B` (Grimblood); not when it points at an A or B prologue
    (then it is the init code of that module).
- **Confirmation:** the sample set-up code, `41FA dddd [D1FC imm32 |
  2748 xxxx D0FC imm16 xxxx xxxx D0FC imm16] 4BFA dddd 72nn`, followed by
  `2418 3618` (the music samples) or `3428 0002 C4FC 0032` (a second set:
  the sound effects). The scan covers the first 16 KB of the module and
  stops at the first sample data seen; set-ups always come before the data
  (checked on all 121 files). The first music set-up whose records check
  out is the module's. A record: `[u32 len][u16 freq][data]`, len up to
  20000, freq 1000-60000 (the files use 3000-33500).
- **Size rule:**
  1. the end of the music samples;
  2. or the end of a second set that starts at or after that end and lies
     entirely in the input, if it ends later (the rips that include the
     effects stop exactly there);
  3. when the end comes from the music samples, plus 64 bytes if they are
     zero (85 of the 121 files end that way).
- **Checked:** 92/121 exact, 22 with size overrides, 7 not detected.
  - The overrides: a block of about 3.8 KB of waveforms that sits between
    the music samples and the effects, kept without the effects (5 files);
    unknown data after the samples (Krusty 46 KB, Snow Strike 11 KB, and 640
    bytes in 3 files); 64 non-zero bytes where the zeros would be (2); part
    of an effects set that runs past the file (Arcade Quizz); a few bytes of
    padding or junk (8).
  - Grimblood's samples end with the file: zero bytes after it in the input
    count as the 64-byte pad.
  - Not detected: the 1987 player (Feud, Leviathan and Xenon effects, QBall)
    and the effects-only files (Anarchy, Blood Money, Spellbound).
- **Start:** in some variants a variable area precedes the code: the
  `47FA` base is up to a few KB before the first instruction (Bubble Bobble
  −1090, Grimblood −906, Krusty −3284, Snow Strike −14). The rip starts at
  the first instruction, like the existing rips; the area is zeros on disk
  and live variables in memory, and is not saved.
- **In memory:** copies may be initialised, with absolute sample addresses
  written into the sample information table. The handler reads only the
  sample records, which the player does not change, so it rips such copies
  the same way.
- **Known limitation:** a module whose own set-up is not found (e.g. an
  effects-only module) would take the set-up of a module that follows
  within 16 KB, before that one's samples. No such case is known.
- **References:** NostalgicPlayer DavidWhittaker (MIT) is reusable. The UADE
  player is binary only; ExoticRipper is closed source.
- **Test files:** Modland has 121.

## Not researched in depth

These came up during the research and are worth a later look:

- **Jochen Hippel, COSO and 7V:** a `TFMX` tag reached through
  `lea x(pc)`; COSO has `COSO` at 0 and `TFMX` at 20. They look easier than
  Whittaker. Modland has about 280 files.
- **Martin Walker (Activision Pro):** a GPL EaglePlayer ripper exists.
- **Mark Cooksey:** a `601A` start.
- **Jason Page:** song and samples are separate files.
- **Custom modules (`CUST.`):** Amiga hunk executables with `DELIRIUM` and
  tag `80004455`. They are exact to size by walking the hunks, but they are
  hand-made, not found in games.

## Licences of reference code (prowiz is GPL-2)

| Reusable with its notice | Read as a spec only |
|---|---|
| libxmp (MIT), OpenMPT (BSD), NostalgicPlayer (MIT) | Wanted Team players and rippers ("do not assume open source") |
| LSPlayer (MIT), pretracker_parse.c and Raspberry Casket (MIT) | Flod / FlodJS (CC BY-NC-SA) |
| libdigibooster3 (BSD-2) | OctaMED replay and reloc code (free programs only) |
| libtfmxaudiodecoder (GPL-2+), EaglePlayer 2.04 rippers (GPL-2+) | Art of Noise and Symphonie sources (no licence) |
| UADE `amifilemagic.c` (GPL/PD), UADE Musicline sources (LGPL-2.1) | GPL-3 code: mega_JAG, DEViLBOX, ScummVM |
