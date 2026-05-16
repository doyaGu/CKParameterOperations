# Parameter Operations

A Virtools plugin manager providing comprehensive parameter operation functions for the Virtools.

## Overview

Extends the Virtools engine with 180+ operation types and 422+ operation functions for data manipulation, mathematical operations, type conversions, and object queries.

## Features

- **180+ Operation Types**: Mathematical, logical, and object manipulation operations
- **422+ Operation Functions**: Categorized by data type (Float, Int, Bool, Vector, String, Matrix, Color, Quaternion, Object, Sound, Mesh, Animation, Camera, Curve)
- **Type Conversions**: Seamless conversion between Float, Int, Bool, Vector, String, Matrix, Color, and more
- **Object Queries**: Access to entity properties, mesh data, materials, animations, sounds, and scene objects
- **Collision Detection**: Built-in collision testing for boxes, entities, and vectors

## Building

### Prerequisites

- Desktop C++ toolchain (MSVC, GCC, or Clang)
- Virtools SDK 2.1
- CMake 3.16+

### Build Steps

```bash
# Set Virtools SDK path
cmake -B build -DVIRTOOLS_SDK_PATH="path/to/virtools/sdk"

# Or fetch SDK automatically
cmake -B build -DVIRTOOLS_SDK_FETCH_FROM_GIT=ON

# Build
cmake --build build --config Release
```

In the SDL3-only Ballanced superproject this manager is built and staged on
Windows, Linux, and macOS as a required runtime manager.

## Project Structure

```
ParameterOperations/
├── ParameterOperations.cpp          # Plugin initialization
├── ParameterOperationFunctions.cpp  # Operation implementations
├── ParameterOperationTypes.h        # Operation type GUIDs
├── ParameterTypes.h                 # Parameter type GUIDs
├── CMakeLists.txt                   # Build configuration
├── docs/FunctionCategories.md       # Detailed function documentation
└── scripts/                         # Export and generation utilities
```

## Plugin Information

- **Name**: Parameter Operations
- **Type**: Manager DLL
- **GUID**: `0x4c8f620e, 0x64521f0a`

## Documentation

See [`docs/FunctionCategories.md`](docs/FunctionCategories.md) for a complete list of all operation functions.

## License

See [`LICENSE`](LICENSE) file for details.
