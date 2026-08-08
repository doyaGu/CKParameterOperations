# Parameter Operations

Parameter Operations is the Virtools manager module that registers the parameter-operation types and functions required by Ballanced compositions.

## Features

- More than 180 operation types and 422 operation functions
- Mathematical, logical, conversion, and object-query operations
- Float, integer, boolean, vector, string, matrix, color, quaternion, object, sound, mesh, animation, camera, and curve support
- Entity, mesh, material, scene, and collision queries

## Support scope

The instructions in this document describe Parameter Operations' `sdl` branch. That branch is built and staged by [Ballanced](https://github.com/doyaGu/Ballanced) on Windows, Linux, and macOS. It can be built as a shared manager module or linked statically into the Ballanced Player.

Output names are platform-native rather than always being a Windows DLL.

## Building

### Recommended: Ballanced superproject

```bash
git clone --recurse-submodules https://github.com/doyaGu/Ballanced.git
cd Ballanced
cmake --preset linux-x64-runtime # choose the preset for your host
cmake --build --preset linux-x64-runtime-stage-release
```

The shared module is staged under `build/<preset>/stage/Managers/`.

### Standalone

Requirements:

- CMake 3.16+
- A desktop C++ toolchain
- CK2 and VxMath, automatically detected from a Ballanced sibling checkout
- A Virtools SDK supplied through `VIRTOOLS_SDK_PATH`, or fetched with `VIRTOOLS_SDK_FETCH_FROM_GIT=ON`, when sibling projects are unavailable

```bash
cmake -S . -B build -DVIRTOOLS_SDK_FETCH_FROM_GIT=ON
cmake --build build --config Release
```

CMake options:

- `CKPARAMOP_BUILD_SHARED`: build a shared manager module; default `ON`
- `CKPARAMOP_BUILD_STATIC`: build a static library; default `OFF`
- `CKPARAMOP_INSTALL`: generate install rules

## Project structure

- `ParameterOperations.cpp`: module registration
- `ParameterOperationFunctions.cpp`: operation implementations
- `ParameterOperationTypes.h`: operation type GUIDs
- `ParameterTypes.h`: parameter type GUIDs
- `docs/FunctionCategories.md`: generated function catalog
- `scripts/`: export and generation utilities

## Plugin information

- Name: Parameter Operations
- Type: manager module
- GUID: `0x4c8f620e, 0x64521f0a`

## Documentation

See [docs/FunctionCategories.md](docs/FunctionCategories.md) for the complete operation catalog.

## Versioning

Parameter Operations is versioned independently. Ballanced releases pin an exact manager commit.

## License

See [LICENSE](LICENSE).
