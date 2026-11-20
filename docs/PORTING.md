# Porting notes

This port is a rewrite of Pro-Wizard for PC 1.70a (the last release by Sylvain
"Asle" Chipaux; source in `tests/reference/prowiz-1.70a-src.zip`) as a portable,
re-entrant C99 library plus a command line tool. The goal is that, for any
input, the port prints the same text and writes the same files as the original
Windows build. The only exceptions are inputs where the original read memory it
did not own (see "Undefined behaviour" below).

## Layout

| Original | Port |
| --- | --- |
| `prowiz.c` main loop | `src/scan.c` (converted mechanically; order of checks untouched) |
| `prowiz.c` `main()` | `cli/main.c` |
| `misc/misc.c`, `misc/testbag.c` | `src/save.c`, `src/core.c`, `src/formats/DietDataPacker.c` |
| `include/*.h` globals | fields of `pw_ctx` (`src/pw_internal.h`) |
| `r/<Name>.c` | `src/formats/<Name>.c` (same base name) |
| `_types_` handling | `pw_types_load()` in `src/core.c` |

Files of the original that its build never compiled (`NTK4.c`,
`TreasurePattern.c`, `Viruz2.c`, `DefjamCruncher32-pro.c`) and handlers its
scanner never calls (`AMF.c`, `SlamPacker.c`, `SGT-Packer.c`, `struggle.c`,
`sidmon2.c`, `Depack_QuadraComposer`) are not ported.

## Conventions for format handlers

**Names.** `testX()` → `int16_t pw_test_X(pw_ctx *c)`, `Rip_X()` →
`void pw_rip_X(pw_ctx *c)`, `Depack_X()` → `void pw_depack_X(pw_ctx *c)`. The
prototypes live in `src/formats/formats.h`.

**Globals.** Every global of the original is a `pw_ctx` field with the same
integer type (table in `src/pw_internal.h`): `PW_i` → `c->i`, `PW_j`..`PW_o` →
`c->j`..`c->o`, `PW_Start_Address` → `c->start`, `OutputSize` → `c->out_size`,
`PW_WholeSampleSize` → `c->wss`, `PW_in_size` → `c->in_size`, `Cpt_Filename` →
`c->file_counter`, `Save_Status` → `c->save_status`, `CONVERT` → `c->convert`,
`Amiga_EXE_Header` → `c->amiga_exe_header`, `Depacked_OutName` →
`c->depacked_name`. Handlers pass values from test to rip through these and
some leak into the next handler, so a global stays a field even when it looks
like a local. Locals of the original stay locals, **with their original
types**. Many tests depend on unsigned wrap-around (`uint32_t` arithmetic,
comparisons between `int32_t` and `uint32_t`), so keep the expression structure
and the types, and C will produce the same values.

**Input.** `in_data[x]` → `IN(x)` (returns 0 outside the input);
`in_data[x] = v` → `IN_SET(x, v)`. A pointer into `in_data` becomes an offset.

**Output files.** `PW_fopen(name, "w+b")` → `pw_fopen(c, name)`. The emulation
supports everything the handlers do with a `FILE`:

| Original | Port |
| --- | --- |
| `fwrite(buf, n, 1, out)` | `pw_fwrite(out, buf, n)` |
| `fwrite(&in_data[x], n, 1, out)` | `pw_fwrite_in(out, x, n)` (zero-fills past the input) |
| `fwrite(&u16, 2, 1, out)` / `fwrite(&u32, 4, 1, out)` | `pw_fput16le` / `pw_fput32le` (the reference build is little-endian x86) |
| `fwrite(&htonl_value, 4, 1, out)` | `pw_fput32be` |
| `fputc`, `fprintf`, `fseek`, `ftell`, `fread` | `pw_fputc`, `pw_fprintf`, `pw_fseek`, `pw_ftell`, `pw_fread` |
| `fclose(out)` | `pw_fclose(out)` (this hands the file to the host) |
| `fopen(OutName_final, "r+b")` (re-reading the rip) | `pw_fopen_last_rip(c)` |
| `Crap(...)`, `Crap15(...)` | `pw_crap(...)`, `pw_crap15(...)` |
| `Save_Rip`, `Save_Rip_Special` | `pw_save_rip`, `pw_save_rip_special` |

`sprintf(Depacked_OutName, "%d.mod", Cpt_Filename-1)` becomes the same call
with `snprintf(c->depacked_name, sizeof c->depacked_name, ...)`.

**Console text.** `printf` → `pw_printf(c, ...)`, with byte-identical text.
The original is a Windows build where `long` is 32 bits, so `%ld`/`%lu`
become `%d`/`%u` on the 32-bit value. `fflush(stdout)` and commented-out debug
output are dropped.

**Memory.** Scratch buffers are zero-initialised (fixed local arrays when the
size is a small constant, `calloc` otherwise). Variables the original left
uninitialised start at 0. Any index into a local or heap array that hostile
input can push out of range is guarded: reads give 0, writes are dropped. Add a
short comment where you guard something the original did not.

## Undefined behaviour

Where the original read past its own buffers, its output depended on whatever
was there. The port defines such bytes as 0. The reference binary used for
testing (`tests/reference/build_reference.sh`) is built with zeroed heap
allocations surrounded by 1 MiB of zeroed slack on each side and with
zero-initialised locals, so it behaves the same way for heap and input
over-reads and under-reads. Two cases still differ and
are left that way on purpose:

* reads past the end of a **local array** (the original picks up neighbouring
  stack variables), e.g. a note index beyond the 37-entry period table;
* writes past the end of a local array (the original corrupted its stack).

Neither happens with well-formed data: both need a corrupt file that passed the
format's tests.

Three related cases have a defined behaviour in the port:

* **Memory around the input.** Some rippers patch bytes past the end of the
  input before they notice the rip does not fit, and a later test may read
  them back. The port treats the space around the input as zero-filled and
  writable: such writes are remembered (`pw_outside_set` in `src/core.c`) and
  `IN()` returns them.
* **Impossible writes.** A garbage size, or a negative `int` handed to
  `fwrite`, asks for gigabytes. The C library of the original's Windows build
  first fills the rest of its 4096-byte stdio buffer from the source, flushes
  it, and only then fails on the remainder; later writes work. `src/file.c`
  reproduces exactly that for any write over 256 MiB. A real case: TetraPack
  2.2 found through its 0x61 signature always has a size of about 3 GB, and
  its rip is the rebuilt header, the input up to the 4096-byte boundary, and
  the closing hunk bytes, as in the original.
* **Where the original never finishes.** A few handlers move the scan position
  backwards on odd sizes, so the original rips the same thing forever (or a
  test walks in a cycle). The port keeps the scan position in those cases
  (the rip in question is saved once) and carries on scanning; each guard is
  commented as "Hang guard (not in the original)". Up to that point the
  output is identical.

## Inherited quirks kept on purpose

* The built-in extension of StoneCracker 2.70 is `"StoneCracker270 "`, with a
  trailing space, as in the original's table. Windows drops trailing spaces
  from file names, so there the file is `0.StoneCracker270`; on other systems
  the space stays, exactly as the original behaved there. A `_types_` file
  can override it.
* Several tests use values a previous handler left in the shared registers
  (`c->start`, `c->wss`, ...), so whether something is found can depend on
  what was tested before it at the same position. This is the original's
  behaviour and is kept.

## Testing

```sh
make                                          # build/prowiz, build/libprowiz.a
sh tests/reference/build_reference.sh         # build/reference/prowiz-ref
python tests/difftest.py --ref build/reference/prowiz-ref --port build/prowiz FILES/DIRS
python tests/fuzzdiff.py --ref build/reference/prowiz-ref --port build/prowiz --count 300 SEEDS
```

`difftest.py` runs both programs on each input in empty directories and
compares stdout and all output files. Inputs listed in
`tests/corpus/known-differences.txt` (deliberate differences, see above) are
reported as KNOWN without failing the run; `tests/corpus/platform-dependent.txt`
lists inputs to skip when the reference is built against a C library other
than the original's Windows one. `tests/run_difftest.sh` uses both. `fuzzdiff.py` does the same for mutants
of seed files. `tests/corpus/synthetic/` holds small hand-made inputs for
formats without public samples; `tests/fetch_corpus.py` downloads real
modules (the libxmp test suite) for wider coverage.
