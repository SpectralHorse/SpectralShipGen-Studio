# SpectralShipGen Studio User Guide

## What Studio is

SpectralShipGen Studio is a desktop workspace for exploring and authoring the deterministic ship generator provided by the separate SpectralShipGen Library.

The most useful mental model is:

```text
Structural Profile = design / silhouette language
Faction Profile    = technology / material / cultural language
Palette            = color language

those three together describe what kind of ships to generate

Recipe              = one exact generated ship state
Favorite            = one exact recipe you chose to keep
```

A seed is not meaningful by itself. For exact reproduction, keep the recipe/configuration, deterministic seed state, and the **exact SpectralShipGen Library release revision**. Within that same Library revision, the same validated semantic input + seed state produces the same ship pixels.

## Two-minute first ship

1. Start Studio. You begin in **Generate**.
2. Leave the default Structural Profile, Faction Profile, and Palette selected for the first run.
3. Press **Space** to generate a ship.
4. Press **Shift+Space** to generate a Gallery batch when you want several alternatives at once.
5. In Gallery, **left-click** a candidate to make it the primary selection. **Right-click** any candidates you want to keep as Favorites; you can favorite several from the same batch.
6. Press **Enter** to accept the highlighted Gallery candidate as the current ship.
7. Press **B** to bookmark the current ship when bookmarking is available.
8. Press **4** to open **Inspect** and see what was actually generated.
9. Press **6** to open **Animation**; press **Space** to play/pause the current animation.
10. Press **F1** in any workspace for contextual help. **Esc** backs out of the current context; it never directly quits the application.

Normal application exit is the window close button or the platform equivalent such as **Alt+F4**.

## The three generation languages

### Structural Profile — structural/design language

Structural Profile controls the ship's form: silhouette tendencies, wings, cockpit, engine placement, weapon/attachment capacity, negative space, complexity, and related structural behavior.

Built-in Structural Profiles are starting points. In **Profiles**, duplicate a built-in or saved item when you want an editable local version rather than changing the built-in.

Use Structural Profile when your thought is closer to:

> "I want heavier ships with broader hulls and more attachment capacity."

### Faction Profile — technology/material/cultural language

Faction Profile changes how the structural design is expressed: material/finish tendencies, weapon language, cockpit/engine preferences, surface detail, livery, palette tendencies, hierarchy, and faction animation response.

Use Faction Profile when your thought is closer to:

> "Keep this kind of silhouette, but make the ships feel like they belong to a different civilization or military culture."

### Palette — color language

Palette controls color independently from Structural and Faction choices. Studio supports faction-derived color behavior, an explicit generated palette profile, or a fixed semantic palette depending on the selected configuration.

Use Palette when your thought is simply:

> "I like the design language, but I want a different color treatment."

## The six workspaces

Use the navigation bar or number keys **1–6**. Switching workspaces keeps the same shared current ship; switching alone does not regenerate it.

### 1 — Generate

Generate is the normal starting point.

Use it to:

- choose the active Structural/Faction/Palette/Full Configuration;
- generate one ship with **Space**;
- generate Gallery with **Shift+Space**;
- move through History with **Left/Right** when Gallery is not active;
- import a recipe with **Ctrl+O**;
- export the current recipe with **Ctrl+E**;
- bookmark the current recipe with **B** when available.

Gallery candidates are immutable recipe snapshots. If you edit pending generation settings while Gallery is open, existing candidates do not silently change; the edits apply to the next explicit Gallery generation.

### 2 — Profiles

Profiles is where reusable authoring lives.

Categories:

- Structural;
- Faction;
- Palette;
- Full Configuration.

Useful shortcuts:

- **Ctrl+D** — duplicate selected item;
- **Ctrl+O** — import the active profile/bundle type;
- **Ctrl+E** — export the selected saved item.

Built-ins are immutable starting points. Duplicate one before editing.

### 3 — Reroll

Reroll answers questions such as:

> "I like this ship except its weapons."

> "Keep the silhouette and colors, but try the details again."

> "Give me another palette without changing geometry."

Select the deterministic generation domains/attributes you want to change, then:

- **Space** generates a candidate;
- **Enter** accepts the current candidate.

Reroll is non-destructive until you accept. The original recipe remains the base. Opening a Favorite in Reroll does not automatically change the stored Favorite.

### 4 — Inspect

Inspect is read-only and describes **one current ship**. It exposes semantic structure/composition/constraint information, weapons and attachments, Overlay/Isolate presentation, generation decisions, and effective seed information where retained.

Inspect does not regenerate the ship and is not the same as DiagnosticsApp. Inspect explains the current ship; DiagnosticsApp studies generator-wide behavior statistically.

### 5 — Favorites

Favorites is your persistent collection of exact recipe-backed ships.

- Arrow keys move selection.
- **Enter** opens the selected Favorite in Generate.
- **Delete** uses an explicit two-action removal confirmation.
- **Ctrl+E** exports the selected Favorite recipe.
- Mouse actions can open a Favorite in Inspect, Animation, or Reroll and can export its image.

Favorites are not PNG-only bookmarks. They keep the self-contained recipe, so custom profile values remain reproducible even if the local profile entry that originally produced the ship is later removed.

### 6 — Animation

Animation Lab exposes the current Library animation model for the shared ship:

- IDLE;
- MOVE_LEFT / MOVE_RIGHT;
- MOVE_UP / MOVE_DOWN;
- FIRE;
- FIRE composed over the current movement posture;
- normalized-time scrubbing;
- adaptive sampled frame information;
- playback speeds;
- frame/spritesheet export where available.

Keyboard:

- **Space** — play/pause;
- **Left/Right** — previous/next sampled frame while paused.

Animation Lab is experimental in 1.0; it is intended for inspection/export of the current generated animation rather than a general-purpose animation editor.

## A normal workflow

A practical loop is:

```text
Generate a few ships
    -> like the direction?
       -> Profiles to refine the design languages
    -> like one exact ship except one aspect?
       -> Reroll selected domains
    -> want to understand it?
       -> Inspect
    -> want to keep it?
       -> Favorite it
    -> want motion/output?
       -> Animation
```

You do not need to visit every workspace for every ship.

## Profiles vs Full Configuration vs Recipe vs Favorite

### Profile

A Structural, Faction, or Palette profile is one reusable **part** of generation authoring. It answers one aspect of "what kind of ships should I generate?"

### Full Configuration

A Full Configuration (bundle) combines:

```text
Structural profile
+ Faction profile
+ Palette configuration
```

It is reusable authoring state. It does **not** represent one exact generated ship and therefore does not replace a Recipe.

### Recipe

A Recipe is a self-contained definition of **one specific reproducible generated ship state**, including deterministic seed/configuration state.

Use a recipe when you want to move or reproduce an exact ship.

### Favorite

A Favorite is a Studio collection entry backed by a Recipe. It means "this is an exact ship I chose to keep."

Opening or rerolling a Favorite does not silently overwrite it. Keep a changed result by bookmarking that result separately.

## Saving and exporting

Studio uses different formats for different jobs:

- `.shipgen.json` — exact public SpectralShipGen Recipe;
- `.shipgenpreset.json` — exported Structural/Faction/Palette authoring item;
- `.shipgenbundle.json` — exported Full Configuration;
- image export — generated pixels for use outside Studio;
- Animation workspace — current frame/spritesheet export where supported.

Automatic local state currently lives relative to Studio's working directory:

- `spectral_ship_gen_preview_user_presets.json`;
- `spectral_ship_gen_preview_favorites.json`;
- `spectral_ship_gen_preview_preferences.json`.

For a portable setup, keep a stable working directory alongside the executable or explicitly preserve those files when moving the application folder.

## Generated-output rights

Images, spritesheets, animation frames, and other output generated with SpectralShipGen or SpectralShipGen Studio may be used **for any purpose**, including commercial use.

You may modify, redistribute, publish, sell, incorporate into games/applications/media, paint over, manually edit, or use the output as placeholders. **No attribution is required.**

The zlib/MPL software licenses apply to the software source, not the generated output.

If you create an interesting or funny result, showing it to the project is appreciated, but that is only a friendly request and is never a condition of use.

## AI clarification

Development of SpectralShipGen and Studio was AI-assisted. The generated spaceship artwork itself is **not AI-image-generated**: generation is performed by the native deterministic C++ generator. The generator does not use reference images, base sprites, or an image-generation model to create ship output.

## Keyboard reference

Global:

```text
1-6          Switch workspaces
F1           Contextual Help
Esc          Back / Cancel / Close overlay; never quits directly
B            Bookmark current ship when available
```

Generate:

```text
Space        Generate one ship
Shift+Space  Generate Gallery
Left/Right   History navigation outside Gallery
Ctrl+O       Import recipe
Ctrl+E       Export current recipe
```

Profiles:

```text
Ctrl+D       Duplicate selected item
Ctrl+O       Import active profile/bundle type
Ctrl+E       Export selected saved item
```

Reroll:

```text
Space        Generate reroll candidate
Enter        Accept candidate
```

Favorites:

```text
Arrows       Move selection
Enter        Open selected Favorite in Generate
Delete       Begin/confirm removal
Ctrl+E       Export selected Favorite recipe
```

Animation:

```text
Space        Play/pause
Left/Right   Previous/next sampled frame while paused
```

Keyboard shortcuts are suppressed while an editor/text field owns keyboard focus.

## Common questions

### Why did the same seed produce a different ship after upgrading?

The deterministic guarantee is scoped to the **same exact SpectralShipGen Library release revision** plus the same semantic configuration and seed state. Cross-release pixel identity is not automatically guaranteed. Keep the exact Library release/tag/revision when pixel-perfect reproduction matters.

### Why can't I edit a built-in Profile directly?

Built-ins are stable immutable starting points. Duplicate the item and edit your local copy.

### Why didn't changing settings alter the Gallery ships already on screen?

Gallery candidates are recipe snapshots. Your change configures the **next** Gallery generation; it does not rewrite existing candidates.

### Why is my Favorite unchanged after Reroll?

Favorites are deliberately non-destructive. Reroll works from the Favorite's recipe, but the saved Favorite changes only if you separately keep/bookmark the new result.

### Recipe or Full Configuration?

Use **Recipe** for one exact ship. Use **Full Configuration** for reusable Structural + Faction + Palette authoring.

### Inspect or DiagnosticsApp?

Use **Inspect** for one current ship. Use **DiagnosticsApp** for broad statistical/calibration/generator analysis.

### Why doesn't Esc quit?

This is intentional. `Esc` backs out of contexts and closes overlays. Use the window close button or normal platform close action such as Alt+F4 to exit.
