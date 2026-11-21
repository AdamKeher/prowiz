#!/usr/bin/env python3
"""
make_dbm.py - builds the small synthetic DigiBooster modules of this
directory (no music: a few notes, generated waveforms).

  python make_dbm.py [OUTDIR]       (default: this directory)

  dbpro2.dbm   DigiBooster Pro 2.21 style: NAME INFO SONG INST VENV PATT SMPL
  db3.dbm      DigiBooster 3 style: NAME INFO SONG INST PENV PATT PNAM SMPL DSPE
"""
import math
import os
import struct
import sys


def chunk(ident, data):
    return ident + struct.pack('>I', len(data)) + data


def pattern(rows, tracks, seed):
    """packed rows: track number, bit field, the fields it announces; 0 ends a row"""
    out = bytearray()
    for r in range(rows):
        if (r + seed) % 4 == 0:
            t = 1 + (r // 4) % tracks
            out += bytes([t, 0x03, 0x20 + (r + seed) % 0x30, 1 + r % 2])
        if r % 16 == 0:
            out += bytes([1, 0x0C, 0x0F, 6])     # command 1: speed
        out.append(0)
    return struct.pack('>HI', rows, len(out)) + bytes(out)


def envelope(instrument):
    e = bytearray(136)
    struct.pack_into('>HBB', e, 0, instrument, 0x01, 3)   # instrument, enabled, 3 sections
    for k, (pos, val) in enumerate([(0, 64), (8, 48), (24, 32), (40, 0)]):
        struct.pack_into('>HH', e, 4 + 4 * k, pos, val)
    return bytes(e)


def build(version, revision):
    tracks, nsongs = 4, 1
    samples = [
        (1, bytes((int(90 * math.sin(2 * math.pi * k / 64)) & 0xFF) for k in range(512))),
        (2, b''.join(struct.pack('>h', 300 * (k % 100) - 15000) for k in range(400))),
    ]
    insts = []
    for k in range(2):
        name = ('synthetic %d' % (k + 1)).encode().ljust(30, b'\0')
        insts.append(name + struct.pack('>HHIIIhH', k + 1, 64, 8363, 0, 64 if k == 0 else 0, 0,
                                        1 if k == 0 else 0))
    body = chunk(b'NAME', b'generated test module'.ljust(44, b'\0'))
    body += chunk(b'INFO', struct.pack('>5H', len(insts), len(samples), nsongs, 2, tracks))
    song = b'main'.ljust(44, b'\0') + struct.pack('>H', 3) + struct.pack('>3H', 0, 1, 0)
    body += chunk(b'SONG', song)
    body += chunk(b'INST', b''.join(insts))
    if version == 2:
        body += chunk(b'VENV', struct.pack('>H', 1) + envelope(1))
    else:
        body += chunk(b'PENV', struct.pack('>H', 1) + envelope(2))
    body += chunk(b'PATT', pattern(64, tracks, 0) + pattern(32, tracks, 3))
    if version == 3:
        body += chunk(b'PNAM', b'\x00' + b'intro\0' + b'verse\0')
    smpl = b''
    for bps, data in samples:
        smpl += struct.pack('>II', bps, len(data) // bps) + data
    body += chunk(b'SMPL', smpl)
    if version == 3:
        body += chunk(b'DSPE', struct.pack('>H', tracks) + bytes([0, 1, 1, 0]) +
                      struct.pack('>4H', 0x40, 0x80, 0x80, 0xFF))
    return b'DBM0' + bytes([version, revision, 0, 0]) + body


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    for name, data in (('dbpro2.dbm', build(2, 0x21)), ('db3.dbm', build(3, 0x05))):
        with open(os.path.join(out, name), 'wb') as f:
            f.write(data)


if __name__ == '__main__':
    main()
