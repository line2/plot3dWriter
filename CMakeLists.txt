cmake_minimum_required(VERSION 3.21)
project(Plot3DWriter VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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
target_include_directories(plot3d_writer_dll
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/plot3dwriter>
)
target_compile_definitions(plot3d_writer_dll PRIVATE PLOT3D_WRITER_DLL_EXPORTS)

# --- VTK Layer ---
add_library(plot3d_writer STATIC
    src/Plot3dWriter.cpp
    src/Plot3dWriter.h
)
target_link_libraries(plot3d_writer PUBLIC plot3d_writer_dll ${VTK_LIBRARIES})
target_include_directories(plot3d_writer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/plot3dwriter>
)

# --- Example ---
add_executable(core_example example/core_example.cpp)
target_link_libraries(core_example PRIVATE plot3d_writer)

# --- Installation ---
install(TARGETS plot3d_writer plot3d_writer_dll
    EXPORT Plot3DWriterTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES 
    src/Plot3dWriterCore.h 
    src/Plot3dWriterExport.h 
    src/Plot3dWriter.h 
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/plot3dwriter
)

# --- CMake Package Export ---
set(PROJECT_CONFIG_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Plot3DWriter)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Plot3DWriterConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Plot3DWriterConfig.cmake
    INSTALL_DESTINATION ${PROJECT_CONFIG_DESTINATION}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/Plot3DWriterConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/Plot3DWriterConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/Plot3DWriterConfigVersion.cmake
    DESTINATION ${PROJECT_CONFIG_DESTINATION}
)

install(EXPORT Plot3DWriterTargets
    NAMESPACE Plot3DWriter::
    DESTINATION ${PROJECT_CONFIG_DESTINATION}
)

# --- CPack ---
set(CPACK_PACKAGE_NAME "Plot3DWriter")
set(CPACK_PACKAGE_VENDOR "Plot3DWriter Contributors")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_GENERATOR "ZIP")
include(CPack)
