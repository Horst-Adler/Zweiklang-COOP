# Publication boundary

Only own source, tools, documentation, example configuration and explicitly hashed synthetic fixtures are allowed. Original games, SDK binaries, foreign mods/archives, private website files, real saves/logs and credentials are excluded.

`.gitignore` excludes common private/build data. `publication-allowlist.json` enumerates every approved path; synthetic fixtures are pinned by hash. `scripts/Audit-Publication.py --staged` checks the complete index, file modes, binary signatures, secret/private-data patterns and Windows case collisions. Index and working files must match exactly before publication.

The own Ninja VDF is built from own sources; no foreign archive is bundled into this repository. Local transformed comfort archives remain private. English is the public primary language; German companion text and explicit German translations/history are clearly secondary. A renamed repository does not create a release or publish a productive website.
