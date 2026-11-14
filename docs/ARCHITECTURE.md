# Architecture

Union is the native main-thread core: Winsock UDP, session/sequence validation, reliable ordered events, bounded queues, world authority, local-player exclusion, actor lifecycle, inventory transactions and recovery. Movement snapshots remain replaceable. No engine access occurs on a network worker thread.

The host owns shared NPC/world decisions. Each PC owns its local character and UI language. Confirmed transaction IDs, commit/abort records and save-bound checkpoints prevent blind inventory replay. Journals remain format 4, recovery checkpoints format 2, personal-character archives format 1; protocol is 11.

The own Ninja layer provides versioned bounded script calls. ABI 2 supports probe, merchant preparation, corpse rune cleanup, confirmed local quest XP and local journal language. Native bindings and command state are restored after calls; unsupported/reentrant operations fail closed. See [Union/Ninja contract](UNION-NINJA.md).

English/German UI uses stable keys, local settings and shared build resources. Optional module activation requires exact supported evidence; the full content fingerprint still includes optional archives. Matching optional profiles are required on both PCs, while UI languages may differ.

Native addresses, API signatures, base scripts and assets are version-bound. Other engines, TCs and language-specific script baselines are not automatically compatible. Implementation does not establish native acceptance; see [test plan](TESTPLAN-CAVALORN.md).
