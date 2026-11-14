# Alpha.17 recovery fixtures

Recovery17Store.bin and Recovery17Checkpoint.bin were emitted by the actual
unchanged Alpha.17 Recovery.h serializer from the preserved pre-Alpha.18 source,
not by reconstructing the old format in the new test. The tiny producer and
build project are retained at work/alpha18-legacy in the project workspace.
The fixed committed transaction ID is 32 times `1`; item 100, amount 3, chest
XARDAS_CHEST. The checkpoint and journal share the generated profile identity.

WorldTests.inl checks old-reader input compatibility and version-two rewrite,
identity/decision preservation, priced trades and corrupt-checkpoint rejection.
These files are synthetic fixtures, not user savegames. Native loading remains
an explicit later game test.

Recovery18Store.bin and Recovery18Checkpoint.bin were subsequently emitted by
the actual unchanged Alpha.18 serializer from the pre-Alpha.19 source snapshot.
They contain a committed merchant sale with currency/price and both effect IDs.
SessionTests.inl checks migration into journal format 3 alongside quest rewards.
The fixture producer is not required at runtime; only these synthetic compatibility inputs are distributed.
