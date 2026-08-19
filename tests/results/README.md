# Hardware run results

One file per (model, firmware build, board), written by `make test-hardware`.
These are the record of what was verified on what; the matrix in
`../../HARDWARE_TESTING.md` is generated from them by
`tools/gen_hardware_matrix.awk` and should never be edited by hand.

Committing these files is the point: a contributor running the suite on a board
we do not own sends back one new file, which cannot conflict with anyone else's
results, and the table regenerates deterministically with `make hardware-matrix`
— no hardware required.

Format: tab-separated, one record per line, first field is the record type.
`M` is run metadata (model, firmware, build, board, interface, baud, level,
contributor, date). `T` is one test outcome (name, outcome, summary, detail),
where outcome is one of `pass`, `fail`, `unsupported`, `discrepancy`,
`skip-build`, `skip-level`.
