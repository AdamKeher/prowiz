#!/bin/sh
# Builds the original Pro-Wizard for PC 1.70a (tests/reference/prowiz-1.70a-src.zip)
# as an instrumented reference binary for tests/difftest.py.
#
#   tests/reference/build_reference.sh [output-dir]      (default: build/reference)
#
# Needs: sh, unzip, a C compiler (CC, default cc) that knows -ftrivial-auto-var-init.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
out=${1:-build/reference}
CC=${CC:-cc}
zip="$here/prowiz-1.70a-src.zip"

( cd "$here" && sha256sum -c prowiz-1.70a-src.zip.sha256 >/dev/null ) || {
    echo "checksum mismatch for $zip" >&2; exit 1; }

mkdir -p "$out"
out=$(cd "$out" && pwd)
src="$out/src"
rm -rf "$src"
mkdir -p "$src"
unzip -q "$zip" -d "$src"

# the source list of the official Windows build (Makefile.win)
objs=$(sed -n 's/^OBJ *= *//p' "$src/Makefile.win" | tr ' ' '\n' | grep '\.o$' | sed 's/\.o$/.c/')
files=""
for f in $objs; do
    [ "$f" = "r/BNR.c" ] && f="r/bnr.c"   # file name case differs in the archive
    files="$files $src/$f"
done

exe=""
case "$(uname -s 2>/dev/null)" in MINGW*|MSYS*|CYGWIN*) exe=".exe";; esac

# -DINCLUDEALL: all crunchers, as in the official build.
# -O0 -fwrapv: take the code literally (it relies on signed wrap-around).
$CC -O0 -w -fwrapv -ftrivial-auto-var-init=zero -DINCLUDEALL \
    -include "$here/refshim.h" -I"$src/include" $files -o "$out/prowiz-ref$exe"
echo "$out/prowiz-ref$exe"
