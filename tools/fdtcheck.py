#!/usr/bin/env python3
"""Host-side check of the shim's fdt_add_framebuffer(): insert a
simple-framebuffer node into a DTB and verify the result with fdtget."""
import struct
import subprocess
import sys

FDT_MAGIC = 0xD00DFEED
FDT_BEGIN_NODE = 1
FDT_END_NODE = 2
FDT_PROP = 3
FDT_NOP = 4
FDT_END = 9


def be32(b, o):
    return struct.unpack_from(">I", b, o)[0]


def wr32(b, o, v):
    struct.pack_into(">I", b, o, v)


def find_string(b, name):
    strings = be32(b, 12)
    size = be32(b, 32)
    off = 0
    want = name.encode() + b"\0"
    while off < size:
        end = b.index(b"\0", strings + off)
        if b[strings + off:end + 1] == want:
            return off
        off = end - strings + 1
    return None


def strings_add(b, name):
    off = find_string(b, name)
    if off is not None:
        return off
    ssize = be32(b, 32)
    total = be32(b, 4)
    strings = be32(b, 12)
    raw = name.encode() + b"\0"
    if strings + ssize != total:
        return None
    b[strings + ssize:strings + ssize + len(raw)] = raw
    wr32(b, 32, ssize + len(raw))
    wr32(b, 4, total + len(raw))
    return ssize


def root_cell(b, name):
    off = find_string(b, name)
    if off is None:
        return 0
    p = be32(b, 8)
    end = p + be32(b, 36)
    if be32(b, p) != FDT_BEGIN_NODE:
        return 0
    p = (p + 4 + len(b[p + 4:b.index(b"\0", p + 4)]) + 1 + 3) & ~3
    while p < end:
        tok = be32(b, p)
        if tok == FDT_END_NODE:
            break
        if tok == FDT_PROP:
            ln = be32(b, p + 4)
            noff = be32(b, p + 8)
            if noff == off and ln == 4:
                return be32(b, p + 12)
            p = (p + 12 + ln + 3) & ~3
            continue
        break
    return 0


def root_end(b):
    p = be32(b, 8)
    end = p + be32(b, 36)
    depth = 0
    while p < end:
        tok = be32(b, p)
        p += 4
        if tok == FDT_BEGIN_NODE:
            depth += 1
            p = (p + len(b[p:b.index(b"\0", p)]) + 1 + 3) & ~3
        elif tok == FDT_END_NODE:
            depth -= 1
            if depth == 0:
                return p - 4
        elif tok == FDT_PROP:
            p = (p + 8 + be32(b, p) + 3) & ~3
        elif tok == FDT_NOP:
            pass
        else:
            break
    return None


def put_u32be(v):
    return struct.pack(">I", v)


def put_u64be(v, cells):
    if cells == 2:
        return struct.pack(">II", v >> 32, v & 0xFFFFFFFF)
    return struct.pack(">I", v)


def put_prop(nameoff, val):
    b = put_u32be(FDT_PROP) + put_u32be(len(val)) + put_u32be(nameoff) + val
    return b + b"\0" * ((4 - len(b) % 4) % 4)


def add_framebuffer(dtb, addr, size, w, h, stride):
    b = bytearray(dtb)
    ac = root_cell(b, "#address-cells")
    sc = root_cell(b, "#size-cells")
    print("root cells: #address-cells=%d #size-cells=%d" % (ac, sc))
    assert 0 < ac <= 2 and 0 < sc <= 2
    names = ["compatible", "reg", "width", "height", "stride", "format"]
    off = []
    for n in names:
        o = strings_add(b, n)
        assert o is not None, "no room for " + n
        off.append(o)

    node = put_u32be(FDT_BEGIN_NODE)
    node += ("framebuffer@%x" % addr).encode() + b"\0"
    node += b"\0" * ((4 - len(node) % 4) % 4)
    node += put_prop(off[0], b"simple-framebuffer\0")
    node += put_prop(off[1], put_u64be(addr, ac) + put_u64be(size, sc))
    node += put_prop(off[2], put_u32be(w))
    node += put_prop(off[3], put_u32be(h))
    node += put_prop(off[4], put_u32be(stride))
    node += put_prop(off[5], b"a8r8g8b8\0")
    node += put_u32be(FDT_END_NODE)

    struct_off = be32(b, 8)
    struct_size = be32(b, 36)
    total = be32(b, 4)
    insert = root_end(b)
    assert insert, "no root end"
    stroff = be32(b, 12)
    if stroff >= insert:
        wr32(b, 12, stroff + len(node))
    b[insert + len(node):insert + len(node) + (total - insert)] = b[insert:total]
    b[insert:insert + len(node)] = node
    wr32(b, 36, struct_size + len(node))
    wr32(b, 4, total + len(node))
    return bytes(b)


if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    out = add_framebuffer(open(src, "rb").read(), 0x5C000000, 0xA00000,
                          1080, 2400, 4320)
    open(dst, "wb").write(out)
    print("wrote %s (%d bytes)" % (dst, len(out)))
    for args in (["fdtget", dst, "/", "compatible"],
                 ["fdtget", dst, "/framebuffer@5c000000", "compatible"],
                 ["fdtget", dst, "/framebuffer@5c000000", "format"],
                 ["fdtget", dst, "/framebuffer@5c000000", "width"]):
        r = subprocess.run(args, capture_output=True, text=True)
        print(" ".join(args[1:]), "->", (r.stdout or r.stderr).strip())
