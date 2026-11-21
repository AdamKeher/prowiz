#!/usr/bin/env python3
"""
fuzznew.py - crash and hang hunting for the format handlers.

Mutates seed files (real modules) and runs "prowiz -sj" on every mutant. It
reports runs that crash (any exit status other than 0 or 1, or a sanitizer
report) or take longer than --timeout seconds. Build prowiz with sanitizers
for this, e.g. on Linux or WSL:

  make O=build-asan CFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" \\
       LDFLAGS="-fsanitize=address,undefined"
  python3 tests/fuzznew.py --port build-asan/prowiz --count 300 SEED_FILE_OR_DIR...

Mutations favour the first 4 KiB, where the headers are: flipped bytes,
16/32-bit fields set to extreme values, truncations, duplicated ranges, and
the seed embedded at an odd or even offset.

Options:
  --port EXE      binary to run (required)
  --count N       mutants per seed file (default 200)
  --timeout S     seconds before a run counts as a hang (default 10)
  --seed N        random seed (default 1)
  --keep DIR      save every crashing or hanging input there
  --jobs N        parallel runs (default: CPU count)
Exit status: 0 if nothing crashed or hung.
"""
import argparse
import concurrent.futures as cf
import os
import random
import subprocess
import sys
import tempfile

EXTREMES16 = [0, 1, 2, 0x7f, 0x80, 0xff, 0x7fff, 0x8000, 0xfffe, 0xffff]
EXTREMES32 = [0, 1, 2, 4, 0x7fff, 0xffff, 0x10000, 0x7fffffff, 0x80000000, 0xfffffffe, 0xffffffff]


def mutate(rng, data):
    d = bytearray(data)
    if not d:
        return bytes(d)

    def pos():
        n = len(d)
        if rng.random() < 0.7:
            return rng.randrange(min(n, 4096))
        return rng.randrange(n)

    for _ in range(rng.randint(1, 4)):
        kind = rng.random()
        if kind < 0.35:
            for _ in range(rng.randint(1, 8)):
                d[pos()] = rng.getrandbits(8)
        elif kind < 0.55:
            p = pos() & ~1
            v = rng.choice(EXTREMES16 + [rng.getrandbits(16)])
            d[p:p + 2] = v.to_bytes(2, 'big')
        elif kind < 0.75:
            p = pos() & ~1
            v = rng.choice(EXTREMES32 + [len(d), len(d) * 2, rng.getrandbits(32)])
            d[p:p + 4] = v.to_bytes(4, 'big')
        elif kind < 0.85 and len(d) > 16:
            del d[rng.randrange(16, len(d)):]
        elif kind < 0.95:
            a = pos()
            b = min(len(d), a + rng.randint(1, 512))
            d[b:b] = d[a:b]
        else:
            pad = rng.choice([1, 2, 3, 1000, 1001])
            d[0:0] = bytes(rng.getrandbits(8) for _ in range(pad))
    return bytes(d)


def run(port, data, timeout):
    with tempfile.NamedTemporaryFile(delete=False, suffix='.bin') as f:
        f.write(data)
        path = f.name
    env = dict(os.environ)
    env.setdefault('ASAN_OPTIONS', 'detect_leaks=0:exitcode=99')
    env.setdefault('UBSAN_OPTIONS', 'halt_on_error=1:exitcode=98:print_stacktrace=1')
    try:
        p = subprocess.run([port, '-sj', path], capture_output=True, timeout=timeout, env=env)
        err = p.stderr.decode('utf-8', 'replace')
        if p.returncode not in (0, 1) or 'runtime error' in err or 'AddressSanitizer' in err:
            return 'CRASH', f'exit {p.returncode}: {err[-1500:]}'
        return None, None
    except subprocess.TimeoutExpired:
        return 'HANG', f'over {timeout} s'
    finally:
        os.unlink(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--port', required=True)
    ap.add_argument('--count', type=int, default=200)
    ap.add_argument('--timeout', type=float, default=10)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--keep')
    ap.add_argument('--jobs', type=int, default=os.cpu_count() or 4)
    ap.add_argument('inputs', nargs='+')
    args = ap.parse_args()
    port = os.path.abspath(args.port)

    seeds = []
    for x in args.inputs:
        if os.path.isdir(x):
            seeds += [os.path.join(dp, n) for dp, _, ns in os.walk(x) for n in sorted(ns)]
        else:
            seeds.append(x)

    jobs = []
    for s in seeds:
        with open(s, 'rb') as f:
            data = f.read()
        rng = random.Random(f'{args.seed}:{os.path.basename(s)}')
        for k in range(args.count):
            jobs.append((s, k, mutate(rng, data)))

    bad = 0
    with cf.ThreadPoolExecutor(args.jobs) as ex:
        for (s, k, data), (kind, info) in zip(jobs, ex.map(lambda j: run(port, j[2], args.timeout), jobs)):
            if kind is None:
                continue
            bad += 1
            print(f'{kind} {os.path.basename(s)} mutant {k}: {info}')
            if args.keep:
                os.makedirs(args.keep, exist_ok=True)
                with open(os.path.join(args.keep, f'{os.path.basename(s)}.{k}.{kind.lower()}'), 'wb') as f:
                    f.write(data)
    print(f'{len(jobs)} runs, {bad} crash(es) or hang(s), {len(seeds)} seed file(s)')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
