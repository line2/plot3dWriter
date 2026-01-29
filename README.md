# Plot3DWriter

A lightweight C++/C library for writing structured Plot3D files (.xyz and .q).

## Project Structure

The project is organized into three main layers:

- **Core Layer (`src/core`)**: The fundamental writer implementation in C++. It handles binary file writing, Fortran record formatting, and byte swapping.
- **C API Layer (`src/c_api`)**: A C-compatible wrapper around the Core Layer, allowing integration with C projects.
- **VTK Layer (`src/vtk`)**: A high-level wrapper that converts VTK objects (`vtkStructuredGrid`) into Plot3D files. Requires VTK.

## Directory Layout

- `src/`: Library source code.
  - `core/`: Core C++ implementation.
  - `c_api/`: C API wrapper and export headers.
  - `vtk/`: VTK integration layer.
- `example/`: Usage examples for different APIs.
- `cmake/`: CMake configuration and helper files.

## Getting Started

### Choosing an API

1. **Modern C++**: Use `src/core/Plot3dWriterCore.h` for direct file writing.
2. **C Projects**: Use `src/c_api/Plot3dWriterC.h` (C API functions).
3. **VTK Projects**: Use `src/vtk/Plot3dWriter.h` to write VTK data objects directly.

### Building

This project uses CMake and vcpkg for dependency management.

```bash
cmake --preset=ninja-msvc-release
cmake --build --preset=ninja-msvc-release
```

## Examples

- `example/core_example.cpp`: Demonstrates using the C++ Core API.
- `example/c_api_example.cpp`: Demonstrates using the C API.
- `example/file_reader_c_api_example.cpp`: Demonstrates reading external mesh files (.vts, .vtk, .vtu, .cgns) using VTK and converting them to Plot3D using the C API.
