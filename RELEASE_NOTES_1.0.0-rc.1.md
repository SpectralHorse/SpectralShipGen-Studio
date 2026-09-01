# SpectralShipGen Studio v1.0.0-rc.1

SpectralShipGen Studio is the standalone SFML desktop application for generating, authoring, rerolling, inspecting, collecting, and animating ships produced by the separate SpectralShipGen C++ Library. `v1.0.0-rc.1` is the first public release candidate.

## Highlights

- Six focused workspaces: Generate, Profiles, Reroll, Inspect, Favorites, and Animation.
- Gallery generation with multiple independent Favorites from one batch.
- Structural, Faction, Palette, and Full Configuration authoring and export.
- Recipe-backed Favorites and portable recipe import/export.
- Selective deterministic rerolls.
- Semantic Inspect workspace.
- Experimental Animation Lab with IDLE, movement, FIRE composition, scrub/playback, and export.
- Separate statistical Diagnostics application.
- Contextual help and first-time-user documentation.

## Required Library

Studio numeric version: `1.0.0`.

Studio 1.0 supports **SpectralShipGen Library >=1.0.0,<2.0.0**. This release candidate is intended to pair specifically with **SpectralShipGen v1.0.0-rc.1**. Source/fetch release declarations pin that exact candidate rather than a moving branch.

The Studio and Library version independently after the coordinated initial 1.0 release.

## Persistence baselines

- Library recipes: schema v6.
- Studio user presets and exported Full Configurations: format v3.
- Favorites: format v1.
- Preview Preferences: format v2.
- Calibration sessions: format v2.
- Diagnostics `.shipdiag.json`: schema v2.
- Full Configuration files require `component_metadata`.

Pre-1.0 private development formats are intentionally unsupported. See [`COMPATIBILITY.md`](COMPATIBILITY.md).

## Generated output

Images, spritesheets, animation frames, and other output generated with SpectralShipGen or SpectralShipGen Studio may be used for any purpose, including commercial use. You may modify, redistribute, publish, sell, incorporate, or paint over generated output with **no attribution requirement**. The software licenses govern the software source, not the generated output.

If you make something interesting or funny with the generator, sharing it with the project is appreciated but entirely optional.

## Known limitations

- Animation Lab is an experimental 1.0 feature; animation behavior and authoring ergonomics may evolve in later compatible releases.
- Studio currently stores its automatic local presets, Favorites, and preferences relative to the application's working directory; keep the executable's working directory stable if you want that local state to follow the same portable folder.
- The first prebuilt portable distribution target is Windows x64. Other supported development platforms may build from source but are not promised a first-party binary package for this RC.
- Exact generated pixels are tied to the exact SpectralShipGen Library release revision, not merely the Studio version or readable recipe schema.

## Getting started

Start with [`docs/USER_GUIDE.md`](docs/USER_GUIDE.md), especially the two-minute Quick Start and the distinction between Profiles, Full Configuration, Recipe, and Favorite.
