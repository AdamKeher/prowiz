#!/usr/bin/env python3
"""
make_synthetic.py - writes the small synthetic Art of Noise files next to
this script (made-up notes and waves, no music by anyone).

  aon4.aon   4 channels, no INAM, ends with the editor's '?' pad
  aon8.aon   8 channels, INAM, no DATE, a size that needs no pad
"""
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
TAG = b'artofnoise by bastian spiegel (twice/lego)'


def chunk(cid, data):
    return cid + struct.pack('>I', len(data)) + data


def song(channels, inam, date, title):
    npos = 3
    plst = bytes([0, 1, 0])
    plst += bytes(((npos + 1) & ~1) - npos)
    patt = bytearray()
    for p in range(2):
        for row in range(64):
            for ch in range(channels):
                if row % 8 == 0 and ch < 2:
                    patt += bytes([0x10 + (row // 8 + p) % 12, 1 + ch % 2, 0, 0])
                else:
                    patt += bytes(4)
    waves = [bytes((i * 4) & 0xFF for i in range(64)),           # saw
             bytes([0x40] * 16 + [0xC0] * 16)]                    # square
    inst = bytearray()
    # sample instrument: type 0, volume, finetune, wave, start, length, loop
    inst += bytes([0, 64, 0, 0]) + struct.pack('>IIII', 0, 32, 0, 32) + bytes(8) + bytes([0, 0, 64, 0])
    # synth instrument: type 1, volume, finetune, wave, length, ...
    inst += bytes([1, 48, 0, 1, 16]) + bytes(5) + bytes([0, 0, 3, 0, 1, 0, 0, 0]) + bytes(10) + bytes([64, 4, 32, 2])
    assert len(inst) == 64
    wlen = struct.pack('>64I', *([len(w) for w in waves] + [0] * 62))
    out = b'AON' + str(channels).encode() + TAG
    out += chunk(b'NAME', title) + chunk(b'AUTH', b'prowiz tests')
    if date:
        out += chunk(b'DATE', b'21-november-2026')
    out += chunk(b'RMRK', b'synthetic test module\n')
    out += chunk(b'INFO', bytes([0x34, npos, 0, 0]))
    out += chunk(b'ARPG', bytes(0x40))
    out += chunk(b'PLST', plst)
    out += chunk(b'PATT', bytes(patt))
    out += chunk(b'INST', bytes(inst))
    if inam:
        names = b'saw'.ljust(32, b'\0') + b'square'.ljust(32, b'\0')
        out += chunk(b'INAM', names.ljust(0x7A0, b'\0'))
    out += chunk(b'WLEN', wlen)
    out += chunk(b'WAVE', b''.join(waves))
    return out


def main():
    a4 = song(4, inam=False, date=True, title=b'synthetic four')
    a4 += b'?' * ((4 - len(a4) % 4) % 4)
    assert len(a4) % 4 == 0 and a4.endswith(b'?')
    title = b'synth 8'
    while len(song(8, inam=True, date=False, title=title)) % 4:
        title += b'!'
    a8 = song(8, inam=True, date=False, title=title)
    for n, d in (('aon4.aon', a4), ('aon8.aon', a8)):
        with open(os.path.join(HERE, n), 'wb') as f:
            f.write(d)
        print(n, len(d))


if __name__ == '__main__':
    main()
