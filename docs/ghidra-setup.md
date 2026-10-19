# Ghidra setup

Reverse-engineering environment for phase 1: Ghidra, the PS1 loader, and an MCP server so Claude can work in the same Ghidra project.

## Versions (checked 2026-10-19)

| Component | Version | Why |
|---|---|---|
| [Ghidra](https://github.com/NationalSecurityAgency/ghidra/releases) | **12.1.3** | Newest version supported by both plugins below |
| [ghidra_psx_ldr](https://github.com/lab313ru/ghidra_psx_ldr/releases) | 2026.10.03 | PS-X EXE loader, PsyQ signatures, GTE instructions, overlays |
| [ReVa](https://github.com/cyberkaida/reverse-engineering-assistant/releases) | v7.3.1 | MCP server inside Ghidra; tools built for LLM use; no Python bridge |
| JDK | 21 (64-bit) | Required by Ghidra 12. Amazon Corretto 21 is fine |

Extensions are built for one exact Ghidra version, so the file names below must match `12.1.3`.

Alternative: [GhidraMCP](https://github.com/LaurieWired/GhidraMCP) 2.0 works too, but only on Ghidra 12.1.4, where ReVa has no build yet.

## 1. JDK 21

Check with `java -version`. If it isn't 21, install a JDK 21 (e.g. Eclipse Temurin or Amazon Corretto). Ghidra asks for the JDK path on its first start if it can't find one.

## 2. Ghidra 12.1.3

1. Download `ghidra_12.1.3_PUBLIC_20260817.zip` from the Ghidra releases page.
2. Unzip it to a folder **without spaces**, e.g. `E:\tools\ghidra_12.1.3_PUBLIC`.
3. Start it with `ghidraRun.bat` (Windows) or `./ghidraRun` (Linux/macOS).

## 3. Extensions

Download:
- `ghidra_12.1.3_PUBLIC_20261004_ghidra_psx_ldr.zip` (ghidra_psx_ldr release 2026.10.03)
- `ghidra_12.1.3_PUBLIC_20260903_reverse-engineering-assistant.zip` (ReVa release v7.3.1)

Install both. Don't unzip them:

1. In the Ghidra project window: **File → Install Extensions**.
2. Click **+**, pick each zip, check both are ticked.
3. Restart Ghidra.

Enable ReVa (it needs to be turned on in two places):

1. Project window: **File → Configure**, click the plug icon (top right), tick **ReVa Application Plugin**.
2. Open any program in the Code Browser (the dragon icon): **File → Configure**, plug icon, tick **ReVa Plugin**. Then **File → Save Tool** so it stays enabled.

By default ReVa listens on `localhost:8080` (changeable in the project window's tool options). It includes PyGhidra scripting tools. Keep it on localhost only.

## 4. Ghidra project

Create the project **outside this repository**, e.g. `E:\re\gt2\gt2.gpr`: it holds the game's code, which must never be committed.

Extract the executables with our tool:

```bash
gt2-discinfo extract "Simulation.cue" SCUS_944.88 E:/re/gt2/SCUS_944.88
gt2-discinfo extract "Arcade.cue" SCUS_944.55 E:/re/gt2/SCUS_944.55
```

Import `SCUS_944.88` (**File → Import File**). The format should be detected as **PSX executable**. Run auto-analysis with the defaults. The PSX loader applies PsyQ signatures, which names most SDK functions.

Overlays come later, once we know which files they are and where they load.

## 5. Connecting Claude (MCP)

With Ghidra open on the project, ReVa serves MCP at `http://localhost:8080/mcp/message`.

This repository's `.mcp.json` declares it as project-scoped server `ReVa`, so Claude Code sessions started in this folder offer to connect to it (you approve it once). The equivalent command is:

```bash
claude mcp add --scope project --transport http ReVa http://localhost:8080/mcp/message
```

Ghidra must be running with the project open before the session starts; otherwise the server shows as failed. Start a new session after opening Ghidra.
