# Third-Party Notices

SpectralShipGen Studio depends on third-party/open-source software but does not vendor those dependency source trees in this repository.

## SpectralShipGen Library

- Project: SpectralShipGen
- Purpose: ship generation, recipes, animation, validation, and reusable diagnostics
- Source: https://github.com/SpectralHorse/SpectralShipGen
- License: zlib License
- Bundling status in this repository: external dependency; Library source is not copied into Studio

## SFML

- Project: Simple and Fast Multimedia Library (SFML)
- Purpose: Studio windowing/graphics integration
- Source: https://github.com/SFML/SFML
- Version policy: 2.6.x branch as configured by Studio CMake
- License: zlib/png License
- Bundling status in this repository: external dependency obtained by CMake FetchContent when enabled, or provided by the build environment
- Upstream license information: https://www.sfml-dev.org/license/

Build tools, compilers, operating-system libraries, and CI actions are not redistributed as source components of this repository. Final binary/release packaging should preserve the applicable upstream notices for any third-party files that are actually redistributed with that package.
