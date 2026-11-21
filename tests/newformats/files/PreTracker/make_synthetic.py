#!/usr/bin/env python3
"""
make_synthetic.py - writes the small synthetic PreTracker files next to this
script (no music by anyone: a few made-up notes and waves).

  v19.prt        version 19: old header, no wave order, POSD at 43
  v1b.prt        version 1B (PreTracker 1.0): wave order, POSD at 5B
  v1e_2sub.prt   version 1E (PreTracker 1.5): 2 subsongs, 64 instruments
  v1e_1sub.prt   version 1E with one subsong
  v1e_rc.bin     v1e_1sub.prt as Raspberry Casket leaves it in memory
                 (POSD + 8, 3C-3F = first subsong header [0],[1],[3],[2])
"""
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))


def name(s):
    return s.encode('ascii') + b'\0'


def wave_record(length_byte, osc):
    r = bytearray(42)
    chip = (length_byte + 1) * 128
    struct.pack_into('>HHH', r, 0, 0, chip, 32)   # loop start, loop end, subloop length
    r[0x07] = 1                                   # subloop wait
    struct.pack_into('>HH', r, 0x08, 2, chip)     # subloop step, sample length
    r[0x14] = osc                                 # flags: oscillator type
    r[0x17] = 0x18                                # base note
    r[0x18] = 0x80                                # gain
    r[0x19] = length_byte
    r[0x1E] = 0xFF                                # sustain
    r[0x27] = 1                                   # modulator length
    r[0x29] = 1                                   # modulator density
    return bytes(r)


def instruments(ninst, nnames):
    names = [name('lead'), name('bass drum'), name('pad')]
    names += [b'\0'] * (nnames - len(names))
    info = bytearray()
    pats = bytearray()
    for i in range(ninst):
        steps = (2, 1, 0)[i] if i < 3 else 0
        info += bytes([2, 3, 4, 1, 2, 10, 3, steps])
        for k in range(steps):
            pats += bytes([0x00, 0x00, 0x01 + (i % 2)])  # select wave 1 or 2
    return b''.join(names) + bytes(info) + bytes(pats)


def waves(nwav, waves_start):
    names = [name('square'), name('noisy saw')] + [b'\0'] * 22
    out = b''.join(names)
    if (waves_start + len(out)) & 1:
        out += b'\0'
    for k in range(nwav):
        out += wave_record(k, k & 3)
    return out


def pattern(nsteps, seed):
    out = bytearray()
    for r in range(nsteps):
        if r % 2 == 0:
            note = 0x0D + (seed + r) % 24
            out += bytes([note, 0x10, 0x00])            # note, instrument 1
        else:
            out += bytes([0, 0, 0])
    return bytes(out)


def header(ver, offs, nums, ninst, nwav, title=b'prowiz test', author=b'synthetic'):
    h = bytearray(b'PRT' + bytes([ver]))
    h += struct.pack('>IIII', *offs)
    h += title.ljust(20, b'\0') + author.ljust(20, b'\0')
    h += bytes(nums)                                    # 3C-3F
    h += bytes([ninst, nwav])
    return h


def old_song(ver):
    npat, npos, nsteps, ninst, nwav = 2, 3, 4, 2, 2
    hdr = 0x43 if ver <= 0x19 else 0x5B
    positions = bytes([1, 0, 2, 0, 0, 0, 0, 0,
                       2, 0x0C, 1, 0, 0, 0, 0, 0,
                       1, 0xFE, 2, 0, 1, 0, 2, 0])
    pats = pattern(nsteps, 0) + pattern(nsteps, 5)
    posd = hdr
    patt = posd + len(positions)
    inst = patt + len(pats)
    ins = instruments(ninst, 32)
    wave = inst + len(ins)
    h = header(ver, (posd, patt, inst, wave), (0, npat, npos, nsteps), ninst, nwav)
    if ver > 0x19:
        h += bytes([1, 0] + list(range(2, 24)))         # wave order
    h += b'\0'                                          # reserved byte
    assert len(h) == hdr
    return bytes(h) + positions + pats + ins + waves(nwav, wave)


def v15_song(subsongs):
    # subsongs: list of (restart, npat, nsteps, npos)
    ninst, nwav, nsub = 64, 2, len(subsongs)
    h = header(0x1E, (0, 0, 0, 0), (0, 0, 0, 0), ninst, nwav)
    h += bytes(range(24)) + bytes([nsub, 0])
    assert len(h) == 0x5C
    subhdr = bytearray()
    positions = bytearray()
    pats = bytearray()
    for n, (restart, npat, nsteps, npos) in enumerate(subsongs):
        subhdr += bytes([restart, npat, nsteps, npos]) + struct.pack('>I', len(pats))
        for p in range(npos):
            for ch in range(4):
                positions += bytes([(p + ch) % (npat + 1), 0])
        for k in range(npat):
            pats += pattern(nsteps, 3 * n + k)
    posd = 0x5C
    patt = posd + len(subhdr) + len(positions)
    inst = patt + len(pats)
    ins = instruments(ninst, 64)
    wave = inst + len(ins)
    struct.pack_into('>IIII', h, 4, posd, patt, inst, wave)
    return bytes(h) + bytes(subhdr) + bytes(positions) + bytes(pats) + ins + waves(nwav, wave)


def rc_patch(d):
    d = bytearray(d)
    hd = d[0x5C:0x60]
    d[0x3C:0x40] = bytes([hd[0], hd[1], hd[3], hd[2]])
    struct.pack_into('>I', d, 4, struct.unpack_from('>I', d, 4)[0] + 8)
    return bytes(d)


def main():
    files = {
        'v19.prt': old_song(0x19),
        'v1b.prt': old_song(0x1B),
        'v1e_2sub.prt': v15_song([(0, 2, 8, 2), (1, 3, 5, 3)]),
        'v1e_1sub.prt': v15_song([(0, 2, 6, 3)]),
    }
    files['v1e_rc.bin'] = rc_patch(files['v1e_1sub.prt'])
    for n, d in files.items():
        with open(os.path.join(HERE, n), 'wb') as f:
            f.write(d)
        print(n, len(d))


if __name__ == '__main__':
    main()
