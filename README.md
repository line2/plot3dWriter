# Plot3DWriter

A lightweight C++/C library for writing structured Plot3D files (`.xyz` and `.q`). It supports multi-block grids, single/double precision, and optional VTK integration for converting standard mesh formats.

## Project Structure

The project is organized into three layers:

- **Core Layer (`src/core`)**: Fundamental writer in C++. Handles binary writing, Fortran record formatting, and byte swapping.
- **C API Layer (`src/c_api`)**: C-compatible wrapper around the Core Layer.
- **VTK Layer (`src/vtk`)**: High-level wrapper converting `vtkStructuredGrid` to Plot3D. Requires VTK.

## Requirements

- **CMake**: version 3.21 or higher.
- **Compiler**: C++17 compatible (e.g., MSVC 2022, Clang 15+, GCC 9+).
- **vcpkg**: Used for dependency management (specifically for VTK).
- **VTK** (Optional): Required for the VTK integration layer and conversion examples.

## Quick Start (Windows)

This repository uses [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) for easy configuration.

1.  **Set up vcpkg**: Ensure `VCPKG_ROOT` environment variable is set to your vcpkg installation path.
2.  **Configure Presets**: The project provides `Debug` and `Release` presets in `CMakeUserPresets.json`. 
    *Note: These presets currently use `clang-cl` and `Ninja`. You may need to adjust them to match your local toolchain.*
3.  **Build**:
    ```powershell
    # Configure and build using the Release preset
    cmake --preset Release
    cmake --build --preset Release
    ```

## Examples

The build process produces several example executables in the output directory:

- `multi_block_example`: Demonstrates using the C++ Core API to write a multi-block grid.
  - Output: `multi_block_test.xyz`
- `file_reader_c_api_example`: A utility to convert various mesh formats to Plot3D using the C API and VTK.
  - Usage: `file_reader_c_api_example <input_mesh_file>`
  - Supported formats: `.vts`, `.vtk`, `.vtu`, `.cgns`.

## Configuration Options

Pass these to CMake during configuration (e.g., `-D<OPTION>=<VALUE>`):

| Option | Description | Default |
| :--- | :--- | :--- |
| `ENABLE_VTK` | Enable VTK integration and conversion examples. | `ON` |
| `PLOT3D_USE_STATIC_MSVC_RUNTIME` | (MSVC Only) Link against the static runtime (`/MT`). | `ON` |

## Consumption

### Using `find_package`

If you install the library, you can consume it in your own CMake project:

```cmake
find_package(Plot3DWriter CONFIG REQUIRED)

# Link against the Core/C API DLL
target_link_libraries(my_app PRIVATE Plot3DWriter::plot3d_writer_dll)

# Link against the VTK Layer (if ENABLE_VTK was ON)
target_link_libraries(my_app PRIVATE Plot3DWriter::plot3d_writer)
```

## Packaging

The project is configured with CPack to generate a redistributable ZIP package.

```powershell
# From the build directory (e.g., out/build/Release)
cpack
```
The resulting package will be located in `out/pack/Release/`.
