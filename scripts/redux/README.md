# PCSX-Redux scripts

Lua scripts for reverse engineering and the phase 2 prototype, run inside [PCSX-Redux](https://github.com/grumpycoders/pcsx-redux).

| Script | Purpose |
|---|---|
| [`stack_watermark.lua`](stack_watermark.lua) | Reports the lowest stack address the game writes, to confirm the AP state block in [ap-hooks.md](../../docs/ap-hooks.md) is safe. |
