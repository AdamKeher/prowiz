#!/usr/bin/env python3
"""Writes two files with the layouts the David Whittaker handler checks, with
filler instead of replay code (they do not play):

  synthetic_a.dw  prologue A (movem.l + bsr.w init, init with lea x(pc),a3),
                  music samples, then the 64 zero bytes
  synthetic_c.dw  prologue C (moveq #0,d0; lea start(pc),a3), music samples,
                  a block of waveforms, then a second (sound effect) set whose
                  end is the end of the module
"""
import os
import struct

NOP, RTS = 0x4E71, 0x4E75


def words(*ws):
    return b''.join(struct.pack('>H', w & 0xFFFF) for w in ws)


def put(m, at, data):
    m[at:at + len(data)] = data


def samples(spec):
    out = b''
    for n, freq in spec:
        out += struct.pack('>IH', n, freq) + bytes(((i * 5) & 0x7F) for i in range(n))
    return out


def music_setup(at, smp, info, n):
    """lea smp(pc),a0; lea info(pc),a5; moveq #n-1,d1; move.l (a0)+,d2; move.w (a0)+,d3"""
    return words(0x41FA, smp - (at + 2), 0x4BFA, info - (at + 6), 0x7200 | (n - 1), 0x2418, 0x3618)


def build_a():
    code = 0x100
    m = bytearray(words(*[NOP] * (code // 2)))
    # jump table: movem.l d0-d3/d7/a0-a6,-(sp); bsr.w x; movem.l (sp)+,...; rts
    put(m, 0x00, words(0x48E7, 0xF1FE, 0x6100, 0x40 - 0x06, 0x4CDF, 0x7F8F, RTS))
    put(m, 0x0E, words(0x48E7, 0xF1FE, 0x6100, 0x90 - 0x14, 0x4CDF, 0x7F8F, RTS))
    # init: lea base(pc),a3; bsr.w sample init; rts
    put(m, 0x40, words(0x47FA, 0x0000 - 0x42, 0x6100, 0x60 - 0x46, RTS))
    # sample init: tst.b x(a3); bne.w done; lea; lea; moveq; move.l; move.w ...; rts
    put(m, 0x60, words(0x4A2B, 0x00F0, 0x6600, 0x0020) +
        music_setup(0x68, code, 0xE0, 3) + words(RTS))
    put(m, 0x90, words(RTS))
    m += samples(((400, 8287), (96, 16574), (1000, 11025)))
    m += bytes(64)
    return bytes(m)


def build_c():
    code = 0x80
    spec1 = ((300, 8287), (500, 11025))
    spec2 = ((200, 8000), (64, 22050))
    music = samples(spec1)
    gap = bytes(64) + bytes(((i * 3) & 0xFF) for i in range(32))   # zeros + waveforms
    second = code + len(music) + len(gap)
    m = bytearray(words(*[NOP] * (code // 2)))
    # moveq #0,d0; lea start(pc),a3; bsr.w x; tst.b x(a3); bne.s; music set-up
    put(m, 0x00, words(0x7000, 0x47FA, 0xFFFC, 0x6100, 0x0060, 0x4A2B, 0x00F0, 0x6610) +
        music_setup(0x10, code, 0x70, 2) + words(RTS))
    # second set: lea x(pc),a0; adda.l #imm,a0; lea info(pc),a5; moveq #1,d1;
    #             move.w 2(a0),d2; mulu.w #50,d2
    at = 0x30
    put(m, at, words(0x41FA, code - (at + 2), 0xD1FC, 0x0000, second - code,
                     0x4BFA, 0x70 - (at + 12), 0x7201, 0x3428, 0x0002, 0xC4FC, 0x0032, RTS))
    m += music + gap + samples(spec2)
    return bytes(m)


if __name__ == '__main__':
    here = os.path.dirname(os.path.abspath(__file__))
    for name, data in (('synthetic_a.dw', build_a()), ('synthetic_c.dw', build_c())):
        with open(os.path.join(here, name), 'wb') as f:
            f.write(data)
