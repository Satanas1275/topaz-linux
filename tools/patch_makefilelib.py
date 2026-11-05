#!/usr/bin/env python3
"""Add -I$(src) to the kernel's c_flags.

Several vendor trace headers (drivers/dma/qcom/bam_dma_trace.h,
drivers/clk/qcom/trace.h, drivers/gpu/drm/msm/disp/dpu1/dpu_trace.h, ...)
set TRACE_INCLUDE_PATH to ".", which makes include/trace/define_trace.h emit
#include "./foo_trace.h".  Nothing puts the file's own directory on the include
path, so those compiles fail.  Adding -I$(src) to c_flags fixes them all.
"""
p = '/home/satanas/linux-topaz/kernel-src/scripts/Makefile.lib'
s = open(p).read()
old = ('c_flags        = -Wp,-MMD,$(depfile) $(NOSTDINC_FLAGS) '
       '$(LINUXINCLUDE)     \\\n')
new = ('c_flags        = -Wp,-MMD,$(depfile) $(NOSTDINC_FLAGS) '
       '$(LINUXINCLUDE) -I$(src) \\\n')
if new in s:
    print('already patched')
else:
    assert old in s, 'c_flags pattern not found'
    open(p, 'w').write(s.replace(old, new, 1))
    print('patched', p)
