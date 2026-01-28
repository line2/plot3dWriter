cmake_minimum_required(VERSION 3.21)
project(Plot3DWriter LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find Dependencies ---
find_package(VTK CONFIG REQUIRED)

# --- Core DLL ---
add_library(plot3d_writer_dll SHARED
    src/Plot3dWriterCore.cpp
    src/Plot3dWriterCore.h
    src/Plot3dWriterExport.h
)
target_include_directories(plot3d_writer_dll PUBLIC src)
target_compile_definitions(plot3d_writer_dll PRIVATE PLOT3D_WRITER_DLL_EXPORTS)

# --- VTK Layer ---
add_library(plot3d_writer STATIC
    src/Plot3dWriter.cpp
    src/Plot3dWriter.h
)
target_link_libraries(plot3d_writer PUBLIC plot3d_writer_dll ${VTK_LIBRARIES})
target_include_directories(plot3d_writer PUBLIC src)

# --- Example ---
add_executable(core_example example/core_example.cpp)
target_link_libraries(core_example PRIVATE plot3d_writer)

# --- Installation ---
# (Optional, but good practice)
install(TARGETS plot3d_writer_dll plot3d_writer core_example
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES 
    src/Plot3dWriterCore.h 
    src/Plot3dWriterExport.h 
    src/Plot3dWriter.h 
    DESTINATION include
)
