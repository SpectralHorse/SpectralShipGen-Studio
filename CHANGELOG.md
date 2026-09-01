# Changelog

All notable public changes to SpectralShipGen Studio are recorded here beginning with the 1.0 compatibility epoch.

## [1.0.0-rc.1] - Release candidate

Initial public release candidate.

### Workspaces

- **Generate** — single generation, Gallery batches, History, recipe import/export, and bookmarking.
- **Profiles** — Structural, Faction, Palette, and Full Configuration authoring/import/export, with bounded integer sliders and dual MIN/MAX range controls for faster editing.
- **Reroll** — selective deterministic domain rerolls with explicit candidate acceptance.
- **Inspect** — semantic inspection of the current generated ship.
- **Favorites** — persistent recipe-backed collection browser and multi-Favorite Gallery workflow.
- **Animation** — Animation Lab for IDLE, movement, FIRE composition, scrubbing, playback, and export.

### Desktop usability

- Native resizable/maximizable Studio window while preserving the fixed 1640×1000 logical UI coordinate space.
- Aspect-preserving letterbox/pillarbox presentation and centralized scaled mouse mapping.
- Initial Windows window size fits the available desktop work area where the normal client size would not fit.
- Windows validation establishes 1920×1080-or-higher as recommended, with functional usability down to 1280×720 and a small-text readability caveat.
- Windows x64 portable documentation records static SFML linkage and the possible Microsoft Visual C++ 2015–2022 Redistributable (x64) prerequisite.

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
