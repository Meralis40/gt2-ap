-- SPDX-License-Identifier: GPL-2.0-or-later
-- Measures how deep the stack goes, to confirm the AP state block (docs/ap-hooks.md) is never
-- written by the game.
--
-- The PsyQ heap is empty on GT2 (Heap_InitDefault, 0x8008ddb4), so everything from 0x801f0d70
-- to the stack top (0x801ffff0) is stack only. This fills the unused part below the current sp
-- with a pattern, then reports on every vsync the lowest address the game has overwritten.
--
-- Usage: boot the Simulation disc, load this script once the title screen shows, then play
-- through as many screens as possible (GT mode menus, races, license tests, replays, memory
-- card save/load, movies). The reported low-water mark must stay above the AP block end.

local AP_BLOCK_START = 0x801f0d80
local AP_BLOCK_END = 0x801f1d80
local FILL_START = 0x801f0d70 -- after the InitHeap header at 0x801f0d60
local PATTERN = 0xa5

local mem = PCSX.getMemPtr()
local function off(addr) return bit.band(addr, 0x1fffff) end

local sp = PCSX.getRegisters().GPR.n.sp
local fillEnd = sp - 0x100
for addr = FILL_START, fillEnd - 1 do mem[off(addr)] = PATTERN end
print(string.format("stack_watermark: filled %08x-%08x (sp %08x), AP block %08x-%08x",
                    FILL_START, fillEnd, sp, AP_BLOCK_START, AP_BLOCK_END))

local lowest = fillEnd
local lowestSp = sp

StackWatermarkListener = PCSX.Events.createEventListener('GPU::Vsync', function()
    local cur = PCSX.getRegisters().GPR.n.sp
    if cur < lowestSp and cur >= FILL_START then lowestSp = cur end
    -- Scan up from the bottom; the first overwritten byte is the low-water mark.
    for addr = FILL_START, lowest - 1 do
        if mem[off(addr)] ~= PATTERN then
            lowest = addr
            local status = addr < AP_BLOCK_END and "OVERLAPS AP BLOCK" or "ok"
            print(string.format("stack_watermark: lowest write %08x, lowest sp seen %08x (%s)",
                                addr, lowestSp, status))
            break
        end
    end
end)
