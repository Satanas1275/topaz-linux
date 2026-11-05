#!/usr/bin/env python3
"""Make the -I$(src) addition safe when $(src) is empty.

The modfinal pass builds with no $(src), so a bare "-I" would swallow the next
flag and turn a header into an extra input file ("cannot specify '-o' with
'-c' with multiple files").  Fall back to "." in that case.
"""
p = '/home/satanas/linux-topaz/kernel-src/scripts/Makefile.lib'
s = open(p).read()
old = '$(LINUXINCLUDE) -I$(src) \\\n'
new = '$(LINUXINCLUDE) -I$(if $(src),$(src),.) \\\n'
if new in s:
    print('already patched')
else:
    assert old in s, 'c_flags pattern not found'
    open(p, 'w').write(s.replace(old, new, 1))
    print('patched', p)
