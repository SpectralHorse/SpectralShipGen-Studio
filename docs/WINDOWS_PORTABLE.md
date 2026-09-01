# Windows x64 Portable Distribution

The 1.0 release-candidate binary target is a portable Windows x64 ZIP named:

```text
SpectralShipGen-Studio-1.0.0-rc.1-Windows-x64.zip
```

The package contains only:

```text
SpectralShipGen-Studio-1.0.0-rc.1-Windows-x64/
    SpectralShipGenStudio.exe
    LICENSE
    THIRD_PARTY_NOTICES.md
    README.md
    USER_GUIDE.md
```

`README.md` is this portable-distribution note; `USER_GUIDE.md` provides the offline first-time-user guide.

## Runtime linkage

Studio sets `BUILD_SHARED_LIBS=OFF`, so the supported FetchContent SFML build is static and the portable package does not expect SFML DLLs beside the executable.

The project does **not** override CMake/MSVC's runtime-library selection. Normal Release builds therefore use the toolchain's default MSVC runtime linkage. A machine may require the matching Microsoft Visual C++ Redistributable. The release build intentionally keeps the toolchain's normal runtime selection rather than forcing `/MT` across Studio and its dependency graph.

## Runtime files and working directory

Studio has no external font/image/audio resource files in the repository; the UI pixel text implementation is source-owned. The executable does not require a bundled asset directory.

Automatic local state is currently written relative to the application's working directory. For a self-contained portable workflow, launch Studio with the extracted application folder as its working directory and keep these generated local files with that folder if you want to move the setup:

- `spectral_ship_gen_preview_user_presets.json`;
- `spectral_ship_gen_preview_favorites.json`;
- `spectral_ship_gen_preview_preferences.json`.

Those user-created files are intentionally **not** shipped in a clean release ZIP.

## Licensing

The portable ZIP includes Studio's MPL-2.0 `LICENSE` and `THIRD_PARTY_NOTICES.md`. SpectralShipGen is separately zlib-licensed and SFML is zlib/png-licensed as recorded in the notices.
