#!/usr/bin/env python3
"""
make_mmd.py - builds the small synthetic OctaMED modules of this directory
(no music: a few notes, generated waveforms) and memory images of them as a
player leaves them, relocated by RelocModule (reloc.a) to a load address,
with the play routine's position written into the header.

  python make_mmd.py [OUTDIR]       (default: this directory)

Files:
  mmd0.med, mmd1.med              file images (MMD0 / MMD1: 1.70a formats)
  mmd2.mmd2, mmd3_multi.mmd3      file images (MMD3: two songs, MCN3)
  reloc_mmd0.bin ... reloc_mmd3_multi.bin   relocated memory images

The relocation follows reloc.a (OctaMED), quirks included, so this module
can also relocate real files: reloc(data, base).
"""
import math
import os
import struct
import sys


class Mod:
    """a module being laid out: structures are appended, on even offsets"""

    def __init__(self):
        self.b = bytearray()

    def put(self, data):
        if len(self.b) & 1:
            self.b.append(0)
        off = len(self.b)
        self.b += data
        return off

    def alloc(self, n):
        return self.put(bytes(n))

    def w8(self, off, v):
        self.b[off] = v & 0xFF

    def w16(self, off, v):
        struct.pack_into('>H', self.b, off, v & 0xFFFF)

    def w32(self, off, v):
        struct.pack_into('>I', self.b, off, v & 0xFFFFFFFF)


def wave(n, kind, amp=100):
    out = bytearray()
    for k in range(n):
        if kind == 'sine':
            v = int(amp * math.sin(2 * math.pi * k / n))
        elif kind == 'saw':
            v = int(-amp + 2 * amp * k / n)
        else:
            v = amp if k < n // 2 else -amp
        out.append(v & 0xFF)
    return bytes(out)


def header(m, ident, extra_songs=0, flags=0):
    h = m.alloc(0x34)
    m.b[h:h + 4] = ident
    m.w8(h + 0x14, flags)
    m.w16(h + 0x30, 0xFFFF)       # actplayline = -1, as files have it
    m.w8(h + 0x33, extra_songs)
    return h


def notes_mmd0(tracks, lines, seed):
    out = bytearray()
    for ln in range(lines):
        for t in range(tracks):
            if (ln + t * 3 + seed) % 8 == 0:
                note, inst = 13 + (ln + seed) % 24, 1 + (t & 1)
                out += bytes([note & 0x3F, (inst & 0x0F) << 4 | 0x0C, 0x30])
            else:
                out += bytes(3)
    return bytes(out)


def notes_mmd1(tracks, lines, seed):
    out = bytearray()
    for ln in range(lines):
        for t in range(tracks):
            if (ln + t * 5 + seed) % 6 == 0:
                out += bytes([13 + (ln + seed) % 36, 1 + (t % 3), 0x0C, 0x28])
            else:
                out += bytes(4)
    return bytes(out)


def song_common(m, song, numblocks, numsamples, svols):
    for k, (rep, replen, vol) in enumerate(svols):
        o = song + 8 * k
        m.w16(o, rep)
        m.w16(o + 2, replen)
        m.w8(o + 6, vol)
    m.w16(song + 0x1F8, numblocks)
    m.w16(song + 0x2FC, 6)          # deftempo
    m.w8(song + 0x2FF, 0x20)        # flags: ST slides
    m.w8(song + 0x301, 6)           # tempo2
    m.w8(song + 0x312, 64)          # mastervol
    m.w8(song + 0x313, numsamples)


def instruments(m, kinds):
    """the instruments (sample / synth / hybrid / stereo16); returns offsets"""
    offs = []
    for kind in kinds:
        if kind == 'sample':
            data = wave(128, 'sine')
            o = m.put(struct.pack('>Ih', len(data), 0) + data)
        elif kind == 'stereo16':
            left = b''.join(struct.pack('>h', 40 * (k % 50) - 1000) for k in range(64))
            right = b''.join(struct.pack('>h', 1000 - 40 * (k % 50)) for k in range(64))
            o = m.put(struct.pack('>Ih', len(left), 0x30) + left + right)
        else:  # 'synth' / 'hybrid': SynthInstr + waveforms (pointers relative to it)
            nwf = 2
            o = m.alloc(278 + 4 * nwf)
            m.w16(o + 4, 0xFFFF if kind == 'synth' else 0xFFFE)
            m.w16(o + 14, 3)        # voltbllen
            m.w16(o + 16, 3)        # wftbllen
            m.w8(o + 18, 1)
            m.w8(o + 19, 1)
            m.w16(o + 20, nwf)
            m.b[o + 22:o + 25] = bytes([0x40, 0x30, 0xFF])         # voltbl
            m.b[o + 150:o + 153] = bytes([0x00, 0x01, 0xFF])       # wftbl
            for k in range(nwf):
                if kind == 'hybrid' and k == 0:
                    data = wave(256, 'saw')
                    w = m.put(struct.pack('>Ih', len(data), 0) + data)
                else:
                    data = wave(32, 'square' if k else 'sine')
                    w = m.put(struct.pack('>H', len(data) // 2) + data)
                m.w32(o + 278 + 4 * k, w - o)
            m.w32(o, len(m.b) - o - 6)  # length of the synthsound
        offs.append(o)
    return offs


def expdata(m, nsamples, name, anno=None, extras=False):
    e = m.alloc(0x50)
    ie = m.put(b''.join(bytes([0, 0, 0, 0, 0, 0, 0, 0]) for _ in range(nsamples)))
    m.w32(e + 4, ie)
    m.w16(e + 8, nsamples)
    m.w16(e + 0x0A, 8)
    if anno:
        a = m.put(anno + b'\0')
        m.w32(e + 0x0C, a)
        m.w32(e + 0x10, len(anno) + 1)
    ii = m.put(b''.join(('instrument %d' % k).encode().ljust(40, b'\0') for k in range(nsamples)))
    m.w32(e + 0x14, ii)
    m.w16(e + 0x18, nsamples)
    m.w16(e + 0x1A, 40)
    sn = m.put(name + b'\0')
    m.w32(e + 0x2C, sn)
    m.w32(e + 0x30, len(name) + 1)
    if extras:
        # MIDI dumps (one), mmdinfo text, an ARexx trigger, command 3x
        dd = m.alloc(8 + 4)
        m.w16(dd, 1)
        dump = m.alloc(10 + 20)
        data = m.put(bytes([0xF0, 0x43, 0x10, 0xF7]))
        m.w32(dump, 4)
        m.w32(dump + 4, data)
        m.w16(dump + 8, 20)
        m.b[dump + 10:dump + 15] = b'reset'
        m.w32(dd + 8, dump)
        m.w32(e + 0x34, dd)
        text = b'made by make_mmd.py\n\0'
        mi = m.alloc(12 + len(text))
        m.w16(mi + 6, 1)
        m.w32(mi + 8, len(text))
        m.b[mi + 12:mi + 12 + len(text)] = text
        m.w32(e + 0x38, mi)
        rx = m.alloc(8)
        m.w16(rx + 2, 20)
        trig = m.alloc(20)
        cmd = m.put(b'say hello\0')
        port = m.put(b'REXX\0')
        m.w8(trig + 4, 1)
        m.w32(trig + 8, cmd)
        m.w32(trig + 12, port)
        m.w16(trig + 16, 9)
        m.w16(trig + 18, 4)
        m.w32(rx + 4, trig)
        m.w32(e + 0x3C, rx)
        c3 = m.alloc(12)
        m.w16(c3 + 2, 2)
        ct = m.put(bytes([1, 2]))
        cn = m.put(struct.pack('>HH', 7, 10))
        m.w32(c3 + 4, ct)
        m.w32(c3 + 8, cn)
        m.w32(e + 0x40, c3)
    return e


def build_mmd01(version):
    m = Mod()
    h = header(m, b'MMD' + version)
    song = m.alloc(0x314)
    numblocks = 2
    song_common(m, song, numblocks, 3, [(0, 64, 64), (0, 1, 48), (0, 0, 64)])
    m.w16(song + 0x1FA, 3)                  # songlen
    m.b[song + 0x1FC:song + 0x1FF] = bytes([0, 1, 0])
    for t in range(16):
        m.w8(song + 0x302 + t, 64)          # trkvol
    blockarr = m.alloc(4 * numblocks)
    for k in range(numblocks):
        if version == b'0':
            b = m.put(bytes([4, 63]) + notes_mmd0(4, 64, k))
        else:
            b = m.alloc(8)
            m.w16(b, 4)
            m.w16(b + 2, 63)
            m.put(notes_mmd1(4, 64, k))
            if k == 1:                      # a BlockInfo: highlight, name, 1 command page
                info = m.alloc(36)
                hl = m.put(struct.pack('>II', 0x11111111, 0x11111111))
                nm = m.put(b'second block\0')
                pt = m.alloc(4 + 4)
                m.w16(pt, 1)
                pg = m.put(bytes(2 * 4 * 64))
                m.w32(pt + 4, pg)
                m.w32(info, hl)
                m.w32(info + 4, nm)
                m.w32(info + 8, 13)
                m.w32(info + 12, pt)
                m.w32(b + 4, info)
        m.w32(blockarr + 4 * k, b)
    smplarr = m.alloc(4 * 3)
    for k, o in enumerate(instruments(m, ['sample', 'synth', 'sample'])):
        m.w32(smplarr + 4 * k, o)
    e = expdata(m, 3, b'synthetic ' + version, anno=b'no music here')
    m.w32(h + 8, song)
    m.w32(h + 0x10, blockarr)
    m.w32(h + 0x18, smplarr)
    m.w32(h + 0x20, e)
    if len(m.b) & 1:
        m.b.append(0)
    m.w32(h + 4, len(m.b))
    return bytes(m.b)


def mmd2_song(m, h, nblocks, tracks, seeds):
    """play sequences, tables, MMD2song, blocks of one song; returns (song, blockarr)"""
    pseqs = []
    for k in range(2):
        seq = [0, 1] if k == 0 else [1, 0, 1]
        ps = m.alloc(42 + 2 * len(seq))
        m.b[ps:ps + 9] = b'sequence' + bytes([0x30 + k])
        m.w16(ps + 40, len(seq))
        for j, blk in enumerate(seq):
            m.w16(ps + 42 + 2 * j, blk % nblocks)
        pseqs.append(ps)
    pst = m.alloc(4 * len(pseqs))
    for k, ps in enumerate(pseqs):
        m.w32(pst + 4 * k, ps)
    sections = [0, 1, 0]
    sec = m.put(b''.join(struct.pack('>H', s) for s in sections))
    tv = m.put(bytes([64] * tracks))
    tp = m.put(bytes([0, 0xF8, 8, 0, 4, 0xFC][:tracks]))
    song = m.alloc(0x314)
    song_common(m, song, nblocks, 4, [(0, 64, 64), (0, 0, 64), (0, 1, 32), (0, 0, 64)])
    m.w16(song + 0x1FA, len(sections))
    m.w32(song + 0x1FC, pst)
    m.w32(song + 0x200, sec)
    m.w32(song + 0x204, tv)
    m.w16(song + 0x208, tracks)
    m.w16(song + 0x20A, len(pseqs))
    m.w32(song + 0x20C, tp)
    m.w32(song + 0x210, 1)          # flags3: stereo
    m.w16(song + 0x216, tracks)     # mixing channels
    m.w8(song + 0x218, 1)           # echo
    m.w8(song + 0x219, 2)
    m.w16(song + 0x21A, 120)
    blockarr = m.alloc(4 * nblocks)
    for k in range(nblocks):
        lines = 32 + 16 * k
        b = m.alloc(8)
        m.w16(b, tracks)
        m.w16(b + 2, lines - 1)
        m.put(notes_mmd1(tracks, lines, seeds + k))
        if k == 0:
            info = m.alloc(36)
            nm = m.put(b'intro\0')
            m.w32(info + 4, nm)
            m.w32(info + 8, 6)
            m.w32(b + 4, info)
        m.w32(blockarr + 4 * k, b)
    m.w32(h + 8, song)
    m.w32(h + 0x10, blockarr)
    return song, blockarr


def build_mmd23(version, songs):
    m = Mod()
    h0 = header(m, b'MMD' + version, extra_songs=songs - 1, flags=1 if version == b'3' else 0)
    mmd2_song(m, h0, 3, 6, 0)
    smplarr = m.alloc(4 * 4)
    for k, o in enumerate(instruments(m, ['sample', 'hybrid', 'stereo16', 'synth'])):
        m.w32(smplarr + 4 * k, o)
    m.w32(h0 + 0x18, smplarr)
    e0 = expdata(m, 4, b'first song', anno=b'two songs, synthetic', extras=True)
    m.w32(h0 + 0x20, e0)
    prev = e0
    for k in range(1, songs):
        hk = header(m, b'MCN' + version)
        mmd2_song(m, hk, 2, 4, 7 * k)
        m.w32(hk + 0x18, smplarr)            # shared instruments
        ek = m.alloc(0x50)
        sn = m.put(b'song %d\0' % (k + 1))
        m.w32(ek + 0x2C, sn)
        m.w32(ek + 0x30, len(b'song %d' % (k + 1)) + 1)
        m.w32(hk + 0x20, ek)
        m.w32(prev, hk)                      # nextmod
        prev = ek
    if len(m.b) & 1:
        m.b.append(0)
    m.w32(h0 + 4, len(m.b))
    return bytes(m.b)


# ------------------------------------------------------------- reloc.a

def reloc(data, base):
    """RelocModule (reloc.a / loadmod.a) on a copy of 'data', loaded at 'base'"""
    m = bytearray(data)
    n = len(m)

    def r8(o):
        return m[o] if 0 <= o < n else 0

    def r16(o):
        return (r8(o) << 8) | r8(o + 1)

    def r32(o):
        return (r16(o) << 16) | r16(o + 2)

    def w32(o, v):
        if 0 <= o and o + 4 <= n:
            struct.pack_into('>I', m, o, v & 0xFFFFFFFF)

    def off(a):  # an absolute address inside the module, or None
        o = (a - base) & 0xFFFFFFFF
        return o if a and o < n else None

    def entr(slot):
        v = r32(slot)
        if v:
            w32(slot, v + base)

    def relocp(h):
        for k in (8, 0x10, 0x18, 0x20):
            entr(h + k)

    def reloci(h, song):
        sa = off(r32(h + 0x18))
        if sa is None:
            return
        for k in range(((r8(song + 0x313) - 1) & 0xFF) + 1):
            entr(sa + 4 * k)
            d3 = r32(sa + 4 * k)
            sp = off(d3)
            if sp is None or (r16(sp + 4) & 0x8000) == 0:
                continue
            for j in range(r16(sp + 20) or 0x10000):
                w32(sp + 278 + 4 * j, r32(sp + 278 + 4 * j) + d3)

    def relocb(h, song):
        ba = off(r32(h + 0x10))
        if ba is None:
            return
        nb = r16(song + 0x1F8)
        count = ((nb & 0xFF00) | ((nb - 1) & 0xFF)) + 1
        for k in range(count):
            entr(ba + 4 * k)
        if m[h + 3] == ord('T') or m[h + 3] < ord('1') or nb > 0x8000:
            return
        for k in range(nb):
            b = off(r32(ba + 4 * (count - 1 - k)))
            if b is None:
                continue
            entr(b + 4)
            info = off(r32(b + 4))
            if info is None:
                continue
            entr(info)
            entr(info + 4)
            entr(info + 12)
            pt = off(r32(info + 12))
            if pt is None:
                continue
            for j in range(r16(pt) or 0x10000):
                entr(pt + 4 + 4 * j)

    def relocmmd2(song):
        for k in (0x1FC, 0x200, 0x204, 0x20C):
            entr(song + k)
        pst = off(r32(song + 0x1FC))
        if pst is None:
            return
        for k in range(r16(song + 0x20A) or 0x10000):
            entr(pst + 4 * k)

    def relocexp(e):
        for k in (0, 4, 0x0C, 0x14, 0x20, 0x28, 0x2C, 0x34, 0x38, 0x3C, 0x40):
            entr(e + k)
        d = off(r32(e + 0x34))
        if d is not None:
            for k in range(r16(d)):
                entr(d + 8 + 4 * k)
                dp = off(r32(d + 8 + 4 * k))
                if dp is not None:
                    entr(dp + 4)
        rx = off(r32(e + 0x3C))
        if rx is not None:
            entr(rx + 4)
            t = off(r32(rx + 4))
            if t is not None:
                entr(t + 8)
                entr(t + 12)
        c3 = off(r32(e + 0x40))
        if c3 is not None:
            entr(c3 + 4)
            entr(c3 + 8)

    h = 0
    relocp(h)
    song = off(r32(h + 8))
    reloci(h, song)
    left = m[0x33]
    while True:
        relocb(h, song)
        if m[h + 3] != ord('T') and m[h + 3] >= ord('2'):
            relocmmd2(song)
        e = off(r32(h + 0x20))
        if e is None:
            break
        relocexp(e)
        left -= 1
        if left < 0:
            break
        nh = off(r32(e))
        if nh is None:
            break
        h = nh
        relocp(h)
        song = off(r32(h + 8))
    return bytes(m)


def played(data, base, h=0, pblock=1, pline=17):
    """the header fields the play routine writes"""
    m = bytearray(data)
    struct.pack_into('>HH', m, h + 0x0C, 1, 4)              # psecnum, pseq
    struct.pack_into('>hHHHhB', m, h + 0x28, -1, pblock, pline, 0, pline, 3)
    return bytes(m)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    files = {
        'mmd0.med': build_mmd01(b'0'),
        'mmd1.med': build_mmd01(b'1'),
        'mmd2.mmd2': build_mmd23(b'2', 1),
        'mmd3_multi.mmd3': build_mmd23(b'3', 3),
    }
    bases = {'mmd0.med': 0x0002A3C8, 'mmd1.med': 0x00C01238,
             'mmd2.mmd2': 0x0000F004, 'mmd3_multi.mmd3': 0x07E41AB0}
    for name, data in list(files.items()):
        img = played(reloc(data, bases[name]), bases[name])
        files['reloc_' + name.split('.')[0] + '.bin'] = img
    for name, data in files.items():
        with open(os.path.join(out, name), 'wb') as f:
            f.write(data)


if __name__ == '__main__':
    main()
