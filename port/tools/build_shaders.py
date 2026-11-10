#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compiles the hardware renderer's shaders (port/hw/shaders/*.hlsl) for SDL_GPU, to SPIR-V
for Vulkan and DXIL for Direct3D 12, into port/hw/shaders/gen/<name>.h. The headers are
checked in, so building the game needs no shader compiler: run this after changing a shader.

It needs dxc twice: one that writes SPIR-V (the Vulkan SDK's), and one that signs DXIL (the
Windows SDK's, beside dxil.dll). Give them with --spirv-dxc and --dxil-dxc if they aren't
found where those SDKs put them.
"""
import argparse
import glob
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SHADERS = os.path.join(HERE, '..', 'hw', 'shaders')
STAGES = [('vertex', 'VertexMain', 'vs_6_0'), ('fragment', 'FragmentMain', 'ps_6_0')]


def newest(pattern):
    found = sorted(glob.glob(pattern))
    return found[-1] if found else None


def default_spirv_dxc():
    sdk = os.environ.get('VULKAN_SDK')
    if sdk:
        return os.path.join(sdk, 'Bin', 'dxc.exe' if os.name == 'nt' else 'dxc')
    return newest('C:/VulkanSDK/*/Bin/dxc.exe') or 'dxc'


def default_dxil_dxc():
    return newest('C:/Program Files (x86)/Windows Kits/10/bin/10.*/x64/dxc.exe')


def compile_one(dxc, source, entry, profile, spirv):
    with tempfile.TemporaryDirectory() as tmp:
        out = os.path.join(tmp, 'out.bin')
        args = [dxc, '-nologo', '-T', profile, '-E', entry, '-O3', '-Fo', out, source]
        if spirv:
            args[1:1] = ['-spirv', '-fspv-target-env=vulkan1.0']
        else:
            args[1:1] = ['-Wno-ignored-attributes']
        subprocess.run(args, check=True)
        with open(out, 'rb') as f:
            return f.read()


def as_array(name, data):
    lines = [f'static const unsigned char {name}[{len(data)}] = {{']
    for i in range(0, len(data), 16):
        lines.append('    ' + ', '.join(f'0x{b:02x}' for b in data[i:i + 16]) + ',')
    lines.append('};')
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--spirv-dxc', default=default_spirv_dxc())
    parser.add_argument('--dxil-dxc', default=default_dxil_dxc())
    options = parser.parse_args()
    if options.dxil_dxc is None:
        sys.exit('no dxc for DXIL: give --dxil-dxc')
    os.makedirs(os.path.join(SHADERS, 'gen'), exist_ok=True)
    for source in sorted(glob.glob(os.path.join(SHADERS, '*.hlsl'))):
        name = os.path.splitext(os.path.basename(source))[0]
        parts = [
            '// SPDX-License-Identifier: GPL-2.0-or-later',
            f'// Generated from {name}.hlsl by port/tools/build_shaders.py. Don\'t edit.',
            '#pragma once',
            '',
        ]
        for stage, entry, profile in STAGES:
            for fmt, dxc, spirv in (('spirv', options.spirv_dxc, True), ('dxil', options.dxil_dxc, False)):
                data = compile_one(dxc, source, entry, profile, spirv)
                parts.append(as_array(f'k_{name}_{stage}_{fmt}', data))
                parts.append('')
        header = os.path.join(SHADERS, 'gen', name + '.h')
        with open(header, 'w', newline='\n') as f:
            f.write('\n'.join(parts))
        print('wrote', os.path.relpath(header))


if __name__ == '__main__':
    main()
