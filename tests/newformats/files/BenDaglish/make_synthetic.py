#!/usr/bin/env python3
"""Writes synthetic.bd: the layout the Ben Daglish handler checks (jump table,
init and sound effect code shapes, two instrument tables), with filler
instead of replay code (it does not play)."""
import os
import struct

NOP, RTS = 0x4E71, 0x4E75
INIT, PLAY, SFX = 0x10, 0x40, 0x50       # entry points
T1, T2, SAMPLES = 0x80, 0xD0, 0x120     # instrument tables, sample data
LENS = (200, 120, 64, 300)              # sample lengths in words


def words(*ws):
    return b''.join(struct.pack('>H', w & 0xFFFF) for w in ws)


def put(m, at, data):
    m[at:at + len(data)] = data


def table(offs, idx):
    """two longword offsets, then 30-byte records: sample offset, loop offset
    (both from table 1), length, loop length (words), volume, 16 more bytes"""
    out = struct.pack('>II', 8, 8 + 30)
    for k in idx:
        loop = offs[k] + 20 if k == 1 else 0
        out += struct.pack('>IIHHH', offs[k], loop, LENS[k], LENS[k] - 10 if loop else 0, 64)
        out += bytes(16)
    return out


def build():
    m = bytearray(words(*[NOP] * (SAMPLES // 2)))
    put(m, 0x00, words(0x6000, INIT - 0x02, 0x6000, PLAY - 0x06, 0x0000, 0x6000, SFX - 0x0C))
    # init: move.w d0,-(sp); bsr.w; move.w #0,$96(a6); lea x(pc),a0;
    #       add.w d0,d0 x 3; lea subsongs(pc),a0; nop; lea table1(pc),a0; rts
    put(m, INIT, words(0x3F00, 0x6100, PLAY - (INIT + 2), 0x3D7C, 0x0000, 0x0096,
                       0x41FA, 0x0060, 0xD040, 0xD040, 0xD040, 0x41FA, 0x0070,
                       NOP, 0x41FA, T1 - (INIT + 0x1E), RTS))
    put(m, PLAY, words(RTS))
    # sound effects: add.w d0,d0 x 2; lea table2(pc),a0; rts
    put(m, SFX, words(0xD040, 0xD040, 0x41FA, T2 - (SFX + 6), RTS))
    offs, o = [], SAMPLES - T1
    for n in LENS:
        offs.append(o)
        o += 2 * n
    put(m, T1, bytes(T2 - T1))
    put(m, T1, table(offs, (0, 1)))
    put(m, T2, bytes(SAMPLES - T2))
    put(m, T2, table(offs, (2, 3)))
    for n in LENS:
        m += bytes(((i * 13) & 0x7F) for i in range(2 * n))
    return bytes(m)


if __name__ == '__main__':
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'synthetic.bd'), 'wb') as f:
        f.write(build())
