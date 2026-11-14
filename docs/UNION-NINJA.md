# Union/Ninja module contract

Union remains the authority for networking, ownership, transactions, persistence and safety. Ninja 4.1.16 loads our own complementary script VDF. Its virtual patch root and `Patch_ZweiklangCore_*` symbols are legacy compatibility identifiers retained for existing saves; the runtime artifact is now `OreboundCoop_Core.vdf`.

ABI 2 verifies a scalar ABI constant, a zero-argument Dispatch function and scalar Command/Value/Result variables. Commands: 0 probe, 1 merchant preparation, 2 corpse rune cleanup, 3 authorized local quest XP (1..1,000,000), 4 local language (0 English / 1 German). Other commands/arguments fail closed. Result is reset before calls; command state and native instance bindings are restored afterwards. A script command alone cannot authorize a transfer.

Language command 4 initializes three exported own journal strings, which Union copies to compatible legacy helpers. This is local state and is not replicated as a game rule. Existing historical journal entries are not rewritten.

Archive hashes, loaded script evidence and known optional bridge revisions are checked. Absent optional modules are allowed; unknown or renamed loaded variants are not silently treated as absent. Toolkit is not required by our own script module, but is conditionally required by the examined comfort modules. Both peers retain the complete matching-content requirement.

Native parser, menu language switching, merchants, quest XP and recovery must be accepted in the game on both PCs. Static archive/ABI tests and loopback UDP do not establish that acceptance.
