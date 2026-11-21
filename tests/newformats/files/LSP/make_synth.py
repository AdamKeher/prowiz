"""Generate small synthetic LightSpeedPlayer test files (no copyrighted music):
the same made-up 4-voice tune with three synthetic waveforms, written in each
.lsmusic layout as LSPConvert would, plus memory images as the players leave
them (relocated pointers, flag byte, ADPCM bank decoded in place).

Usage: python -I make_synth.py <output dir>
The files next to this script are its output, byte for byte.
"""
import os
import random
import struct
import sys

PT = [856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
      428, 404, 381, 360, 340, 320, 302, 285, 269, 254, 240, 226,
      214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113]
DPCM = [0, 1, 2, 4, 8, 16, 32, 64, -128, -64, -32, -16, -8, -4, -2, -1]


def be16(v): return struct.pack('>H', v & 0xFFFF)
def be32(v): return struct.pack('>I', v & 0xFFFFFFFF)


# ------------------------------------------------------------ samples / bank
def make_samples():
    sq = bytes((40 if k < 32 else -40) & 0xFF for k in range(64))           # looped square
    rng = random.Random(1234)
    noise = [0, 0] + [int(rng.randint(-100, 100) * (1 - k / 600)) for k in range(2, 600)]
    noise = bytes(v & 0xFF for v in noise)                                   # one-shot burst
    saw = bytes(((k * 7) % 200 - 100) & 0xFF for k in range(200))           # intro + loop
    # (data, repStart, repLen) in bytes; one-shot = repLen 2 at 0
    return [(sq, 0, 64), (noise, 0, 2), (saw, 72, 128)]


SAMPLES = make_samples()


def bank_layout():
    offs, o = [], 4
    for d, _, _ in SAMPLES:
        offs.append(o)
        o += len(d)
    return offs, o


# LSP instruments: (sample index, 9xx offset in bytes)
INSTRS = [(0, 0), (1, 0), (2, 0), (1, 256)]


def instrument_table():
    offs, _ = bank_layout()
    out = []
    for s, so in INSTRS:
        d, rs, rl = SAMPLES[s]
        out.append((offs[s] + so, (len(d) - so) // 2, offs[s] + rs, rl // 2))
    return out


def pack_instruments(tab, base=0, odd=False):
    b = b''
    for st, ln, rp, rl in tab:
        b += be32(st + base + (1 if odd else 0)) + be16(ln) + be32(rp + base) + be16(rl)
    return b


def dpcm_encode(data):
    out, s = bytearray(), 0
    for k in range(0, len(data), 2):
        nibs = []
        for x in (data[k], data[k + 1]):
            t = x - 256 if x > 127 else x
            best, bd = 0, None
            for i, dv in enumerate(DPCM):
                e = s + dv
                if e < -128 or e > 127:
                    continue
                dist = abs((t - s) - dv)
                if bd is None or dist < bd:
                    bd, best = dist, i
            s += DPCM[best]
            nibs.append(best)
        out.append((nibs[0] << 4) | nibs[1])
    return bytes(out)


def dpcm_decode(packed):
    out, s = bytearray(), 0
    for b in packed:
        for n in (b >> 4, b & 15):
            s += DPCM[n]
            out.append(s & 0xFF)
    return bytes(out)


def bank(uid, adpcm_lossless=None):
    """(file bytes, in-memory bytes after the player decoded it, adpcm info)"""
    raw = b''.join(d for d, _, _ in SAMPLES)
    if adpcm_lossless is None:
        f = be32(uid) + raw
        return f, f, None
    total = len(raw)
    packed_parts, decoded = [], b''
    for k, (d, _, _) in enumerate(SAMPLES):
        if k in adpcm_lossless:
            packed_parts.append(d)
            decoded += d
        else:
            p = dpcm_encode(d)
            packed_parts.append(p)
            decoded += dpcm_decode(p)
    packed = b''.join(packed_parts)
    inplace = total - len(packed)
    f = be32(uid) + bytes(inplace) + packed
    mem = be32(uid) + decoded
    mask = 0
    for k in range(len(SAMPLES)):
        if k in adpcm_lossless:
            mask |= 1 << (31 - k)
    info = be32(inplace) + be32(mask) + b''.join(be16(len(d) // 2 - 1) for d, _, _ in SAMPLES) + be16(0)
    return f, mem, info


# ------------------------------------------------------------------ the tune
def tune(no_half=False):
    """frames: list of per-voice dicts {vol, per, code, inst}; bpm changes;
    sequence start frames; loop frame"""
    frames = []
    prev_dma = [False] * 4
    cur_vol = [-1] * 4
    cur_per = [-1] * 4
    notes = [0, 4, 7, 12, 7, 4, 2, 5, 9, 14, 9, 5]
    for f in range(192):
        vf = []
        for v in range(4):
            e = {'vol': None, 'per': None, 'code': 0, 'inst': None}
            step = f // 6
            if f % 6 == v % 3 and (step + v) % (v + 2) == 0:
                inst = [0, 2, 1, 3][v] if v != 3 else (1 if step % 2 else 3)
                e['inst'], e['code'] = inst, 3
                p = PT[(notes[step % len(notes)] + 6 * (v % 2)) % 36]
                if p != cur_per[v]:
                    e['per'] = cur_per[v] = p
                vol = 64 - 4 * (step % 5)
                if vol != cur_vol[v]:
                    e['vol'] = cur_vol[v] = vol
            else:
                if prev_dma[v]:
                    e['code'] = 1  # reload the repeat part
                if v == 0 and f % 3 == 1 and cur_per[0] > 0:          # vibrato
                    p = cur_per[0] + (2 if f % 6 == 1 else -2)
                    e['per'] = cur_per[0] = p
                if v == 2 and f % 4 == 2 and cur_vol[2] > 8:          # fade
                    e['vol'] = cur_vol[2] = cur_vol[2] - 3
                if not no_half and v == 1 and f % 48 == 30 and e['code'] == 0:
                    e['code'], e['inst'] = 2, 2                       # sample, no note
            vf.append(e)
        prev_dma = [vf[v]['code'] == 3 for v in range(4)]
        frames.append(vf)
    bpm = {0: None, 96: 140, 150: 125}
    seqs = [0, 48, 96, 144]
    return frames, bpm, seqs, 48


def word_new(vf):
    w = 0
    for v in range(4):
        w |= (vf[v]['code'] & 3) << (8 + 2 * v)
        if vf[v]['vol'] is not None:
            w |= 1 << (4 + v)
        if vf[v]['per'] is not None:
            w |= 1 << v
    return w


def word_a(vf):
    w = 0
    for v in range(4):
        if vf[v]['code'] == 1:
            w |= 1 << (12 + v)
        if vf[v]['vol'] is not None:
            w |= 1 << (8 + v)
        if vf[v]['per'] is not None:
            w |= 1 << (4 + v)
        if vf[v]['code'] == 3:
            w |= 1 << v
    return w


# --------------------------------------------------------------- LSP1 writer
def lsp1(layout, ver, uid, getpos=False, setpos=False, adpcm_info=None, bpmchg=True):
    frames, bpms, seqs, loop = tune(no_half=(layout == 'A'))
    words = [word_a(vf) if layout == 'A' else word_new(vf) for vf in frames]
    count = {}
    order = []
    for w in words:
        if w not in count:
            count[w] = 0
            order.append(w)
        count[w] += 1
    codes = sorted(order, key=lambda w: -count[w])
    if layout == 'A':
        esc_rew, esc_bpm, esc_pos = 0xFFFF, 0xF00F, None
        codes += [0xFFFF, 0xF00F]
    else:
        if layout == 'D':
            v = 0
            while len(codes) < 255:                      # dummy codes: ESC past 255
                while v in count or v in codes:
                    v += 1
                codes.append(v)
        used = set(codes)
        free = (x for x in range(1, 0x10000) if x not in used)
        esc_rew, esc_bpm = next(free), next(free)
        codes += [esc_rew, esc_bpm]
        esc_pos = None
        if layout == 'D':
            esc_pos = next(free)
            codes.append(esc_pos)
    code_of = {w: i for i, w in enumerate(codes)}
    table = []
    for i, w in enumerate(codes):
        if i % 255 == 0:
            table.append(0)
        table.append(w)

    def put_code(bs, w):
        i = code_of[w]
        bs += bytes(i // 255) + bytes([i % 255 + 1])

    bs, ws = bytearray(), bytearray()
    seqpos = {}
    bloop = wloop = 0
    for f, vf in enumerate(frames):
        if f == loop:
            bloop, wloop = len(bs), len(ws)
        if f in seqs and (getpos or setpos):
            seqpos[seqs.index(f)] = (len(ws), len(bs))
            if getpos:
                put_code(bs, esc_pos)
                bs.append(seqs.index(f))
        if bpmchg and bpms.get(f):
            put_code(bs, esc_bpm)
            bs.append(bpms[f])
        w = words[f]
        put_code(bs, w)
        for v in (3, 2, 1, 0):
            if vf[v]['vol'] is not None:
                bs.append(vf[v]['vol'])
        for v in (3, 2, 1, 0):
            if vf[v]['per'] is not None:
                ws += be16(vf[v]['per'])
        cur = -12
        for v in (3, 2, 1, 0):
            if vf[v]['code'] in (2, 3):
                off = vf[v]['inst'] * 12 - cur
                if vf[v]['code'] == 2:
                    off += 6
                ws += be16(off)
                cur += off + 6
    put_code(bs, esc_rew)
    insts = instrument_table()
    h = b'LSP1' + be32(uid) + be16(ver)
    if layout == 'A':
        h += be16(125)
    elif layout == 'B':
        h += be16(125) + be16(esc_rew) + be16(esc_bpm) + be32(len(frames))
    else:
        flags = (1 if getpos else 0) | (2 if setpos else 0) | (4 if adpcm_info else 0)
        h += be16(flags) + be16(125) + be16(esc_rew) + be16(esc_bpm)
        if layout == 'D':
            h += be16(esc_pos)
        h += be32(len(frames))
        if adpcm_info:
            h += adpcm_info
    h += be16(len(insts))
    inst_off = len(h)
    h += pack_instruments(insts)
    h += be16(len(table)) + b''.join(be16(x) for x in table)
    seq_off = None
    if layout in 'CD':
        if setpos:
            h += be16(len(seqs))
            seq_off = len(h)
            for k in range(len(seqs)):
                wpos, bpos = seqpos[k]
                h += be32(wpos) + be32(bpos + len(ws))
        else:
            h += be16(0)
    h += be32(len(ws)) + be32(bloop) + be32(wloop)
    wstart = len(h)
    data = h + bytes(ws) + bytes(bs)
    return data, {'inst_off': inst_off, 'seq_off': seq_off, 'nseq': len(seqs) if setpos else 0,
                  'wstart': wstart, 'n': len(insts)}


# --------------------------------------------------------------- LSPm writer
def lspm(ver):
    frames, _, _, loop = tune(no_half=True)
    streams = [bytearray() for _ in range(16)]  # 0-3 per, 4-7 cmd, 8-11 vol, 12-15 inst
    for v in (3, 2, 1, 0):
        for f, vf in enumerate(frames):
            e = vf[v]
            cmd = 0
            if e['vol'] is not None:
                cmd |= 0x80
                streams[8 + v].append(e['vol'])
            if e['per'] is not None:
                cmd |= 0x40
                streams[0 + v] += be16(e['per'])
            if e['code'] == 3:
                cmd |= 0x20
                streams[12 + v].append(e['inst'])
            if v == 3:
                if loop > 0 and f == loop - 1:
                    cmd |= 2 << 3
                if f == len(frames) - 1:
                    cmd |= 3 << 3
            streams[4 + v].append(cmd)
    offs, o = [], 0
    for s in streams:
        offs.append(o)
        o += len(s)
    order = [4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 12, 13, 14, 15]
    insts = instrument_table()
    h = b'LSPm' + be16(ver)
    if ver >= 0x0118:
        h += be16(125)
    h += be16(len(insts))
    inst_off = len(h)
    h += pack_instruments(insts)
    h += b''.join(be32(offs[i]) for i in order)
    return h + b''.join(bytes(s) for s in streams), {'inst_off': inst_off, 'n': len(insts)}


# ------------------------------------------------------------ memory images
def relocate(music, info, layout, bank_addr, music_addr=0x10000, flag=True, insts=True,
             odd=False):
    m = bytearray(music)
    if flag and layout in 'CD':
        m[0x0A] = 0xFF
    if insts:
        tab = instrument_table()
        m[info['inst_off']:info['inst_off'] + 12 * len(tab)] = pack_instruments(tab, bank_addr, odd)
    if info.get('seq_off') and info['nseq']:
        wabs = music_addr + info['wstart']
        for k in range(info['nseq']):
            o = info['seq_off'] + 8 * k
            w, b = struct.unpack_from('>II', m, o)
            m[o:o + 8] = be32(w + wabs) + be32(b + wabs)
    return bytes(m)


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    files = {}

    def put(name, data):
        files[name] = data
        with open(os.path.join(out, name), 'wb') as f:
            f.write(data)

    variants = [
        ('synth_a', 'A', 0x0103, 0x5A5A0103, {}),
        ('synth_b', 'B', 0x0105, 0x5A5A0105, {}),
        ('synth_c', 'C', 0x010A, 0x5A5A010A, {}),
        ('synth_d', 'D', 0x0121, 0x5A5A0121, {'getpos': True, 'setpos': True}),
    ]
    infos = {}
    for name, layout, ver, uid, kw in variants:
        m, info = lsp1(layout, ver, uid, **kw)
        bf, _, _ = bank(uid)
        put(name + '.lsmusic', m)
        put(name + '.lsbank', bf)
        infos[name] = (m, info, layout)

    # ADPCM (1.30+): sample 0 kept lossless
    uid = 0x5A5AADCC
    bf, bmem, ainfo = bank(uid, adpcm_lossless={0})
    m, info = lsp1('D', 0x0121, uid, getpos=True, adpcm_info=ainfo)
    put('synth_adpcm.lsmusic', m)
    put('synth_adpcm.lsbank', bf)
    infos['synth_adpcm'] = (m, info, 'D')

    for name, ver in (('synth_m', 0x0121), ('synth_m120', 0x0114)):
        m, info = lspm(ver)
        bf, _, _ = bank(0x5A5A0E00 | (ver & 0xFF))
        put(name + '.lsmusic', m)
        put(name + '.lsbank', bf)
        infos[name] = (m, info, 'm')

    B = 0x2A400
    # 1.33 standard player: flag, instruments, sequence table
    m, info, lay = infos['synth_d']
    put('mem_d_std.bin', relocate(m, info, lay, B))
    # 1.30+ "insane" player: flag only; ADPCM bank decoded in place right after
    m, info, lay = infos['synth_adpcm']
    _, bmem, _ = bank(0x5A5AADCC, adpcm_lossless={0})
    put('mem_adpcm_insane.bin', relocate(m, info, lay, 0, insts=False) + bytes(16) + bmem)
    # 1.30+ standard player: flag, instruments, ADPCM bank decoded
    put('mem_adpcm_std.bin', relocate(m, info, lay, B) + bytes(16) + bmem)
    # 1.10 standard player
    m, info, lay = infos['synth_c']
    put('mem_c_std.bin', relocate(m, info, lay, B))
    # 1.04-1.05 player: bit 0 of each start set, then the bank added
    m, info, lay = infos['synth_b']
    put('mem_b_std.bin', relocate(m, info, lay, B, odd=True))
    # 1.00-1.03 player: no flag
    m, info, lay = infos['synth_a']
    put('mem_a_std.bin', relocate(m, info, lay, B))
    # micro player, in a dump of memory from address 0 whose music is at 1000
    # (where tests/newformats.py's embedded check puts it): bank right after
    m, info, lay = infos['synth_m']
    bf = files['synth_m.lsbank']
    put('mem_m_dump.bin', relocate(m, info, lay, 1000 + len(m), flag=False) + bf)
    for k in sorted(files):
        print(k, len(files[k]))


if __name__ == '__main__':
    main()
