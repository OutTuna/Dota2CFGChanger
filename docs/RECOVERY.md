# Recovery after an interrupted transfer

Close Dota 2, Steam and every instance of Dota2CFGChanger first. Keep a separate copy
of all directories mentioned below before moving or removing anything. Do not
resolve an interruption while another transfer is running.

For a destination `<userdata>/<account>/570`, the sibling paths are:

| Path | Meaning |
| --- | --- |
| `570` | Installed configuration, if installation finished. |
| `570.dotamanager_tmp` | Prepared configuration; it is not proof of a successful commit. |
| `570.dotamanager_backup` | Previous configuration retained for rollback. |
| `570.dotamanager_lock` | An active operation or the marker left by an interrupted process. |

1. If `570` is missing and `570.dotamanager_backup` exists, preserve both recovery
   directories elsewhere, then rename `570.dotamanager_backup` to `570`. Do not
   install the temporary directory just because it exists.
2. If both `570` and `570.dotamanager_backup` exist, preserve both. Inspect the
   working configuration and the app's verified backup history before choosing
   which version to keep. Do not automatically delete either copy.
3. Once the previous configuration has been recovered and all copies needed for
   recovery have been saved, move any remaining `.dotamanager_tmp` and
   `.dotamanager_backup` directories out of the account directory.
4. Remove the **empty** `570.dotamanager_lock` directory only after verifying that
   no app instance is operating on the account. There is intentionally no automatic
   timeout that might mistake a slow operation for a dead process.
5. Reopen the app and generate a fresh preview before retrying.

Permanent verified backups are in the application configuration directory under
`backups/<account>/<timestamp>/`. Keep `backup.json` and `570/` together. Invalid
metadata or changed file hashes must be investigated; do not edit the manifest
just to make a damaged backup pass validation.

The lock coordinates application instances writing the same destination. It does
not lock out Steam, the game, administrators or other programs. Close Steam and
Dota 2 before transferring. Filesystem metadata checks and content verification
reduce accidental concurrent-edit risks; they are not an atomic snapshot of a
hostile or constantly changing filesystem. A process crash and a storage device
losing power have different durability guarantees; no power-loss guarantee has
been established.

## Interrupted application updates

Beside the executable, `.update-new` is the staged update and `.update-old` is the
previous executable. Close the app and updater before investigating. Preserve both
before manual recovery; if the executable is missing, recover `.update-old` to its
original name. Use the official release page to replace the executable if unsure.
Do not run an unverified `.update-new` file.

A failed download can leave `<asset>.download/` inside the versioned updates
cache. After closing all instances, preserve any diagnostic information and remove
that incomplete workspace before retrying. Never remove an active download.
