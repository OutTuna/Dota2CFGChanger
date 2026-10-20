# Changelog

## Unreleased — local audit changes

No stable application version was manually changed. Alpha branch CI publishes isolated prereleases; stable latest is untouched.

### Safety

- Verify staged configuration contents and recheck source/destination state before replacement.
- Serialize writes to a destination across application processes; retain stale locks for explicit crash recovery.
- Reject non-regular filesystem entries, linked account directories and Windows reparse points at checked components; compare Windows path components without case sensitivity.
- Check available disk space before staging and backup creation; preserve existing rollback and verified-backup behavior.
- Report invalid backup metadata rather than silently hiding it.
- Reject overflowing/invalid Steam IDs, malformed asset sizes and prereleases.
- Reserve download workspaces, reject linked cached updates, enforce HTTPS redirects and bound metadata responses.
- Bound avatar/profile input and avatar decoding dimensions; report unreadable settings/caches.

### Features and resource use

- Background transfer preview with additions, replacements, unchanged paths, removals, file count and copy volume; revalidate before applying.
- Skip identical transfers and unnecessary backups.
- Search account lists, open account folders through a context menu, clip off-screen rows and enable keyboard navigation.
- Stop continuous redraw after the success opening animation; limit retained avatar pixel payload to 64 × 64 RGBA.

- Identify alpha builds in the window and Info; keep stable version comparisons separate.
- Publish alpha CI artifacts under unique prerelease tags with `--latest=false`; never replace stable `latest`.

### Verification and documentation

- Add safety, concurrency/crash-lock and large-list/preview layout regressions; extend fault-injection, updater, avatar and localization checks.
- Add portable/process-integration CTest labels and an ASan/UBSan workflow without changing release publication.
- Refresh the AUR compatibility patch and checksums; keep existing package/application versions.
- Document recovery, update trust/signing requirements, measured performance and outstanding release gates.
