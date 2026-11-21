#!/usr/bin/env python3
"""
newformats.py - tests of the formats Pro-Wizard 1.70a never had.

Each line of a list file (default: tests/newformats/*.txt; '#' starts a
comment) names one module file and what prowiz must make of it:

  id=<format id> file=<sha256>:<source> [size=<n>] [rip=<sha256>]
      [companion=<ext>:<sha256>:<source>]... [restored=1] [alone=0]

  source     a path relative to the repository (small committed files) or an
             http(s) URL, downloaded once into the corpus directory and
             checked against the sha256
  size       expected size of the rip (default: the file size)
  rip        sha256 of the expected rip file (default: the first 'size'
             bytes of the file)
  companion  a file that must be saved as a companion of the rip, named
             "<index>.<ext>"
  restored   the finding must be flagged "restored" (in-memory copies)
  alone=0    skip the "alone" check (the module needs its companion to be
             recognised at all)
  gap=<n>    bytes of noise between the module and its first companion in
             the "embedded" check (default 778; 0 for companions that can
             only be found right after the module)

Each module is scanned twice with "prowiz -j":
  alone     the file followed by 128 zero bytes: a finding of that format at
            offset 0, with the expected rip
  embedded  noise, the file, noise, then each companion behind more noise:
            the finding at the file's offset, the expected rip and every
            companion file. A finding of a new format inside the noise fails
            too (a false positive).
Noise is pseudo-random (seeded by the sha256) and every offset is even.

Options:
  --port EXE      the prowiz binary to test (required)
  --corpus DIR    download cache (default: tests/newformats/cache, git-ignored)
  --offline       skip entries whose files are not in the cache or the repo
  --import DIR    first copy every file under DIR whose sha256 a list names
                  into the cache (for files you already have)
  --only TEXT     only entries whose line contains TEXT
  --negative DIR  also scan every file under DIR and list the findings of
                  new formats there, for review (they do not fail the run)
  --jobs N        parallel runs (default: CPU count)
Exit status: 0 if every check passed.
"""
import argparse
import concurrent.futures as cf
import glob
import hashlib
import json
import os
import random
import shutil
import subprocess
import sys
import tempfile
import urllib.parse
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
FIRST_NEW_ID = 168          # pw_format_count_170a()
NEW_IDS_BELOW = {146}       # SidMon 2: an ID 1.70a reserved but never used
TIMEOUT = 60


def is_new(fid):
    return fid >= FIRST_NEW_ID or fid in NEW_IDS_BELOW


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def parse_line(line, where):
    e = {'companions': [], 'where': where, 'line': line, 'alone': True, 'restored': False,
         'gap': 778}
    for tok in line.split():
        if '=' not in tok:
            raise ValueError(f'{where}: bad token {tok!r}')
        k, v = tok.split('=', 1)
        if k == 'id':
            e['id'] = int(v)
        elif k == 'file':
            h, src = v.split(':', 1)
            e['sha'], e['src'] = h.lower(), src
        elif k == 'size':
            e['size'] = int(v)
        elif k == 'rip':
            e['rip'] = v.lower()
        elif k == 'companion':
            ext, h, src = v.split(':', 2)
            e['companions'].append({'ext': ext, 'sha': h.lower(), 'src': src})
        elif k == 'restored':
            e['restored'] = v == '1'
        elif k == 'alone':
            e['alone'] = v != '0'
        elif k == 'gap':
            e['gap'] = int(v)
        else:
            raise ValueError(f'{where}: unknown key {k!r}')
    if 'id' not in e or 'sha' not in e:
        raise ValueError(f'{where}: needs id= and file=')
    return e


def read_lists(paths):
    entries = []
    for p in paths:
        with open(p, encoding='utf-8') as f:
            for n, line in enumerate(f, 1):
                line = line.split('#', 1)[0].strip()
                if line:
                    entries.append(parse_line(line, f'{os.path.relpath(p, ROOT)}:{n}'))
    return entries


def cache_name(sha, src):
    base = os.path.basename(urllib.parse.unquote(urllib.parse.urlparse(src).path)) or 'file'
    base = ''.join(ch if ch.isalnum() or ch in '._-' else '_' for ch in base)
    return f'{sha[:16]}-{base}'


class Corpus:
    def __init__(self, cache, offline):
        self.cache = cache
        self.offline = offline

    def get(self, sha, src):
        """bytes of a file, or None if offline and not available"""
        if not src.startswith(('http://', 'https://')):
            path = os.path.join(ROOT, src)
        else:
            path = os.path.join(self.cache, cache_name(sha, src))
            if not os.path.exists(path):
                if self.offline:
                    return None
                os.makedirs(self.cache, exist_ok=True)
                req = urllib.request.Request(src, headers={'User-Agent': 'prowiz-tests'})
                with urllib.request.urlopen(req, timeout=120) as r:
                    data = r.read()
                if sha256(data) != sha:
                    raise RuntimeError(f'sha256 mismatch for {src}')
                with open(path + '.part', 'wb') as f:
                    f.write(data)
                os.replace(path + '.part', path)
        with open(path, 'rb') as f:
            data = f.read()
        if sha256(data) != sha:
            raise RuntimeError(f'sha256 mismatch for {path}')
        return data


def import_files(entries, src_dir, cache):
    wanted = {}
    for e in entries:
        for sha, src in [(e['sha'], e['src'])] + [(c['sha'], c['src']) for c in e['companions']]:
            if src.startswith(('http://', 'https://')):
                wanted[sha] = cache_name(sha, src)
    os.makedirs(cache, exist_ok=True)
    n = 0
    for dirpath, _, names in os.walk(src_dir):
        for name in names:
            p = os.path.join(dirpath, name)
            try:
                with open(p, 'rb') as f:
                    h = sha256(f.read())
            except OSError:
                continue
            if h in wanted and not os.path.exists(os.path.join(cache, wanted[h])):
                shutil.copyfile(p, os.path.join(cache, wanted[h]))
                n += 1
    return n


def noise(rng, n):
    return bytes(rng.getrandbits(8) for _ in range(n))


def run_port(port, data, outdir):
    """prowiz -j on 'data'; returns (report, {file name: bytes})"""
    inp = os.path.join(outdir, 'input.bin')
    with open(inp, 'wb') as f:
        f.write(data)
    out = os.path.join(outdir, 'out')
    os.makedirs(out)
    p = subprocess.run([port, '-j', '-o', out, inp], capture_output=True, timeout=TIMEOUT)
    rep = json.loads(p.stdout.decode('utf-8', 'replace'))
    files = {}
    for name in os.listdir(out):
        with open(os.path.join(out, name), 'rb') as f:
            files[name] = f.read()
    return rep, files


def check_finding(e, rep, files, offset, expect_rip, companions, problems, label):
    want = [f for f in rep['findings'] if f['format_id'] == e['id'] and f['offset'] == offset]
    if not want:
        got = [(f['format_id'], f['offset'], f['size'], f['status']) for f in rep['findings']
               if is_new(f['format_id'])]
        problems.append(f'{label}: no finding id {e["id"]} at {offset} (new-format findings: {got})')
        return
    f = want[0]
    if f['status'] != 'saved':
        problems.append(f'{label}: status {f["status"]} (size {f["size"]}, missing {f.get("missing")})')
        return
    if f['size'] != e['size']:
        problems.append(f'{label}: size {f["size"]}, expected {e["size"]}')
    if e['restored'] != bool(f.get('restored')):
        problems.append(f'{label}: restored is {f.get("restored")}')
    rips = [x for x in f['files'] if x['role'] == 'rip']
    if len(rips) != 1:
        problems.append(f'{label}: {len(rips)} rip files')
    else:
        got = files.get(rips[0]['name'])
        if got is None or sha256(got) != expect_rip:
            problems.append(f'{label}: rip file {rips[0]["name"]} differs from the expected bytes')
    comps = [x for x in f['files'] if x['role'] == 'companion']
    for cmp in companions:
        name = f'{f["index"]}.{cmp["ext"]}'
        match = [x for x in comps if x['name'] == name]
        if not match:
            problems.append(f'{label}: no companion {name} (companions: {[x["name"] for x in comps]})')
        elif sha256(files.get(name, b'')) != cmp['sha']:
            problems.append(f'{label}: companion {name} differs from the expected bytes')


def test_entry(args, corpus, e):
    """returns (entry, problems or None if skipped)"""
    data = corpus.get(e['sha'], e['src'])
    comps = []
    for cmp in e['companions']:
        d = corpus.get(cmp['sha'], cmp['src'])
        if d is None:
            data = None
            break
        comps.append(d)
    if data is None:
        return e, None
    e.setdefault('size', len(data))
    expect_rip = e.get('rip') or sha256(data[:e['size']])
    problems = []

    if e['alone']:
        with tempfile.TemporaryDirectory(prefix='pwnew-') as tmp:
            rep, files = run_port(args.port, data + bytes(128), tmp)
            check_finding(e, rep, files, 0, expect_rip, [], problems, 'alone')

    rng = random.Random(e['sha'])
    parts = [noise(rng, 1000)]
    offset = 1000
    parts.append(data)
    pos = offset + len(data)
    pad = (e['gap'] + (pos & 1)) if comps else 778 + (pos & 1)
    parts.append(noise(rng, pad))
    pos += pad
    noise_ranges = [(0, offset), (offset + len(data), pos)]
    for d in comps:
        parts.append(d)
        pos += len(d)
        pad = 778 + (pos & 1)
        parts.append(noise(rng, pad))
        noise_ranges.append((pos, pos + pad))
        pos += pad
    blob = b''.join(parts)
    with tempfile.TemporaryDirectory(prefix='pwnew-') as tmp:
        rep, files = run_port(args.port, blob, tmp)
        check_finding(e, rep, files, offset, expect_rip, e['companions'], problems, 'embedded')
        for f in rep['findings']:
            if is_new(f['format_id']) and any(a <= f['offset'] < b for a, b in noise_ranges):
                problems.append(f'embedded: false positive in noise: id {f["format_id"]} '
                                f'"{f["format"]}" at {f["offset"]}')
    return e, problems


def negative(args, path):
    p = subprocess.run([args.port, '-sj', path], capture_output=True, timeout=TIMEOUT)
    try:
        rep = json.loads(p.stdout.decode('utf-8', 'replace'))
    except ValueError:
        return path, [('?', 'no JSON output', 0, 0)]
    return path, [(f['format_id'], f['format'], f['offset'], f['size'])
                  for f in rep['findings'] if is_new(f['format_id'])]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--port', required=True)
    ap.add_argument('--corpus', default=os.path.join(HERE, 'newformats', 'cache'))
    ap.add_argument('--offline', action='store_true')
    ap.add_argument('--import', dest='import_dir')
    ap.add_argument('--only')
    ap.add_argument('--negative', action='append', default=[])
    ap.add_argument('--jobs', type=int, default=os.cpu_count() or 4)
    ap.add_argument('lists', nargs='*')
    args = ap.parse_args()
    args.port = os.path.abspath(args.port)

    lists = args.lists or sorted(glob.glob(os.path.join(HERE, 'newformats', '*.txt')))
    entries = read_lists(lists)
    if args.only:
        entries = [e for e in entries if args.only in e['line']]
    if args.import_dir:
        print(f'imported {import_files(entries, args.import_dir, args.corpus)} file(s)')

    corpus = Corpus(args.corpus, args.offline)
    passed = failed = skipped = 0
    with cf.ThreadPoolExecutor(args.jobs) as ex:
        for e, problems in ex.map(lambda e: test_entry(args, corpus, e), entries):
            if problems is None:
                skipped += 1
            elif problems:
                failed += 1
                print(f'FAIL {e["where"]}: {e["src"]}')
                for p in problems:
                    print(f'     {p}')
            else:
                passed += 1
    print(f'{passed} passed, {failed} failed, {skipped} skipped (not available offline) '
          f'- {len(entries)} entries')

    for d in args.negative:
        paths = [os.path.join(dp, n) for dp, _, ns in os.walk(d) for n in ns]
        hits = 0
        with cf.ThreadPoolExecutor(args.jobs) as ex:
            for path, found in ex.map(lambda p: negative(args, p), sorted(paths)):
                for fid, fmt, off, size in found:
                    hits += 1
                    print(f'NEW  {os.path.relpath(path, d)}: id {fid} "{fmt}" at {off}, {size} bytes')
        print(f'{hits} new-format finding(s) in {len(paths)} file(s) under {d}')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
