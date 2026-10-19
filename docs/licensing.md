# Licensing analysis

## Beetle PSX (libretro/beetle-psx-libretro)

Analysed at commit `df2b53b306ba52d2116f8e8cc93b9ee33d239be7` (2026-10-19) by classifying the header of every source file (607 files).

| Part | License (from headers / license files) | Notes |
|---|---|---|
| Top-level `COPYING` | GPLv2 text | No "or later" statement in the repo itself |
| Mednafen emulation core (`mednafen/psx/*.c`: CPU, GTE, GPU, SPU, CDC, MDEC, DMA, timers…; `mednafen/cdrom`, `state.c`, …) | GPL-2.0-or-later | Mednafen Team headers |
| `pgxp/` | GPL-2.0-or-later (8 files), no header (5) | |
| libretro glue (`libretro.c`, `input*.c`, `mc_async.c`, …), most of `rhi/` (renderer interface), many Mednafen headers | **No header** | Only covered by the top-level GPLv2 `COPYING` |
| `libretro-common/` | MIT | |
| `deps/openbios/` | MIT (PCSX-Redux authors) | |
| `parallel-psx/` (SPIR-V helpers) | ISC | |
| `parallel-psx/SPIRV-Cross`, Khronos Vulkan headers | **Apache-2.0** | Only built with `HAVE_VULKAN` |
| `deps/lightrec/` (dynarec) | LGPL-2 (Library GPL v2) | Only built with `HAVE_LIGHTREC` (default on in the Makefile) |
| `deps/lightning/` (GNU lightning, used by lightrec) | **LGPL-3.0-or-later** | Only built with `HAVE_LIGHTREC` |

### Why the sources disagree

- The emulation core really is **GPL-2.0-or-later**, which is what you saw in the headers.
- A large amount of libretro-written code has **no header at all**. Its only license grant is the bare GPLv2 `COPYING`. GPLv2 §9 says that if no version is specified you may pick any version, but shipping the v2 text is commonly read as "version 2". The safe reading for those files is **GPL-2.0-only**.
- Arch packages it as `GPL-2.0-only` (PKGBUILD `license=(GPL-2.0-only)`). That matches this conservative reading of the whole combined work. Most likely it was taken from `COPYING` and not from a per-file audit.
- So for the **combined work**, we treat Beetle PSX as **GPL-2.0-only**.

Side note: under a GPLv2-only reading, the default build is already in tension with its optional parts: Apache-2.0 (Vulkan path) and LGPLv3 (lightning) are not GPLv2-compatible. We avoid both.

## Decisions for this project

- **Our client code: GPL-2.0-or-later.** It can be combined with Beetle (the combined binary is then distributed under GPLv2), and it stays open to GPLv3 later, for example once the recompiled runtime replaces Beetle.
- **AP world: MIT**, like Archipelago.
- **While Beetle is linked in, every dependency must be GPLv2-compatible:**
  - Build Beetle **without lightrec/lightning**. Its interpreter is also simpler to hook at code addresses.
  - Build Beetle **without the Vulkan path** (SPIRV-Cross is Apache-2.0); use the OpenGL or software renderer.
  - TLS: **mbedTLS** under its GPL-2.0-or-later option, **not OpenSSL 3** (Apache-2.0).
  - SDL3 (zlib), Dear ImGui (MIT), apclientpp (MIT), asio/websocketpp (BSL/BSD) are fine.
- Re-run the header audit when we pin a Beetle commit for our fork.
