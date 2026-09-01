# Changelog

All notable public changes to SpectralShipGen Studio are recorded here beginning with the 1.0 compatibility epoch.

## [1.0.0-rc.1] - Release candidate

Initial public release candidate.

### Workspaces

- **Generate** — single generation, Gallery batches, History, recipe import/export, and bookmarking.
- **Profiles** — Structural, Faction, Palette, and Full Configuration authoring/import/export.
- **Reroll** — selective deterministic domain rerolls with explicit candidate acceptance.
- **Inspect** — semantic inspection of the current generated ship.
- **Favorites** — persistent recipe-backed collection browser and multi-Favorite Gallery workflow.
- **Animation** — Animation Lab for IDLE, movement, FIRE composition, scrubbing, playback, and export.

### Authoring and persistence

- User preset library/export format v3.
- Preview Preferences format v2.
- Calibration session format v2.
- Full Configuration `component_metadata` required.
- Recipe-backed Favorites preserve custom configuration independently of the local preset database.

### Diagnostics

- `SpectralShipGenStudioDiagnostics` provides the SFML visualization/application layer over the Library diagnostics backend.

### Release contract

- Studio numeric version `1.0.0` with independent Semantic Versioning after the coordinated initial release.
- Studio 1.0 supports SpectralShipGen Library `>=1.0.0,<2.0.0`.
- Initial RC source/CI dependency declaration pins Library `v1.0.0-rc.1` rather than a moving branch.
- First-time-user guide and explicit generated-output rights policy added for the public RC.

Pre-public development history is intentionally summarized rather than reproduced as an internal task diary.
