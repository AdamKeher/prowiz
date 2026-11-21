#!/usr/bin/env python3
"""
make_synthetic.py - writes the small synthetic Musicline Editor files next
to this script (made-up notes and waves, no music by anyone).

  ml_new.ml   MODL data + VERS 1.15; exact TUNE lengths; a packed and an
              unpacked SMPL; ends with INFO at the end of the file
  ml_old.ml   VERS right at 08 (very old files); TUNE lengths as files up to
              1.06 store them (as if 8 channel sizes were there), including
              a 0-channel subsong
"""
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))


def chunk(cid, data, length=None):
    return cid + struct.pack('>I', len(data) if length is None else length) + data


def tune(title, channels, seqs, old):
    head = title.ljust(32, b'\0') + struct.pack('>HBBHBB', 125, 6, 0, 64, 0, channels)
    sizes = b''.join(struct.pack('>I', len(s)) for s in seqs[:channels])
    data = head + sizes + b''.join(seqs[:channels])
    length = len(data) + (4 * (8 - channels) if old else 0)
    return chunk(b'TUNE', data, length)


def part(number, rows):
    out = struct.pack('>H', number)
    for note, inst in rows:
        out += bytes([0x01, note, inst])           # note + instrument only
    out += b'\xff'
    if len(out) & 1:
        out += b'\0'
    return chunk(b'PART', out)


def arpg(number):
    rows = bytes([0, 1, 0, 0, 0, 0, 12, 1, 0, 0, 0, 0])
    return chunk(b'ARPG', struct.pack('>H', number) + rows)


def inst(title, sample):
    d = bytearray(206)
    d[0:len(title)] = title
    d[32] = sample                                  # sample number
    struct.pack_into('>H', d, 38, 32)               # sample length (words)
    struct.pack_into('>H', d, 58, 64)               # volume
    return chunk(b'INST', bytes(d))


def smpl(title, data, packed=None):
    """packed: (command byte, packed bytes) for a delta-packed sample"""
    hdr = bytearray(50)
    hdr[0:len(title)] = title
    struct.pack_into('>IHIH', hdr, 34, 0, len(data) // 2, 0, len(data) // 2)
    body = packed[1] if packed else data
    pre = struct.pack('>I', len(data)) + bytes([packed[0] if packed else 0, 0])
    return b'SMPL' + struct.pack('>I', 50 + len(body)) + pre + bytes(hdr) + body


def info():
    strings = [b'synthetic', b'prowiz tests', b'21-11-2026', b'0:05', b'', b'', b'', b'', b'']
    return chunk(b'INFO', b''.join(s + b'\0' for s in strings))


def seq(parts):
    return b''.join(struct.pack('>H', 0x0010 | (p << 4)) for p in parts)


def main():
    saw = bytes((i * 8) & 0xFF for i in range(64))
    ramp = bytes(range(0, 64))
    # delta-packed ramp: command 0x80, start byte 0, 63 deltas of +1 (nibbles)
    packed = (0x80, bytes([0x80, 0x00, 0x00, 0x3F]) + bytes([0x11] * 32))

    new = b'MLEDMODL' + struct.pack('>I', 4) + b'\x87\x65\x43\x21'
    new += b'VERS' + struct.pack('>I', 6) + b'\x01\x151.15'
    new += tune(b'synthetic song', 4, [seq([1, 2]), seq([2]), b'', seq([1])], old=False)
    new += tune(b'second subsong', 8, [seq([2])] * 8, old=False)
    new += part(1, [(0x25, 1), (0x29, 1), (0x2C, 2)])
    new += part(2, [(0x20, 2), (0x24, 1)])
    new += arpg(1)
    new += inst(b'saw lead', 1) + inst(b'ramp bass', 2)
    new += smpl(b'saw', saw) + smpl(b'ramp', ramp, packed)
    new += info()

    old = b'MLEDMODL' + b'VERS' + struct.pack('>I', 6) + b'\x00\x790.79'
    old += tune(b'old song', 4, [seq([1]), seq([1]), seq([1]), seq([1])], old=True)
    old += tune(b'------------', 0, [], old=True)
    old += part(1, [(0x25, 1), (0x30, 1)])
    old += inst(b'saw', 1)
    old += smpl(b'saw', saw)
    old += info()

    for n, d in (('ml_new.ml', new), ('ml_old.ml', old)):
        with open(os.path.join(HERE, n), 'wb') as f:
            f.write(d)
        print(n, len(d))


if __name__ == '__main__':
    main()
