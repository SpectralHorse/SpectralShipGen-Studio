# SpectralShipGen Studio

SpectralShipGen Studio is the standalone SFML application for generating, authoring, inspecting, bookmarking, rerolling, and animating ships produced by the separate [SpectralShipGen](https://github.com/SpectralHorse/SpectralShipGen) C++ library.

The repositories are intentionally separate:

```text
SpectralShipGen Studio
    -> SpectralShipGen
```

Studio owns the user interface, local authoring/persistence, Favorites workflow, file interaction, SFML rendering, and statistical Diagnostics application. The Library remains independently usable and SFML-independent.

## Requirements

- CMake 3.20 or newer for the tracked Visual Studio presets
- C++17 compiler
- SFML 2.6.x for the Studio and Diagnostics application targets
- SpectralShipGen Library, normally from a sibling checkout or the optional FetchContent path

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

If no local Library path is supplied, Studio can fetch the private Library repository using normal Git authentication:

```text
cmake -S . -B build \
  -DSPECTRAL_SHIP_GEN_FETCH_LIBRARY=ON
```

An explicitly supplied `SPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR` always takes priority over the fetch option.

### Installed Library package

When neither a source checkout nor the Library FetchContent fallback is selected, Studio uses the normal Task-103 package interface:

```text
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=<SpectralShipGen-install-prefix> \
  -DSPECTRAL_SHIP_GEN_FETCH_LIBRARY=OFF
```

This resolves `find_package(SpectralShipGen CONFIG REQUIRED)` and links the same `SpectralShipGen::Core` / `SpectralShipGen::Diagnostics` targets used by source integration. No Library source include path is required.

Studio's SFML mechanism is independent: `SPECTRAL_SHIP_GEN_FETCH_SFML` controls the existing SFML 2.6.x FetchContent path.

## CI and private Library access

Studio CI deliberately obtains SpectralShipGen as a second repository rather than copying Library implementation into Studio. While the Library is private, GitHub Actions uses the Studio repository secret `SPECTRAL_SHIP_GEN_CI_TOKEN` solely for read access to `SpectralHorse/SpectralShipGen`. No credential belongs in CMake or source.

- `.github/workflows/studio-ci.yml` covers Windows and Linux source-checkout integration plus a Linux installed-package integration job.
- `.github/workflows/studio-sanitizers.yml` runs the no-SFML Preview/model regression under Clang ASan+UBSan, keeping sanitizer focus on project-owned code.
- `.github/workflows/studio-long.yml` keeps the existing LONG suite on manual/weekly execution rather than ordinary push/PR CI.

Cross-repository jobs intentionally avoid `pull_request_target`. Pull requests whose source context cannot receive the private Library-read secret do not run privileged cross-repository integration; this can be simplified once the Library becomes public.

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

These files are Studio-local convenience state. Public SpectralShipGen recipes remain portable and independent of the local preset database.

## Recipe and profile import/export

Generate owns exact ship recipe import/export. Profiles owns reusable authoring-item import/export. Keeping those workflows separate prevents reusable configuration bundles from being confused with exact generated-ship recipes.

## Diagnostics application

`SpectralShipGenStudioDiagnostics` uses the Library's reusable diagnostics backend but is a separate SFML application owned by this repository. Use it for broad statistical/generator-wide analysis rather than inspecting one current ship.

## Library API documentation

For C++ generation/configuration/recipe/animation examples, see the separate [SpectralShipGen Library repository](https://github.com/SpectralHorse/SpectralShipGen).

## License

SpectralShipGen Studio is licensed under the **Mozilla Public License 2.0 (MPL-2.0)**. See [`LICENSE`](LICENSE).

The repository convention is to provide the MPL-2.0 notice through the root `LICENSE` file rather than duplicating the Exhibit A notice in every project-owned source file. Studio consumes the separately zlib-licensed SpectralShipGen Library and the zlib/png-licensed SFML dependency; see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

