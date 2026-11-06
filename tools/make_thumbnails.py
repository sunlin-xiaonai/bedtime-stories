#!/usr/bin/env python3
"""Make small RGB565 story covers from the app's existing cover art."""
from pathlib import Path
import subprocess

art = Path(__file__).resolve().parent.parent / 'main' / 'art'
panel = (0x20 >> 3) << 11 | (0x2A >> 2) << 5 | (0x23 >> 3)
background = panel.to_bytes(2, 'little')

for name in ('plant', 'book', 'animal', 'train', 'cat'):
    image = bytearray((art / f'{name}.bin').read_bytes())
    if len(image) != 160 * 140 * 2:
        raise ValueError(f'{name}: unexpected cover size')
    for i in range(0, len(image), 2):
        if image[i:i + 2] == b'\0\0':
            image[i:i + 2] = background
    result = subprocess.run(
        ['ffmpeg', '-v', 'error', '-f', 'rawvideo', '-pixel_format', 'rgb565le',
         '-video_size', '160x140', '-i', 'pipe:0', '-frames:v', '1',
         '-vf', 'scale=52:46:flags=area', '-f', 'rawvideo', '-pix_fmt', 'rgb565le', 'pipe:1'],
        input=image, stdout=subprocess.PIPE, check=True,
    ).stdout
    if len(result) != 52 * 46 * 2:
        raise ValueError(f'{name}: unexpected thumbnail size')
    (art / f'{name}_thumb.bin').write_bytes(result)
