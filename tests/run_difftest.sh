#!/bin/sh
# Build the port and the original (reference), then compare them on every
# test input: the committed synthetic corpus and the libxmp samples (fetched
# on first use).
#
#   sh tests/run_difftest.sh [extra difftest.py options]
set -eu
cd "$(dirname "$0")/.."

make -j4 >/dev/null
sh tests/reference/build_reference.sh build/reference >/dev/null

exe=""
[ -f build/prowiz.exe ] && exe=".exe"
py=python3
$py -c "" >/dev/null 2>&1 || py=python   # Windows may have a dead python3 alias

[ -d tests/corpus/libxmp ] || $py tests/fetch_corpus.py

# off Windows the reference's C library differs for a few inputs (see the file)
skip=""
[ -z "$exe" ] && skip="--skip-list tests/corpus/platform-dependent.txt"

$py tests/difftest.py --ref "build/reference/prowiz-ref$exe" --port "build/prowiz$exe" \
    --quiet $skip --known-list tests/corpus/known-differences.txt "$@" tests/corpus
