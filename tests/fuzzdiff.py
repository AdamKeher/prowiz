#!/usr/bin/env python3
"""
fuzzdiff.py - mutation fuzzing, differential against the original Pro-Wizard.

Takes seed files, derives mutants (byte flips, field tweaks, truncation,
garbage around the data, ...), runs the reference and the port on each and
reports any difference. Mutants that differ are saved for replay with
difftest.py.

  python tests/fuzzdiff.py --ref build/reference/prowiz-ref.exe \
         --port build/prowiz.exe --count 300 --save fuzz-fail SEED...

Reproducible: the same --seed gives the same mutants.
"""
import argparse
import concurrent.futures as cf
import os
import random
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import difftest  # noqa: E402


def mutate(rng, data):
    d = bytearray(data)
    ops = rng.randint(1, 4)
    for _ in range(ops):
        if not d:
            d = bytearray(rng.randbytes(256))
        op = rng.randrange(9)
        n = len(d)
        p = rng.randrange(n)
        if op == 0:  # flip a few bytes
            for _ in range(rng.randint(1, 8)):
                d[rng.randrange(n)] = rng.randrange(256)
        elif op == 1:  # interesting values
            d[p] = rng.choice([0x00, 0x01, 0x02, 0x1f, 0x20, 0x3f, 0x40, 0x41, 0x7f, 0x80, 0xfe, 0xff])
        elif op == 2:  # 16/32-bit big-endian field
            w = rng.choice([2, 4])
            v = rng.choice([0, 1, 2, 3, 4, 0x7f, 0x80, 0xff, 0x100, 0x400, 0x7fff, 0x8000, 0xffff,
                            0x10000, 0x7fffffff, 0xffffffff, rng.randrange(1 << (8 * w))])
            d[p:p + w] = (v & ((1 << (8 * w)) - 1)).to_bytes(w, 'big')
        elif op == 3:  # truncate
            del d[max(65, p):]
        elif op == 4:  # garbage prefix (moves the data)
            d[0:0] = rng.randbytes(rng.randint(1, 2048))
        elif op == 5:  # garbage / zero suffix
            d += rng.randbytes(rng.randint(1, 4096)) if rng.random() < .5 else bytes(rng.randint(1, 4096))
        elif op == 6:  # copy a block over another place
            q = rng.randrange(n)
            k = rng.randint(1, 256)
            d[q:q + k] = d[p:p + k]
        elif op == 7:  # zero a block
            k = rng.randint(1, 512)
            d[p:p + k] = bytes(min(k, n - p))
        else:  # +-1 on a byte (size fields, counters)
            d[p] = (d[p] + rng.choice([-1, 1])) & 0xff
    return bytes(d)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--ref', required=True)
    ap.add_argument('--port', required=True)
    ap.add_argument('--count', type=int, default=200, help='mutants per seed file')
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--jobs', type=int, default=os.cpu_count() or 4)
    ap.add_argument('--timeout', type=float, default=30)
    ap.add_argument('--save', help='directory for mutants that differ')
    ap.add_argument('seeds', nargs='+')
    args = ap.parse_args()
    args.ref = os.path.abspath(args.ref)
    args.port = os.path.abspath(args.port)
    args.keep = None
    args.only = None
    args.types = None

    if args.save:
        os.makedirs(args.save, exist_ok=True)
    total = bad = skipped = 0
    with tempfile.TemporaryDirectory(prefix='pwfz') as tmp:
        jobs = []
        for si, seed in enumerate(args.seeds):
            data = open(seed, 'rb').read()
            rng = random.Random(f'{args.seed}:{os.path.basename(seed)}')
            for k in range(args.count):
                path = os.path.join(tmp, f'{si:03d}_{k:05d}_{os.path.basename(seed)}')
                with open(path, 'wb') as f:
                    f.write(mutate(rng, data))
                jobs.append(path)
        with cf.ThreadPoolExecutor(args.jobs) as ex:
            for inp, status, problems, rrc in ex.map(lambda f: difftest.compare(args, f), jobs):
                total += 1
                if status == 'refcrash' and not problems:
                    skipped += 1
                    continue
                if status != 'ok':
                    bad += 1
                    print(f'DIFF {os.path.basename(inp)}')
                    for p in problems:
                        print('  ' + p)
                    if args.save:
                        with open(inp, 'rb') as f, open(os.path.join(args.save, os.path.basename(inp)), 'wb') as g:
                            g.write(f.read())
    print(f'\n{total} mutants: {total - bad - skipped} identical, {bad} different, {skipped} skipped (reference crashed)')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
