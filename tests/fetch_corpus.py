#!/usr/bin/env python3
"""
fetch_corpus.py - download real-world test modules for difftest.py.

Fetches the module samples of the libxmp test suite
(https://github.com/libxmp/libxmp, test-dev/data) - many of them packed with
the formats Pro-Wizard handles - into tests/corpus/libxmp/ (not committed).

  python tests/fetch_corpus.py [destination]
"""
import concurrent.futures as cf
import json
import lzma
import os
import re
import sys
import urllib.parse
import urllib.request

TREE = 'https://api.github.com/repos/libxmp/libxmp/git/trees/master?recursive=1'
RAW = 'https://raw.githubusercontent.com/libxmp/libxmp/master/'

SKIP_EXT = re.compile(r'\.(it|s3m|data|wav|raw|ogg|flac|gz|zip|rar|bz2|lha|lzx|j2b|mtm|ptm|ult|far|gdm|imf|'
                      r'stm|stx|dtm|dbm|psm|669|rtm|mdl|fnk|xmf|liq|amd|rad|ims|mfp|wow|sym|dsym|oxm|dmf)$')
SKIP_NAME = re.compile(r'^(arc|l[012]_|lzx|bzip|compress|gzip|xz|zip|j2b|beep|blip|buzz|adlib|test\.mmcmp)')
FUZZ_KEEP = re.compile(r'load_(mod|st|ice|chip|stim|flt|hmn|digi|okt|mmd|med|abk|emod|sfx)_|'
                       r'depack_pp20|depack_s404|depack_sqsh')


def wanted(entry):
    if entry['type'] != 'blob':
        return False
    p = entry['path']
    if not p.startswith('test-dev/data/'):
        return False
    if p.startswith('test-dev/data/f/'):
        return bool(FUZZ_KEEP.search(p))
    name = p.rsplit('/', 1)[-1].lower()
    if SKIP_EXT.search(name) or SKIP_NAME.search(name):
        return False
    return entry.get('size', 0) <= 600000


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    dst = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, 'corpus', 'libxmp')
    os.makedirs(dst, exist_ok=True)
    req = urllib.request.Request(TREE, headers={'User-Agent': 'prowiz-test-fetch'})
    tree = json.load(urllib.request.urlopen(req, timeout=60))
    paths = [e['path'] for e in tree['tree'] if wanted(e)]

    def get(p):
        out = os.path.join(dst, p[len('test-dev/data/'):].replace('/', '__'))
        if os.path.exists(out) or os.path.exists(out[:-3]):
            return None
        try:
            data = urllib.request.urlopen(RAW + urllib.parse.quote(p), timeout=60).read()
            if out.endswith('.xz'):
                data, out = lzma.decompress(data), out[:-3]
            with open(out, 'wb') as f:
                f.write(data)
            return None
        except Exception as e:  # noqa: BLE001 - report and continue
            return f'{p}: {e}'

    with cf.ThreadPoolExecutor(8) as ex:
        errors = [r for r in ex.map(get, paths) if r]
    print(f'{len(paths)} files in {dst}, {len(errors)} errors')
    for e in errors:
        print('  ' + e)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
