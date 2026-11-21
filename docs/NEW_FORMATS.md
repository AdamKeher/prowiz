# New formats: research notes

These are formats Pro-Wizard 1.70a never had, researched on 2026-11-21 before
adding them to prowiz. Plan: the new formats are on by default, and a command
line flag restores the 1.70a set (and so its exact output, which the
difftest/fuzzdiff comparisons keep checking).

The research used each format's own loader and replayer sources. A **Checked**
figure means a script ran the size algorithm on real files (mostly from
Modland, plus each tool's own examples) and the computed end matched the file
size. Offsets are hex and relative to the module start; all fields are
big-endian.

## Summary

| Format | Signature | Files | Size from | Checked | In memory dumps | Verdict |
|---|---|---|---|---|---|---|
| OctaMED MMD2/MMD3 | `MMD2`/`MMD3` at 0 | 1 | `modlen` at 04 | 15/15 | pointers relocated (as for MMD0/1 today) | easy (file images) |
| PreTracker | `PRT` + version byte | 1 | header walk | 83/83 | verbatim | easy |
| SidMon 2 | `SIDMON II - THE MIDI VERSION` at 3A | 1 | header sums + sample table | 101/107 | intact; sample bytes may be negated | easy |
| Delta Music 2 | `.FNL` at BC6 | 1 (contains its player) | sample length table | 93/98 | player globals, waveform 0 change | easy |
| Art of Noise | `AON4`/`AON8` + 42-byte string | 1 | chunk walk | 8/8 | verbatim | easy |
| DigiBooster Pro | `DBM0` + version 2/3 | 1 | chunk walk | 4/4 | rare (loaded into structures) | easy |
| Musicline Editor | `MLEDMODL` | 1 | chunk walk | 4/4 | rare (loaded into structures) | easy |
| Rob Hubbard | 5 × `bra.w` + code | 1 (player code + data) | sample walk to `4E71` | 12/12 | - | easy |
| Ben Daglish | `bra.w` table + init code | 1 (player code + data) | instrument tables | 28/44 exact, 33/44 within 34 bytes | offsets, probably intact | easy-moderate |
| LightSpeedPlayer | `LSP1` / `LSPm` | 2 (music + bank) | stream walk; bank from instrument table | 34/34 music, 32/32 banks | relocated, can be undone | moderate |
| Symphonie | `SymM` + version 1 | 1 (`.SymSONG`: plus samples) | hunk walk + slot count | 4/4 | rare | moderate |
| Startrekker AM | `.nt`: `ST1.x ModuleINFO` | 2 (mod + `.nt`) | `.nt` sizes are near fixed | 44 `.nt` | no link between the two | moderate |
| TFMX | `TFMX-SONG ` and others | 2 (mdat + smpl) | mdat: walk; smpl: lower bound | mdat 69/74, smpl 53/74 | smpl found only via player variables | hard |
| David Whittaker | 3 code prologues | 1 (player code + data) | heuristic: samples + 64 | 88/121 | initialised copies | moderate-hard |

Packed productions: LSP and PreTracker songs in released demos and intros are
almost always inside crunched executables (Shrinkler, Cranker, LDOS packers).
They will mostly be found in memory dumps, decrunched files and plain data
files.

## Work shared by several formats

1. **The 1.70a-only flag.** This is a CLI option plus a field in the library
   options. New handlers run only when the flag is off. difftest.py and
   fuzzdiff.py pass the flag to the port.
2. **Several files from one finding.**
   - LSP needs `N.lsmusic` + `N.lsbank`.
   - TFMX needs `mdat.N` + `smpl.N`.
   - Startrekker AM needs `N.mod` + `N.mod.nt`.

   File names are now always `"%d.%s"` (`save_prologue`), so this needs a
   companion-file role in `pw_file_role`. The JSON `files` list already
   handles several files.
3. **Undoing relocation in memory images.**
   - **Who needs it:**
     - LSP: instrument pointers and the flag at 0A.
     - MMD0-MMD3: the whole pointer graph.
     - PreTracker: the Raspberry Casket header patch.
   - **What it takes:** inferring the load address (the "base"), which is
     simply the file offset in a flat chip-RAM dump starting at address 0.
   - **Bonus:** for MMD this also fixes an existing 1.70a limitation, where
     relocated MMD0/1 modules are rejected or saved still relocated.
4. **Test files.** Modland files cannot be committed.
   - Fetch a pinned list into the git-ignored `tests/corpus/`, like
     `tests/fetch_corpus.py` does.
   - Commit small synthetic files.
   - Per format, check that:
     - each real file is found standalone, at the exact size;
     - it is found again at odd and even offsets inside noise;
     - nothing is found in the existing corpus.

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
- **Differences from MMD0/1 that matter for a test:**
  - The song structure (MMD2song, 314 bytes) is not at 34. The playseq,
    section and track-volume tables come first; the song was found between
    6E and F8.
  - MMD2song fields: +1F8 numblocks, +1FA songlen, and pointers at +1FC,
    +200, +204 and +20C.
  - Blocks use the MMD1 layout.
  - `mmdflags` (14) bit 0 is set in MMD3 files.
- **Upstream support was started:** the original has `'2'`/`'3'` commented
  out of its `MMD` check (`src/scan.c` ~1693). `pw_rip_MMD0` saves only IDs
  ending in `0`/`1` (`src/formats/MED-Octamed.c` ~59). `pw_test_MMD0`
  already passes every MMD2/MMD3 file examined.
- **To add it:** enable the IDs, give them names and format IDs, and tighten
  the test:
  - `modlen` even and at least 348;
  - `blockarr`, `smplarr` and `expdata` either 0 or even and inside
    [34, modlen);
  - song + 314 ≤ modlen;
  - numblocks ≥ 1, numsamples ≤ 63;
  - the word at 30 is FFFF in files.
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
| 03 | 1 | version: 18-19 (pre-0.87), 1A-1B (0.87-1.02), 1E (1.5); 1C/1D never seen |
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
  table and the code prefix.
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

## Art of Noise

- **Origin:** Bastian Spiegel (Twice/Lego), 1993-96; `.aon`.
- **Variants:**
  - AON4: 4 channels, patterns 400 bytes.
  - AON8: 8 software-mixed channels, patterns 800 bytes.
- **Header:**
  - 00 `AON4`/`AON8`.
  - 04: 42 bytes, either `artofnoise by bastian spiegel (twice/lego)` or
    `ArtOfNoise by Bastian Spiegel(twice/lego)` followed by a NUL byte.
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
  - the string, case-insensitively;
  - every chunk ID is known;
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
- **Test files:** Modland has 211.

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
- **Test files:** Modland has 846, and libxmp test-dev about 14.

## Musicline Editor

- **Origin:** the Cyréus brothers and others, 1993-95; `.ml`.
- **Header:** 00 `MLED`, 04 `MODL`, 08 u32 length (4), then `VERS`.
  Very old files have `VERS` at 08.
- **Chunks** (ID + u32 length, unaligned): TUNE, PART, ARPG, INST, SMPL,
  INFO.
  - **SMPL** takes 8 + 6 + length bytes: the stored length leaves out a
    6-byte prefix.
- **Size:**
  1. Skip VERS.
  2. Walk the chunks until an unknown ID.
- **Checks:**
  - the first chunk after VERS is TUNE;
  - TUNE: channels ≤ 8, play mode 0/1, length = 28 + 4·channels + Σ
    channel sizes;
  - PART number < 1024;
  - ARPG (len − 2) % 6 == 0.
- **Packing:** samples can be delta-packed and parts are packed, but the
  stored lengths cover both.
- **References:** NostalgicPlayer MusiclineEditor (MIT) is reusable. The UADE
  deliplayer and format doc are LGPL-2.1 (68k assembly, a spec in practice).
- **Test files:** Modland has 817.

## Rob Hubbard

- **What a file is:** raw game player code followed by song data and
  samples; Modland `RH.`/`.rh`.
- **Signature:**
  - `6000` at 00, 04, 08, 0C and 10;
  - `41FA` at 14;
  - `4E75 41FA` at 1C.
- **Size** (exact on 12/12):
  1. The sample count is the byte before the first `2418` within 8 words
     of 40.
  2. The samples start at the target of the `41FA` within 5 words of 36,
     plus 40 if `D1FC` follows.
  3. Walk the `[u32 len][u16 freq][data]` records. Reject len > 10000.
  4. Add 2 for the closing `4E71`, which must be present.
- **Out of scope:**
  - sound-effect banks (4 × `bra.w`);
  - tunes with separate samples;
  - Rob Hubbard ST (`RHO.`), Steve Bak's routine, which has a heuristic end.
- **References:** the Wanted Team EagleRipper and player are not open
  source, so they are spec only. Flod is CC BY-NC-SA, also spec only.
- **Test files:** Modland has 14 + 3 RHO.

## Ben Daglish

- **What a file is:** raw, position-independent code + data; `BD.`/`.bd`.
- **Signature:**
  - `6000` at 00, 04 and 0A, each with a positive even displacement;
  - at the init target: `3F00 6100`, `3D7C` at +6, `41FA` at +C.
- **Size:**
  1. Find the instrument table(s) via `D040 D040 D040 41FA`.
  2. End = table base + max(sample offset + 2 × (length [+ loop length])).
  3. Bound it by the input: Super Cars' second table points past the
     file.
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
  B/C: escapes tested on every code; D: only when ext
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

  Odd sample offsets and period 0 do occur, so don't reject them. **Every
  LSP player contains `0C98 'LSP1'`**, so expect one rejected candidate per
  copy of the player.
- **In memory:** init adds the bank address B to each instrument's start
  and repeat. The exact changes by player version:
  - 1.04-1.06 also set bit 0 of the start offset;
  - from 1.10, 0A := FF, and sequence entries get the word-stream address
    added;
  - ADPCM banks (from 1.30) are decoded in place.

  To undo:
  - B = min(pointer) − 4;
  - or search for the ID to find the bank, skipping hits preceded by
    `20 3C`, the insane code's immediate.
- **Rip plan:**
  - Save `N.lsmusic`, un-relocated if needed.
  - Save `N.lsbank`, found via the ID (`LSP1`) or via the pointers (micro).
  - Write two files from one finding.
- **Test files:**
  - The repository's `Example/` at 15 git revisions (layouts A-D).
  - Game and demo repos with MIT/GPL licences: Ozzyboshi/Planartunnel
    (micro), nattfalk/ins-gerp24, danamini/aura-tunnel-amiga,
    asimilon/AmigaDemoTemplate.
  - The pouet list 200 has about 69 productions.

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
- **Size:** walk the hunks.
  - Exact end: stop after N sample hunks (−11, −12, −17, −18).
  - N = (low word of −4) − the number of −14 records with byte +84 == 2
    (StereoR slots).
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

## Startrekker AM (mod + `.nt`)

- **Origin:** Björn Wesen (Exolon/Fairlight), 1990. Audio Sculpture is the
  successor and uses `.as` files with the same layout.
- **The mod:** a 31-instrument MOD with `FLT4` (or `EXO4`) at 438. AM
  instruments have sample length 0 and volume 64.
- **Existing gaps:** prowiz rips FLT4 today, but `EXO4`, `FLT8` and `EXO8`
  are never triggered. FLT8 needs 1024 × (max(order) + 2) bytes of patterns
  for even orders (libxmp `flt_load.c`).
- **`.nt` header:** 000 is `ST1.2 ModuleINFO`, `ST1.3 ModuleINFO` or
  `AudioSculpture10`.
- **`.nt` records:** 090 + 78·i holds instrument i+1 for i < 31. Each
  record is `AM` + 4 zero bytes + parameters (waveform ≤ 3).
- **ST1.3 and AS only:** 16 macro slots × 110 bytes at F90 (`ABCD` at +10
  means empty).
- **`.nt` size:**
  - Use 2090 for `AudioSculpture10`, or for `ST1.3` with macros.
  - Otherwise use 1008 if the input has room.
  - At least 090 + 78 × (highest AM index) bytes are needed.
  - Truncated copies exist and players accept them.
- **Pairing:** nothing links the two files. The `.nt`'s AM indices must be
  a subset of the mod's instruments with length 0 and volume > 0.
- **References** (reusable): libxmp `flt_load.c` (MIT), OpenMPT
  `Load_mod.cpp` (BSD), NostalgicPlayer ModTracker (MIT).
- **Test files:** Modland has 47 mods (44 with `.nt`), plus 5 Audio
  Sculpture pairs.

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
| 000 | `TFMX-SONG `, `tfmxsong` or `TFMX ` + `00 00 01` |
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
  - Packed: trackstep < patterns < macros.
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

## David Whittaker

- **What a file is:** raw game player code, data and samples; `DW.`.
  - There are many player variants, plus the old 1987 format `DWOLD`.
  - Other composers used the same routine.
- **Prologues:**
  - `48E7 F1FE 6100`: 99 of 121 files;
  - `47FA` with a target at the module start: 9;
  - `08F9 0001 00BF E001`: 8.

  Confirm with a NostalgicPlayer-style scan of the init code.
- **End:** the end of the `[u32 len][u16 freq][data]` sample chain + 64 zero
  bytes. That is exact for 88/121 files; 24 have extra waveform data after
  the chain.
- **Start:** in some variants a variable area precedes the code (−1090
  bytes in Bubble Bobble).
- **In memory:** copies may be initialised, with absolute sample addresses
  written into the sample table.
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
