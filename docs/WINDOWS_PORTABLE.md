# Windows x64 Portable Distribution

The 1.0 release-candidate binary target is a portable Windows x64 ZIP named:

```text
SpectralShipGen-Studio-1.0.0-rc.1-Windows-x64.zip
```

The validated package contains:

```text
SpectralShipGen-Studio-1.0.0-rc.1-Windows-x64/
    SpectralShipGenStudio.exe
    LICENSE
    THIRD_PARTY_NOTICES.md
    README.md
    USER_GUIDE.md
```

`README.md` is this portable-distribution note; `USER_GUIDE.md` is the offline first-time-user guide.

## Running the portable package

1. Extract the entire ZIP to a normal writable folder.
2. Double-click `SpectralShipGenStudio.exe`.
3. Keep the extracted folder as Studio's working folder if you want its local Favorites, presets, and preferences to stay with that portable setup.

The Release application and package were validated outside the source/build tree on Windows x64.

## Display and window behavior

Studio uses a fixed 1640×1000 logical interface presented through a normal resizable/maximizable desktop window.

- **1920×1080 or higher is recommended** for the most comfortable and fully readable experience.
- Studio remains **functionally usable down to 1280×720**, although some of the smallest helper text becomes difficult to read at that scale.
- The interface preserves its aspect ratio. Depending on the physical window shape, unused letterbox/pillarbox regions may appear rather than stretching the UI.
- If a normal 1640×1000 client area does not fit the Windows desktop work area, Studio starts with a smaller aspect-preserving physical window so the complete application remains reachable.

## Runtime linkage

The supported portable build uses `BUILD_SHARED_LIBS=OFF`, and the validated Windows Release executable links SFML statically. The package therefore does **not** require `sfml-*.dll` files beside `SpectralShipGenStudio.exe`.

The executable is Windows x64 (`8664` machine type) and uses the normal dynamically linked Microsoft C++ runtime. The validated executable depends on runtime components including:

```text
MSVCP140.dll
VCRUNTIME140.dll
VCRUNTIME140_1.dll
```

Therefore **Microsoft Visual C++ 2015–2022 Redistributable (x64) may be required** on a system where it is not already installed.

If Windows reports that one of these runtime DLLs is missing, install the Microsoft Visual C++ 2015–2022 Redistributable (x64). Do not download individual runtime DLL files from third-party sites.

## Runtime files and working directory

Studio has no external font/image/audio asset directory; the UI pixel-text implementation is source-owned. The executable does not require separate SFML DLLs or a bundled application asset folder.

Automatic local state is written relative to Studio's working directory:

- `spectral_ship_gen_preview_user_presets.json`;
- `spectral_ship_gen_preview_favorites.json`;
- `spectral_ship_gen_preview_preferences.json`.

For a self-contained portable workflow, keep these generated local files with the extracted application folder if you want to move the setup while preserving the same Favorites/presets/preferences.

Those user-created files are intentionally **not** shipped in a clean release ZIP.

Normal exports such as PNG images, spritesheets, Recipes, and Profile/Full Configuration exports are also written to the current working directory unless the workflow explicitly reads an import path from the console.

## Licensing

The portable ZIP includes Studio's MPL-2.0 `LICENSE` and `THIRD_PARTY_NOTICES.md`. SpectralShipGen is separately zlib-licensed and SFML is zlib/png-licensed as recorded in the notices.
