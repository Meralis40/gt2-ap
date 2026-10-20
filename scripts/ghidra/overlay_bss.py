# SPDX-License-Identifier: GPL-2.0-or-later
# Gives each GT2.OVL overlay space its own copy of the BSS.
#
# Overlay_ClearBss zeroes the whole BSS on every overlay switch, so each overlay
# uses that range for its own variables. This script adds an uninitialized
# "<overlay>_bss" block to every overlay address space, then moves the overlay's
# references into it. Labels, data and comments at addresses that only one
# overlay uses move with them. Boot and resident code keep the main-space view.
#
# Run it from the Script Manager, or with ReVa's run-script, after the overlays
# are loaded (see docs/ghidra-setup.md). It can be run again: blocks that exist
# are kept and references already moved are skipped.
#
# @category GT2
# @runtime PyGhidra

from collections import defaultdict

from ghidra.program.model.symbol import SourceType

# Executable name: (BSS start, BSS end inclusive, first resident address).
LAYOUTS = {
    "SCUS_944.88": (0x800a8d5c, 0x801c93af, 0x8005d600),  # Simulation, checked against Overlay_ClearBss
    "SCUS_944.55": (0x800a8b04, 0x801c917f, 0x8005d5c0),  # Arcade, from the EXE header, not tested
}
OVERLAY_BASE = 0x80010000
DRY_RUN = False
COMMENT_TYPES = range(5)  # EOL, PRE, POST, PLATE, REPEATABLE


def overlay_spaces(program):
    spaces = []
    for space in program.getAddressFactory().getAddressSpaces():
        if space.isOverlaySpace() and program.getMemory().getBlock(space.getAddressInThisSpaceOnly(OVERLAY_BASE)):
            spaces.append(space)
    return spaces


def owner(address, resident_start):
    space = address.getAddressSpace()
    if space.isOverlaySpace():
        return space.getName()
    return "resident" if address.getOffset() >= resident_start else "boot"


def ensure_block(program, space, bss_start, bss_end):
    memory = program.getMemory()
    start = space.getAddressInThisSpaceOnly(bss_start)
    if memory.getBlock(start) is not None:
        return False
    block = memory.createUninitializedBlock(space.getName().lower() + "_bss", start, bss_end - bss_start + 1, False)
    block.setRead(True)
    block.setWrite(True)
    block.setExecute(False)
    block.setComment(space.getName() + " BSS: cleared by Overlay_ClearBss on every overlay switch")
    return True


def snapshot_users(program, bss_start, bss_end, resident_start):
    """Maps each main-space BSS address to the set of code owners that reference it."""
    refs = program.getReferenceManager()
    ram = program.getAddressFactory().getDefaultAddressSpace()
    end = ram.getAddress(bss_end)
    users = defaultdict(set)
    it = refs.getReferenceDestinationIterator(ram.getAddress(bss_start), True)
    while it.hasNext():
        to = it.next()
        if to.compareTo(end) > 0:
            break
        for ref in refs.getReferencesTo(to):
            users[to.getOffset()].add(owner(ref.getFromAddress(), resident_start))
    return users


def move_markup(program, src, dst, stats):
    listing = program.getListing()
    symbols = program.getSymbolTable()
    data = listing.getDefinedDataAt(src)
    if data is not None:
        dtype, length = data.getDataType(), data.getLength()
        listing.clearCodeUnits(src, src.add(length - 1), False)
        try:
            listing.createData(dst, dtype, length)
            stats["data"] += 1
        except Exception:
            stats["data failed"] += 1
    for symbol in list(symbols.getSymbols(src)):
        if symbol.getSource() == SourceType.DEFAULT:
            continue
        name, namespace, source, primary = symbol.getName(), symbol.getParentNamespace(), symbol.getSource(), symbol.isPrimary()
        symbol.delete()
        moved = symbols.createLabel(dst, name, namespace, source)
        if primary:
            moved.setPrimary()
        stats["labels"] += 1
    for kind in COMMENT_TYPES:
        comment = listing.getComment(kind, src)
        if comment:
            listing.setComment(dst, kind, comment)
            listing.setComment(src, kind, None)
            stats["comments"] += 1


def retarget(program, ref, space, stats):
    refs = program.getReferenceManager()
    source, ref_type, op, primary = ref.getSource(), ref.getReferenceType(), ref.getOperandIndex(), ref.isPrimary()
    to = space.getAddressInThisSpaceOnly(ref.getToAddress().getOffset())
    refs.delete(ref)
    if ref.isOffsetReference():
        base = space.getAddressInThisSpaceOnly(ref.getBaseAddress().getOffset())
        new = refs.addOffsetMemReference(ref.getFromAddress(), base, True, ref.getOffset(), ref_type, source, op)
    elif ref.isShiftedReference():
        new = refs.addShiftedMemReference(ref.getFromAddress(), to, ref.getShift(), ref_type, source, op)
    else:
        new = refs.addMemoryReference(ref.getFromAddress(), to, ref_type, source, op)
    if primary:
        refs.setPrimary(new, True)
    stats["references"] += 1


def migrate(program, space, users, bss_start, bss_end):
    stats = defaultdict(int)
    name = space.getName()
    ram = program.getAddressFactory().getDefaultAddressSpace()
    refs = program.getReferenceManager()
    for offset, owners in sorted(users.items()):
        if owners == {name}:
            move_markup(program, ram.getAddress(offset), space.getAddressInThisSpaceOnly(offset), stats)
        if name not in owners:
            continue
        for ref in list(refs.getReferencesTo(ram.getAddress(offset))):
            if ref.getFromAddress().getAddressSpace() == space:
                retarget(program, ref, space, stats)
    return dict(stats)


def shared_labels(program, users):
    """User labels left in main space at addresses that several overlays use."""
    symbols = program.getSymbolTable()
    ram = program.getAddressFactory().getDefaultAddressSpace()
    found = []
    for offset, owners in sorted(users.items()):
        overlays = sorted(o for o in owners if o not in ("boot", "resident"))
        if len(overlays) < 2:
            continue
        for symbol in symbols.getSymbols(ram.getAddress(offset)):
            if symbol.getSource() != SourceType.DEFAULT:
                found.append((offset, symbol.getName(True), overlays))
    return found


def run(program):
    layout = LAYOUTS.get(program.getName())
    if layout is None:
        print("Unknown executable %s; add its BSS range to LAYOUTS." % program.getName())
        return
    bss_start, bss_end, resident_start = layout
    spaces = overlay_spaces(program)
    users = snapshot_users(program, bss_start, bss_end, resident_start)
    print("%d overlay spaces, %d referenced BSS addresses" % (len(spaces), len(users)))
    if DRY_RUN:
        for space in spaces:
            mine = sum(1 for owners in users.values() if space.getName() in owners)
            only = sum(1 for owners in users.values() if owners == {space.getName()})
            print("%s: %d addresses, %d used by it alone" % (space.getName(), mine, only))
    else:
        for space in spaces:
            created = ensure_block(program, space, bss_start, bss_end)
            print("%s: %s %s" % (space.getName(), "block created," if created else "block exists,",
                                 migrate(program, space, users, bss_start, bss_end)))
    for offset, label, overlays in shared_labels(program, users):
        print("Shared address 0x%08x keeps its main-space label %s; used by %s. Move it by hand to the overlay it describes."
              % (offset, label, ", ".join(overlays)))


tx = currentProgram.startTransaction("Per-overlay BSS")
ok = False
try:
    run(currentProgram)
    ok = True
finally:
    currentProgram.endTransaction(tx, ok)
