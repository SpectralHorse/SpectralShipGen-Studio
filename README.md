# SpectralShipGen Studio

SpectralShipGen Studio 1.0.0 is the standalone SFML application for generating, authoring, inspecting, bookmarking, rerolling, and animating ships produced by the separate [SpectralShipGen](https://github.com/SpectralHorse/SpectralShipGen) C++ library.

The repositories are intentionally separate:

```text
SpectralShipGen Studio
    -> SpectralShipGen
```

Studio owns the user interface, local authoring/persistence, Favorites workflow, file interaction, SFML rendering, and statistical Diagnostics application. The Library remains independently usable and SFML-independent.

## First time using SpectralShipGen Studio?

Start with the **[User Guide](docs/USER_GUIDE.md)**. Its two-minute Quick Start explains the Structural/Faction/Palette mental model, the six workspaces, Profile range sliders, Reroll, Favorites, Animation, and the difference between a Profile, Full Configuration, Recipe, and Favorite.

For the Windows x64 portable package:

1. extract the ZIP to a normal writable folder;
2. double-click `SpectralShipGenStudio.exe`;
3. use **Generate** and press **Space** for your first ship;
4. keep the extracted folder as the working folder if you want its local Favorites/presets/preferences to travel together.

**1920×1080 or higher is recommended.** Studio remains functionally usable at lower resolutions down to 1280×720, although some of the smallest helper text becomes difficult to read. The Studio window is resizable/maximizable and preserves the 1640×1000 logical interface proportions.

The portable build statically links SFML. **Microsoft Visual C++ 2015–2022 Redistributable (x64) may be required** if the matching Microsoft runtime is not already installed. See [Windows x64 Portable Distribution](docs/WINDOWS_PORTABLE.md).

## Development requirements

- CMake 3.20 or newer for the tracked Visual Studio presets
- C++17 compiler
- SFML 2.6.x for the Studio and Diagnostics application targets
- SpectralShipGen Library `>=1.0.0,<2.0.0`, normally from a sibling checkout, exact release-tag FetchContent path, or installed package

## Recommended local development layout

```text
Development/
    SpectralShipGen/
    SpectralShipGen-Studio/
```

The primary local-development path is the editable sibling Library checkout.

### Visual Studio / CMake Presets

Open this repository as a CMake project in Visual Studio and select the tracked `sibling-vs2022` configure preset. It resolves the Library from:

```text
${sourceDir}/../SpectralShipGen
```

Command-line equivalents are:

```text
cmake --preset sibling-vs2022
cmake --build --preset sibling-debug
```

or for Release:

```text
cmake --build --preset sibling-release
```

`CMakeUserPresets.json` is intentionally ignored so developers can add machine-local overrides without editing tracked presets.

### Direct CMake configuration

You can also point Studio at any separate local Library checkout explicitly:

```text
cmake -S . -B build \
  -DSPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR=<path-to-SpectralShipGen>
cmake --build build --target SpectralShipGenStudio
```

On Windows with a multi-config generator, select the desired configuration with the usual `--config Debug` or `--config Release` argument.

### Optional Library FetchContent fallback

If no local Library path is supplied, Studio can fetch the exact public Library release dependency:

```text
cmake -S . -B build \
  -DSPECTRAL_SHIP_GEN_FETCH_LIBRARY=ON
```

An explicitly supplied `SPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR` always takes priority over the fetch option. For Studio `v1.0.0`, the fetch declaration is pinned to Library `v1.0.0`; it does not track a moving branch.

### Installed Library package

When neither a source checkout nor the Library FetchContent fallback is selected, Studio uses the normal installed-package interface:

```text
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=<SpectralShipGen-install-prefix> \
  -DSPECTRAL_SHIP_GEN_FETCH_LIBRARY=OFF
```

This resolves `find_package(SpectralShipGen 1.0.0 CONFIG REQUIRED)` and links the same `SpectralShipGen::Core` / `SpectralShipGen::Diagnostics` targets used by source integration. No Library source include path is required.

The installed-package contract is SpectralShipGen `>=1.0.0,<2.0.0`, implemented through the Library package's `SameMajorVersion` rule. Studio and Library version independently after their coordinated initial 1.0 release.

Studio's SFML mechanism is independent: `SPECTRAL_SHIP_GEN_FETCH_SFML` controls the existing SFML 2.6.x FetchContent path.

## CI and Library integration

Studio CI obtains SpectralShipGen as a second public repository rather than copying Library implementation into Studio. Cross-repository jobs pin the immutable Library `v1.0.0` tag and use read-only repository access; no private Library PAT is required.

- `.github/workflows/studio-ci.yml` covers Windows and Linux source-checkout integration plus a Linux installed-package integration job.
- `.github/workflows/studio-sanitizers.yml` runs the no-SFML Preview/model regression under Clang ASan+UBSan, keeping sanitizer focus on project-owned code.
- `.github/workflows/studio-long.yml` keeps the existing LONG suite on manual/weekly execution rather than ordinary push/PR CI.

The cross-repository steps require only `contents: read`, keep checkout credentials from persisting, and can run for external fork pull requests because both repositories are public and no repository secret is required.

## Main targets

- `SpectralShipGenStudio` — the six-workspace application.
- `SpectralShipGenStudioDiagnostics` — broad diagnostics/statistical dashboard.
- `SpectralShipGenStudioPreviewRegression` — unified Preview/Application regression runner.

Studio links the same public Library targets available to third-party consumers: `SpectralShipGen::Core` and `SpectralShipGen::Diagnostics`.

## The six workspaces

Use the top navigation or keys `1`–`6` to switch workspaces. Workspace switching shares one current Preview session and does not itself regenerate the ship.

### 1. Generate

The normal ship-generation workflow.

- configure Structural/Profile, Faction, Palette, Full Configuration, dimensions, and other normal generation controls;
- `Space` generates one ship;
- `Shift+Space` opens/generates Gallery;
- `Left` / `Right` move through generation History outside Gallery;
- `Ctrl+O` imports a generation recipe;
- `Ctrl+E` exports the current generation recipe;
- `B` bookmarks the exact current recipe where available.

Generate intentionally keeps animation presentation simple: the normal current ship plus IDLE convenience playback. Advanced animation inspection belongs in Animation.

### 2. Profiles

Author and manage reusable generation configuration.

Profiles supports four categories:

- Structural
- Faction
- Palette
- Full Configuration

Built-in entries are immutable starting points. Duplicate a built-in or saved item to create an editable local copy. The editors expose the public SpectralShipGen configuration values rather than a separate GUI-only generation model.

Useful shortcuts:

- `Ctrl+D` — duplicate selected Profiles item;
- `Ctrl+O` — import the active Profiles type;
- `Ctrl+E` — export the selected saved item.

### 3. Reroll

A non-destructive selective reroll workflow based on deterministic generation domains.

Choose the domains/attributes to change, then:

- `Space` — generate the configured reroll candidate;
- `Enter` — accept the current candidate.

The original recipe remains the base until the candidate is explicitly accepted. Opening a Favorite in Reroll does not reroll it automatically.

### 4. Inspect

Read-only semantic inspection of **one current ship**.

Inspect exposes semantic groups/views such as structure, composition, constraints, weapon/attachment information, Overlay/Isolate presentation, decision details, and effective generation seed information. It uses the shared current ship and does not regenerate it.

Inspect is intentionally different from `SpectralShipGenStudioDiagnostics`:

```text
Inspect
    one current ship and its retained generation/debug information

DiagnosticsApp
    broad statistical, calibration, benchmark, and generator-wide analysis
```

### 5. Favorites

A persistent collection browser for ships deliberately kept by the user.

Favorites are recipe-backed, self-contained entries rather than PNG-only thumbnails. Custom structural/faction/palette ships remain reproducible even if the local authoring presets used to create them are later deleted.

- arrows move the selection;
- `Enter` opens the selected Favorite in Generate;
- `Delete` starts/remains the explicit two-action removal confirmation;
- `Ctrl+E` exports the selected Favorite recipe;
- mouse actions can also open a Favorite in Inspect, Animation, or Reroll and export its image.

The browser is paginated at 25 Favorites per page; the entire persistent collection remains reachable.

### 6. Animation

The experimental Animation Lab for the shared current ship.

It exposes the existing Core animation system:

- IDLE;
- MOVE_LEFT / MOVE_RIGHT;
- MOVE_UP / MOVE_DOWN;
- FIRE;
- movement posture + transient FIRE composition;
- normalized semantic-time scrubber;
- adaptive frame-count information;
- semantic phase display;
- application-only playback speeds;
- current-frame/spritesheet export where supported.

Keyboard:

- `Space` — play/pause;
- `Left` / `Right` — previous/next sampled frame while paused.

FIRE is composed over the current movement posture through the Core coordinator; Studio does not create combined clip types such as `MOVE_LEFT_FIRE`.

## Global input/help model

The simplified global controls are:

```text
1-6    Switch workspaces
F1     Contextual Help
Esc    Back / Cancel / Close overlay
B      Bookmark current ship when available
```

`Esc` never directly quits the application. At the root it is a no-op; in contextual states it closes/releases the current context. Normal application termination uses the window close button or the platform equivalent such as Alt+F4.

Keyboard shortcuts are suppressed when an editor/text field owns keyboard focus.

## Gallery workflow

Gallery is part of Generate and shows a batch of deterministic candidate recipes.

- **Left-click** a candidate to make it the primary Gallery selection.
- **Right-click** a candidate to bookmark/unbookmark it immediately.
- Multiple Gallery candidates can therefore be kept as Favorites before choosing the primary ship.
- `Enter` accepts the highlighted primary candidate as the shared current ship.

Existing Gallery candidates are immutable recipe snapshots. Changing generation configuration while Gallery is open changes the pending configuration for the **next explicit Gallery generation**, not the recipes/images already displayed.

Single-current-ship actions such as current Save/recipe export and History navigation remain contextually disabled while Gallery itself is active.

## Full Configuration vs Recipe vs Favorite

These are deliberately different concepts:

### Full Configuration / Bundle

A reusable authoring bundle containing:

```text
Structural profile
+ Faction profile
+ Palette configuration
```

It answers: **what kind of ships should I generate?**

### Recipe

A self-contained definition of one specific reproducible generated ship, including deterministic seed/configuration state.

It answers: **how do I reproduce this exact ship?**

### Favorite

A persistent Studio collection entry whose canonical saved state is a public SpectralShipGen recipe.

It answers: **which exact ships did I choose to keep?**

Opening or rerolling a Favorite never silently mutates the stored Favorite. Bookmark the new result separately if you want to keep it.

## Local profiles, bundles, Favorites, and preferences

Studio currently stores its persistent local authoring state relative to the application's working directory:

- `spectral_ship_gen_preview_user_presets.json` — Structural, Faction, Palette, and Full Configuration saved items;
- `spectral_ship_gen_preview_favorites.json` — recipe-backed Favorites collection;
- `spectral_ship_gen_preview_preferences.json` — Preview preferences.

Individual authoring exports use the current `.shipgenpreset.json` / `.shipgenbundle.json` formats. Generation recipes are exported/imported as `.shipgen.json` by the current Studio workflow.

User preset/export format v3 and Favorites format v1 are public 1.0 Studio user-data baselines. Preview Preferences v2 is local convenience state. Public SpectralShipGen recipes remain portable and independent of the local preset database. See [`COMPATIBILITY.md`](COMPATIBILITY.md) for the exact persistence promises.

## Recipe and profile import/export

Generate owns exact ship recipe import/export. Profiles owns reusable authoring-item import/export. Keeping those workflows separate prevents reusable configuration bundles from being confused with exact generated-ship recipes.

## Diagnostics application

`SpectralShipGenStudioDiagnostics` uses the Library's reusable diagnostics backend but is a separate SFML application owned by this repository. Use it for broad statistical/generator-wide analysis rather than inspecting one current ship.

## Release version and compatibility

The Studio numeric CMake version and first public release tag are `1.0.0` / `v1.0.0`. Studio and the Library use independent Semantic Versioning after their coordinated initial release.

See [`COMPATIBILITY.md`](COMPATIBILITY.md) for the supported Library range and Studio persistence contract, and [`CHANGELOG.md`](CHANGELOG.md) / [`RELEASE_NOTES_1.0.0.md`](RELEASE_NOTES_1.0.0.md) for the 1.0 release summary.

## Generated output rights

Images, spritesheets, animation frames, and other output generated with SpectralShipGen or SpectralShipGen Studio may be used for **any purpose**, including commercial use, with **no attribution requirement**. You may modify, redistribute, publish, sell, incorporate, or paint over generated output. The software licenses apply to the software source, not generated output.

Sharing interesting/funny generated results with the project is appreciated but entirely optional.

## Procedural artwork and AI-assisted development

Development/coding was AI-assisted. The generated ship artwork itself is **not produced by an image-generation model**: ships are generated by explicit deterministic C++ procedural systems. No reference images are supplied to an image model and no hidden base sprites are chopped/recombined to produce the generated ships.

## Windows x64 portable release

The supported first-party binary package target is `SpectralShipGen-Studio-1.0.0-Windows-x64.zip`. The validated Windows package is x64, statically links SFML, and does not require `sfml-*.dll` files beside the executable. The normal Release build dynamically uses the Microsoft C++ runtime, so **Microsoft Visual C++ 2015–2022 Redistributable (x64) may be required** on systems where it is not already installed.

On Windows x64, build target `SpectralShipGenStudioPortableZip` after configuring the normal supported SFML build. See [`docs/WINDOWS_PORTABLE.md`](docs/WINDOWS_PORTABLE.md) for the exact package contents, working-directory behavior, and runtime assumptions.

## Library API documentation

For C++ generation/configuration/recipe/animation examples, see the separate [SpectralShipGen Library repository](https://github.com/SpectralHorse/SpectralShipGen).

## License

SpectralShipGen Studio is licensed under the **Mozilla Public License 2.0 (MPL-2.0)**. See [`LICENSE`](LICENSE).

The repository convention is to provide the MPL-2.0 notice through the root `LICENSE` file rather than duplicating the Exhibit A notice in every project-owned source file. Studio consumes the separately zlib-licensed SpectralShipGen Library and the zlib/png-licensed SFML dependency; see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

