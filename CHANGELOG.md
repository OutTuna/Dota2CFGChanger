# Changelog

## Unreleased

No stable application version was manually changed. Alpha branch CI publishes isolated prereleases; stable latest is untouched.

### Safety

- Make the intended static MSVC runtime effective through CMake 3.15 policy behavior; verify standalone EXE dependencies before alpha artifact upload.

- Verify staged configuration contents and recheck source/destination state before replacement.
- Serialize writes to a destination across application processes; retain stale locks for explicit crash recovery.
- Reject non-regular filesystem entries, linked account directories and Windows reparse points at checked components; compare Windows path components without case sensitivity.
- Check available disk space before staging and backup creation; preserve existing rollback and verified-backup behavior.
- Report invalid backup metadata rather than silently hiding it.
- Reject overflowing/invalid Steam IDs, malformed asset sizes and prereleases.
- Reserve download workspaces, reject linked cached updates, enforce HTTPS redirects and bound metadata responses.
- Bound avatar/profile input and avatar decoding dimensions; report unreadable settings/caches.

### Features and resource use

- Read custom themes from the settings directory, falling back to the executable's folder, and open them correctly from non-ASCII paths on Windows.
- Show the `userdata` folder next to backups taken from another folder.
- Try the next Linux folder picker when one fails, and lock path fields while profile names load.
- Remove unreachable hard-coded theme copies; the embedded JSON themes were always used.
- Allow a failed release publication to be retried after its version tag was pushed.
- Delete downloaded updates that are not newer than the running version at startup.
- Show updater errors through the shared error dialog, which finds `zenity` or `kdialog` on `PATH`.
- Keep the window responsive while the Linux folder picker is open.
- Pin `linuxdeploy` and its AppImage plugin to fixed releases with SHA-256 checks.
- Compare configuration state by content, so a timestamp-only change no longer cancels a confirmed transfer.
- List and restore an account's backups after its Steam `userdata` folder has moved; the target must still be that account's `570` directory.
- Allow copying and restoring while profile names are still loading after a scan.
- Search Linux font folders for DejaVu, Liberation, Noto, Adwaita, Open Sans, Roboto, Ubuntu and FreeSans on any distribution layout; locate Windows fonts through `WINDIR`.
- Report a missing Linux folder picker in the status line and accept `qarma` and `yad`; show an error when the window cannot be created.
- Background transfer preview with additions, replacements, unchanged paths, removals, file count and copy volume; revalidate before applying.
- Skip identical transfers and unnecessary backups.
- Search account lists, open account folders through a context menu, clip off-screen rows and enable keyboard navigation.
- Stop continuous redraw after the success opening animation; limit retained avatar pixel payload to 64 × 64 RGBA.

- Identify alpha builds in the window and Info; keep stable version comparisons separate.
- Publish alpha CI artifacts under unique prerelease tags with `--latest=false`; never replace stable `latest`.

### Verification and documentation

- Exercise native Windows junctions, symlinks, sharing violations, denied-read ACLs, three process-kill transfer stages and manual recovery; add real helper startup-failure rollback on test application copies.
- Make stale-preview metadata regression deterministic on NTFS and record native MSVC build/test and idle measurements.
- Validate process interruption after saving the old executable, installing the new file and confirming startup before cleanup; document manual recovery when both versions remain.

- Add safety, concurrency/crash-lock and large-list/preview layout regressions; extend fault-injection, updater, avatar and localization checks.
- Add portable/process-integration CTest labels and an ASan/UBSan workflow without changing release publication.
- Refresh the AUR compatibility patch and checksums; keep existing package/application versions.
- Document recovery, update trust/signing requirements, measured performance and outstanding release gates.
