#!/usr/bin/env python3
"""Writes synthetic.rh: the layout the Rob Hubbard handler checks, with
filler instead of replay code (it does not play)."""
import os
import struct

NOP, RTS = 0x4E71, 0x4E75


def words(*ws):
    return b''.join(struct.pack('>H', w) for w in ws)


def build():
    m = bytearray(words(*[NOP] * 0x30))      # 0x60 bytes of nop
    for k, target in enumerate((0x44, 0x48, 0x4C, 0x50, 0x54)):
        m[4 * k:4 * k + 4] = words(0x6000, target - (4 * k + 2))   # bra.w
    m[0x14:0x18] = words(0x41FA, 0x0010)                            # lea x(pc),a0
    m[0x18:0x1A] = words(RTS)
    m[0x1C:0x20] = words(RTS, 0x41FA)                               # rts; lea x(pc),a0
    m[0x20:0x22] = words(0x0008)
    samples = 0x60 + 0x40                                           # after adda.l #$40
    m[0x36:0x3E] = words(0x41FA, samples - 0x40 - 0x38, 0xD1FC, 0x0000)  # lea; adda.l
    m[0x3E:0x40] = words(0x0040)
    m[0x40:0x44] = words(0x7202, 0x2418)                            # moveq #2,d1; move.l (a0)+,d2
    m[0x5E:0x60] = words(RTS)
    m += bytes(0x40)                                                # player variables
    for n, freq in ((300, 8000), (1000, 11025), (64, 16000)):
        m += struct.pack('>IH', n, freq) + bytes((i * 7) & 0xFF for i in range(n))
    m += words(NOP)                                                 # the closing nop
    return bytes(m)


if __name__ == '__main__':
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'synthetic.rh'), 'wb') as f:
        f.write(build())
