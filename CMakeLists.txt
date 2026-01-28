cmake_minimum_required(VERSION 3.21)
project(Plot3DWriter VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find Dependencies ---
option(ENABLE_VTK "Enable VTK integration layer" ON)
if(ENABLE_VTK)
    find_package(VTK CONFIG REQUIRED)
endif()

# --- Core DLL (Includes C++ and C API) ---
add_library(plot3d_writer_dll SHARED
    src/core/Plot3dWriterCore.cpp
    src/core/Plot3dWriterCore.h
    src/c_api/Plot3dWriterC.cpp
    src/c_api/Plot3dWriterC.h
    src/c_api/Plot3dWriterExport.h
)
target_include_directories(plot3d_writer_dll
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/core>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/c_api>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/plot3dwriter>
)
target_compile_definitions(plot3d_writer_dll PRIVATE PLOT3D_WRITER_DLL_EXPORTS)

# --- VTK Layer ---
if(ENABLE_VTK)
    add_library(plot3d_writer STATIC
        src/vtk/Plot3dWriter.cpp
        src/vtk/Plot3dWriter.h
    )
    target_link_libraries(plot3d_writer PUBLIC plot3d_writer_dll ${VTK_LIBRARIES})
    target_include_directories(plot3d_writer
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/vtk>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/plot3dwriter>
    )
endif()

# --- Examples ---
add_executable(core_example example/core_example.cpp)
target_link_libraries(core_example PRIVATE plot3d_writer_dll)

add_executable(c_api_example example/c_api_example.cpp)
target_link_libraries(c_api_example PRIVATE plot3d_writer_dll)


# On Windows, copy the DLL to the same directory as the examples for easy execution
if(WIN32)
    add_custom_command(TARGET core_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:plot3d_writer_dll>
        $<TARGET_FILE_DIR:core_example>
    )
    add_custom_command(TARGET c_api_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:plot3d_writer_dll>
        $<TARGET_FILE_DIR:c_api_example>
    )
endif()

# --- Installation ---
set(INSTALL_TARGETS plot3d_writer_dll)
if(ENABLE_VTK)
    list(APPEND INSTALL_TARGETS plot3d_writer)
endif()

install(TARGETS ${INSTALL_TARGETS}
    EXPORT Plot3DWriterTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

set(INSTALL_HEADERS
    src/core/Plot3dWriterCore.h 
    src/c_api/Plot3dWriterC.h 
    src/c_api/Plot3dWriterExport.h
)
if(ENABLE_VTK)
    list(APPEND INSTALL_HEADERS src/vtk/Plot3dWriter.h)
endif()

install(FILES ${INSTALL_HEADERS}
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

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    # 定义一个来源路径（当前构建目录）和目标路径（项目根目录）
    set(SRC_JSON "${CMAKE_BINARY_DIR}/compile_commands.json")
    set(DST_JSON "${CMAKE_SOURCE_DIR}/compile_commands.json")

    # 方法 A：在构建任何目标时，自动检查并更新根目录的 json (推荐)
    # 缺点：必须执行一次 build (哪怕是空 build) 才会同步
    add_custom_target(update_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC_JSON} ${DST_JSON}
        COMMENT "Updating compile_commands.json to root folder"
        VERBATIM
    )
endif()
