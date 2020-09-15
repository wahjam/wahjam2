#!/usr/bin/env python3
#
# $ ./backtrace.py 0x32000000 | c++filt <winedbg.backtrace
#
# Print symbols from a winedbg backtrace.  The mmap base address must be given
# (use 'info share' in winedbg).
#
# SPDX-License-Identifier: Apache-2.0

import sys
import subprocess

image = 'debug/vsttest1.dll'

mmap_base = int(sys.argv[1], 16)

# ImageBase               6b800000
image_base = None
result = subprocess.run(['i686-w64-mingw32.static.posix-objdump', '-p', image], stdout=subprocess.PIPE)
for line in result.stdout.decode().split('\n'):
    if line.startswith('ImageBase'):
        _, image_base = line.split()
        image_base = int(image_base, 16)
        break
if image_base is None:
    print('unable to determine ImageBase')
    sys.exit(1)

def resolve(addr):
    addr = int(addr, 16)
    addr = addr - mmap_base + image_base
    result = subprocess.run(['i686-w64-mingw32.static.posix-addr2line', '-p', '-f', '-s', '--exe={0}'.format(image), hex(addr)], stdout=subprocess.PIPE)
    return '{0} ({1})'.format(result.stdout.decode().strip(), hex(addr))

if len(sys.argv) == 3:
    print(resolve(sys.argv[2]))
else:
    #  9 0x4007c8bf in vsttest1 (+0xd9c8be) (0x0b05f720)
    for line in sys.stdin:
        line = line.strip()
        frame, pc, _, module, _, _ = line.split()
        if module != 'vsttest1':
            print(line)
            continue
        pretty = resolve(pc)
        print('{0} {1}'.format(frame, pretty))
