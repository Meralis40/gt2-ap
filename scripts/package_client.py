#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Packs a built client for a release: the executable, its licenses and a short README.

  package_client.py --binary <gt2ap> --build-dir <cmake build dir> --platform windows-x86
                    --version <version> --out-dir <dir>

Prints the archive's path. Windows gets a .zip, the rest a .tar.gz, which keeps the mode bits.
"""
import argparse
import pathlib
import shutil
import tempfile

REPOSITORY = "https://github.com/Meralis40/gt2-ap"

THIRD_PARTY = {
    "SDL3.txt": "_deps/sdl3-src/LICENSE.txt",
    "mbedTLS.txt": "_deps/mbedtls-src/LICENSE",
    "nlohmann-json.txt": "_deps/nlohmann_json-src/LICENSE.MIT",
}

LINUX_NOTE = """
This is a 32-bit program. It needs the 32-bit X11 and PulseAudio libraries, which on Ubuntu are:

  sudo dpkg --add-architecture i386
  sudo apt update
  sudo apt install libx11-6:i386 libxext6:i386 libxcursor1:i386 libxi6:i386 libxfixes3:i386 \\
      libxrandr2:i386 libxss1:i386 libxrender1:i386 libxkbcommon0:i386 libpulse0:i386
"""


def readme(version, platform):
    tag = "main" if version.startswith("dev") else f"v{version}"
    note = LINUX_NOTE if platform.startswith("linux") else ""
    return f"""Gran Turismo 2 native client for Archipelago, {version} ({platform})

No game data comes with this program. You need your own NTSC-U 1.2 Simulation disc (SCUS-94488),
as a .cue with raw 2352-byte sectors.

Run gt2ap. The first time, it asks for your disc image and remembers it; you can also give it on
the command line:

  gt2ap "<your disc>.cue"

It opens on the Archipelago page. Choose Play offline to play without a server. The settings,
the memory card and the saves folder are written to the current folder.

With a gamepad, the left stick steers. In a session, Tab or a click of the right stick pauses and
shows what you have unlocked.

Setup guide: {REPOSITORY}/blob/{tag}/world/gt2/docs/setup_en.md
{note}
License: GPL-2.0-or-later (LICENSE), with the libraries in third_party.
Source: {REPOSITORY}/tree/{tag}
The libraries' versions and sources are in port/CMakeLists.txt there. mbedTLS is used under its
GPL-2.0-or-later option.
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=pathlib.Path, required=True)
    parser.add_argument("--build-dir", type=pathlib.Path, required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--out-dir", type=pathlib.Path, required=True)
    args = parser.parse_args()

    root = pathlib.Path(__file__).resolve().parent.parent
    name = f"gt2ap-{args.version}-{args.platform}"
    args.out_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as temp:
        folder = pathlib.Path(temp, name)
        (folder / "third_party").mkdir(parents=True)
        shutil.copy2(args.binary, folder)
        shutil.copy2(root / "LICENSE", folder)
        (folder / "README.txt").write_text(readme(args.version, args.platform), encoding="utf-8", newline="\n")
        for target, source in THIRD_PARTY.items():
            shutil.copy2(args.build_dir / source, folder / "third_party" / target)
        kind = "zip" if args.platform.startswith("windows") else "gztar"
        print(shutil.make_archive(str(args.out_dir / name), kind, temp, name))


if __name__ == "__main__":
    main()
