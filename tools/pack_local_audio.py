#!/usr/bin/env python3
"""Convert local audio into the app's numbered Opus clip format using FFmpeg."""

import argparse
import io
from pathlib import Path
import re
import subprocess


def pack_opus(stream):
    clip, packet = bytearray(), bytearray()
    headers = iter((b"OpusHead", b"OpusTags"))
    while page := stream.read(27):
        if len(page) != 27 or not page.startswith(b"OggS\0"):
            raise ValueError("FFmpeg returned an invalid Ogg page")
        lengths = stream.read(page[26])
        if len(lengths) != page[26]:
            raise ValueError("FFmpeg returned a truncated Ogg page")
        for length in lengths:
            part = stream.read(length)
            if len(part) != length:
                raise ValueError("FFmpeg returned a truncated Ogg packet")
            packet.extend(part)
            if length == 255:
                continue
            header = next(headers, None)
            if header is not None:
                if not packet.startswith(header):
                    raise ValueError("FFmpeg returned an invalid Opus header")
            else:
                if not 0 < len(packet) <= 1276:
                    raise ValueError("FFmpeg returned an unsupported Opus packet")
                clip.extend(len(packet).to_bytes(2, "little"))
                clip.extend(packet)
                if len(clip) > 2 * 1024 * 1024:
                    raise ValueError("audio exceeds 2 MiB after conversion; split it into shorter files")
            packet.clear()
    if packet or not clip:
        raise ValueError("no complete Opus clip returned by FFmpeg")
    return clip


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path, help="a filename such as 8000.bin")
    args = parser.parse_args()
    if not args.input.is_file():
        parser.error("input must be a local audio file")
    if not re.fullmatch(r"[0-9]{4}\.bin", args.output.name):
        parser.error("output filename must be four digits followed by .bin")
    if args.output.exists():
        parser.error("output already exists; choose an unused filename")
    try:
        ogg = subprocess.check_output(
            ["ffmpeg", "-nostdin", "-v", "error", "-protocol_whitelist", "file,pipe",
             "-i", str(args.input.resolve()), "-map", "0:a:0", "-map_metadata", "-1",
             "-ac", "1", "-ar", "16000", "-c:a", "libopus", "-b:a", "16k",
             "-frame_duration", "20", "-f", "opus", "pipe:1"]
        )
        clip = pack_opus(io.BytesIO(ogg))
        with args.output.open("xb") as file:
            file.write(clip)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f"{exc}\n")


if __name__ == "__main__":
    main()
