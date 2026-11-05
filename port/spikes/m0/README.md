# M0 spike

Throwaway test of the port's riskiest assumptions, on the host toolchains:

- a generated assembly file binds `extern` names to absolute guest addresses;
- guest RAM can be reserved at `0x80000000` and the scratchpad at `0x1F800000`;
- a 24-bit OT link resolves with `link | 0x80000000`;
- libc `setjmp`/`longjmp` work from the trampoline loop the overlay dispatch needs;
- a jump into guest RAM (a missed code pointer) faults and reports the address.

`spike` exits 0 only if every check passes.
