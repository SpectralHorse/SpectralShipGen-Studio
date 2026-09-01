# SpectralShipGen Studio 1.0 Compatibility Policy

SpectralShipGen Studio `1.0.0` begins the application's public compatibility epoch. The Studio and SpectralShipGen Library are separate products with independent Semantic Versioning after their coordinated initial 1.0 release.

## Studio version

The authoritative numeric application version is the root CMake `PROJECT_VERSION`, currently `1.0.0`. The initial RC tag candidate is `v1.0.0-rc.1`; the prerelease qualifier is release/tag metadata and is not part of CMake's numeric `VERSION` field.

## Supported SpectralShipGen Library

Studio 1.0 supports SpectralShipGen Library:

```text
>= 1.0.0 and < 2.0.0
```

Installed-package consumption requests `find_package(SpectralShipGen 1.0.0 CONFIG REQUIRED)` and relies on the Library package's `SameMajorVersion` compatibility rule.

For the initial release candidate, source/fetch/CI declarations pin the exact intended Library candidate `v1.0.0-rc.1`; they do not track a moving `master`/`main` branch. Before the tag exists, local RC validation uses the exact current Library RC source tree through `SPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR` rather than pretending a remote tag was fetched.

A future Studio release may raise its minimum supported Library version independently. A future Library 1.x release does not automatically require a matching Studio version bump if the Studio compatibility contract still holds.

## Portable/public user data

### Generation recipes

`.shipgen.json` files use the public SpectralShipGen recipe contract. Recipe schema v6 is the 1.0 baseline. Recipe compatibility is governed by the Library's [`COMPATIBILITY.md`](https://github.com/SpectralHorse/SpectralShipGen/blob/v1.0.0-rc.1/COMPATIBILITY.md); Studio must not reinterpret unsupported Library recipe schemas itself.

### User preset and Full Configuration exports

Studio user-preset library files and individual `.shipgenpreset.json` / `.shipgenbundle.json` exports use **format v3** as the 1.0 baseline. Full Configuration data requires `component_metadata`.

These are user-facing authoring formats. Once a persistence schema has shipped publicly in Studio 1.x, later Studio 1.x releases should continue to read it. Older Studio releases are not required to read newer schema versions. Unknown/incompatible future versions fail safely. Dropping readability of a public 1.x authoring schema would normally be a major-version compatibility event.

### Favorites

The persistent Favorites collection uses **format v1** at the 1.0 baseline and stores self-contained public recipes. Favorites are user data. Publicly shipped 1.x Favorites schemas should remain readable by later Studio 1.x releases under the same policy as other portable user data.

## Local application state

### Preview Preferences

Preview Preferences use **format v2** at the 1.0 baseline. Preferences are local convenience state rather than a portable interchange format. Studio should preserve readable public 1.x preference schemas where practical, but recovering from incompatible/corrupt local preferences may reasonably fall back to defaults rather than being treated like loss of a portable recipe or exported profile.

### Calibration sessions

Calibration sessions use **format v2** at the 1.0 baseline. Calibration is an advanced/developer-facing workflow rather than the primary portable user-data contract. Public 1.x readers should reject unsupported versions safely; long-term portability promises are narrower than for recipes, exported profiles, Full Configurations, and Favorites.

## Developer diagnostics data

Library diagnostics `.shipdiag.json` uses schema **v2** at the 1.0 baseline. It is developer/analysis data, not a replacement for recipes or exported Studio authoring data. Unsupported pre-1.0/future schemas fail safely.

## Pre-1.0 formats

Private development-era migration support removed before 1.0 remains intentionally absent. Pre-1.0 user preset, preferences, calibration, diagnostics, and other development formats are outside the public compatibility contract.

## Deterministic reproduction

For the same exact SpectralShipGen Library release revision, the same validated semantic recipe/configuration and deterministic seed state produce the same pixels. Studio persistence compatibility across versions does **not** imply cross-Library-release pixel identity. Preserve the exact Library release/tag/revision when exact reproduction matters.
