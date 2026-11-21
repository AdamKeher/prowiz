#!/usr/bin/env python3
"""
difftest.py - differential test of the port against the original Pro-Wizard.

Runs the reference binary (tests/reference/build_reference.sh) and the port on
the same inputs, each in its own empty directory, and compares the console
text and every file written, byte for byte.

  python tests/difftest.py --ref build/reference/prowiz-ref.exe \
                           --port build/prowiz.exe  FILE_OR_DIR...

Options:
  --jobs N         parallel runs (default: CPU count)
  --timeout S      per-run timeout in seconds (default 60)
  --keep DIR       keep the output of differing inputs under DIR
  --only TEXT      only report differences in found-lines containing TEXT
  --types FILE     put FILE in both working directories as "_types_"
  --skip-list FILE skip inputs whose path contains any line of FILE
                   (# starts a comment); see tests/corpus/platform-dependent.txt
  --known-list FILE report differences of inputs listed in FILE as KNOWN
                   without failing; see tests/corpus/known-differences.txt
  --quiet          print only the summary and the failures
Exit status: 0 if everything matched, 1 otherwise.
"""
import argparse
import concurrent.futures as cf
import os
import shutil
import subprocess
import sys
import tempfile


# Some inputs make the ORIGINAL loop forever while writing (e.g. its P30A
# converter writes ~350 MB/s). Kill any run whose output passes this size.
MAX_OUTPUT = 768 * 1024 * 1024


def dir_size(path):
    total = 0
    try:
        for e in os.scandir(path):
            try:
                total += e.stat().st_size
            except OSError:
                pass
    except OSError:
        pass
    return total


def run(exe, inp, workdir, timeout, types=None, extra=()):
    if types is not None:
        shutil.copyfile(types, os.path.join(workdir, '_types_'))
    outf = tempfile.TemporaryFile()
    p = subprocess.Popen([exe, *extra, inp], cwd=workdir, stdout=outf, stderr=subprocess.DEVNULL)
    rc = None
    t = 0.0
    while rc is None:
        try:
            rc = p.wait(timeout=0.25)
        except subprocess.TimeoutExpired:
            t += 0.25
            if t >= timeout:
                p.kill()
                p.wait()
                rc = 'timeout'
            elif dir_size(workdir) > MAX_OUTPUT:
                p.kill()
                p.wait()
                rc = 'outputcap'
    outf.seek(0)
    out = outf.read()
    outf.close()
    files = {}
    if rc in ('timeout', 'outputcap'):
        return rc, out.replace(b'\r\n', b'\n'), files
    for name in sorted(os.listdir(workdir)):
        if name == '_types_':
            continue
        with open(os.path.join(workdir, name), 'rb') as f:
            files[name] = f.read()
    return rc, out.replace(b'\r\n', b'\n'), files


def crashed(rc):
    if rc in ('timeout', 'outputcap'):
        return True
    if isinstance(rc, int) and (rc < 0 or rc > 255):
        return True  # signal / Windows exception code
    return False


def first_diff(a, b):
    n = min(len(a), len(b))
    for i in range(n):
        if a[i] != b[i]:
            return i
    return n


def compare(args, inp):
    inp = os.path.abspath(inp)
    with tempfile.TemporaryDirectory(prefix='pwdt') as tmp:
        dref = os.path.join(tmp, 'ref')
        dport = os.path.join(tmp, 'port')
        os.mkdir(dref)
        os.mkdir(dport)
        types = getattr(args, 'types', None)
        rrc, rout, rfiles = run(args.ref, inp, dref, args.timeout, types)
        # -c: the port with Pro-Wizard 1.70a's formats only
        prc, pout, pfiles = run(args.port, inp, dport, args.timeout, types, ('-c',))
        problems = []
        if crashed(prc):
            problems.append(f'port crashed/timed out (rc={prc})')
        if crashed(rrc):
            # nothing reliable to compare against
            return inp, 'refcrash', problems, rrc
        if args.only:
            rl = [l for l in rout.split(b'\n') if args.only.encode() in l]
            pl = [l for l in pout.split(b'\n') if args.only.encode() in l]
            if rl != pl:
                problems.append('found-lines differ:\n    ref : %r\n    port: %r' % (rl[:4], pl[:4]))
        elif rout != pout:
            rlines, plines = rout.split(b'\n'), pout.split(b'\n')
            k = first_diff(rlines, plines)
            problems.append('stdout differs at line %d:\n    ref : %r\n    port: %r' % (
                k + 1, rlines[k] if k < len(rlines) else b'<eof>', plines[k] if k < len(plines) else b'<eof>'))
        for name in sorted(set(rfiles) | set(pfiles)):
            if name not in pfiles:
                problems.append(f'missing file {name} ({len(rfiles[name])} bytes)')
            elif name not in rfiles:
                problems.append(f'extra file {name} ({len(pfiles[name])} bytes)')
            elif rfiles[name] != pfiles[name]:
                a, b = rfiles[name], pfiles[name]
                problems.append(f'file {name} differs: ref {len(a)} bytes, port {len(b)} bytes, '
                                f'first difference at offset {first_diff(a, b)}')
        if problems and args.keep:
            dst = os.path.join(args.keep, os.path.basename(inp))
            shutil.rmtree(dst, ignore_errors=True)
            shutil.copytree(tmp, dst)
            with open(os.path.join(dst, 'ref.stdout'), 'wb') as f:
                f.write(rout)
            with open(os.path.join(dst, 'port.stdout'), 'wb') as f:
                f.write(pout)
        return inp, ('diff' if problems else 'ok'), problems, rrc


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--ref', required=True)
    ap.add_argument('--port', required=True)
    ap.add_argument('--jobs', type=int, default=os.cpu_count() or 4)
    ap.add_argument('--timeout', type=float, default=60)
    ap.add_argument('--keep')
    ap.add_argument('--only')
    ap.add_argument('--quiet', action='store_true')
    ap.add_argument('--types')
    ap.add_argument('--skip-list')
    ap.add_argument('--known-list')
    ap.add_argument('inputs', nargs='+')
    args = ap.parse_args()
    args.ref = os.path.abspath(args.ref)
    args.port = os.path.abspath(args.port)

    def read_list(path):
        out = []
        if path:
            for line in open(path, encoding='utf-8'):
                line = line.split('#', 1)[0].strip()
                if line:
                    out.append(line.replace('\\', '/'))
        return out

    def listed(f, lst):
        return any(s in f.replace('\\', '/') for s in lst)

    skip = read_list(args.skip_list)
    known = read_list(args.known_list)

    files = []
    for x in args.inputs:
        if os.path.isdir(x):
            for root, _, names in os.walk(x):
                files += [os.path.join(root, n) for n in sorted(names)]
        else:
            files.append(x)
    files = [f for f in files if not listed(f, skip)]

    counts = {'ok': 0, 'diff': 0, 'known': 0, 'refcrash': 0}
    with cf.ThreadPoolExecutor(args.jobs) as ex:
        for inp, status, problems, rrc in ex.map(lambda f: compare(args, f), files):
            if status == 'diff' and listed(inp, known):
                status = 'known'
            counts[status] += 1
            if status in ('diff', 'known') or (status == 'refcrash' and problems):
                print(f"{'KNOWN' if status == 'known' else 'DIFF'} {inp}")
                for p in problems:
                    print('  ' + p)
            elif status == 'refcrash':
                if not args.quiet:
                    print(f'SKIP {inp} (reference crashed: rc={rrc})')
            elif not args.quiet:
                print(f'ok   {inp}')
    print(f"\n{counts['ok']} identical, {counts['diff']} different, "
          f"{counts['known']} known differences, "
          f"{counts['refcrash']} skipped (reference crashed) - {len(files)} inputs")
    return 1 if counts['diff'] else 0


if __name__ == '__main__':
    sys.exit(main())
