# This script is intended to be used via CMAKE_PROJECT_TOP_LEVEL_INCLUDES
# to automatically detect the vtk-compile-tools directory when using vcpkg
# with a static triplet on Windows.

if(NOT ENABLE_VTK)
    message(STATUS "[Plot3D] VTK integration disabled, skipping host-triplet detection.")
    return()
endif()

message(STATUS "[Plot3D] Detecting vtk-compile-tools for host triplet...")
if(DEFINED VCPKG_INSTALLED_DIR)
    set(_plot3d_vtk_compiletools_dir "")
    set(_plot3d_vcpkg_host_prefix "")
    set(_plot3d_host_triplet_candidates "")

    if(DEFINED VCPKG_HOST_TRIPLET AND NOT VCPKG_HOST_TRIPLET STREQUAL "")
        list(APPEND _plot3d_host_triplet_candidates "${VCPKG_HOST_TRIPLET}")
    endif()
    if(DEFINED VCPKG_TARGET_TRIPLET AND NOT VCPKG_TARGET_TRIPLET STREQUAL "")
        string(REGEX REPLACE "-static$" "" _plot3d_derived_host_triplet "${VCPKG_TARGET_TRIPLET}")
        if(NOT _plot3d_derived_host_triplet STREQUAL "${VCPKG_TARGET_TRIPLET}")
            list(APPEND _plot3d_host_triplet_candidates "${_plot3d_derived_host_triplet}")
        endif()
    endif()
    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            list(APPEND _plot3d_host_triplet_candidates "x64-windows")
        else()
            list(APPEND _plot3d_host_triplet_candidates "x86-windows")
        endif()
    endif()
    list(REMOVE_DUPLICATES _plot3d_host_triplet_candidates)

    foreach(_t IN LISTS _plot3d_host_triplet_candidates)
        set(_cand "${VCPKG_INSTALLED_DIR}/${_t}/share/vtk-compile-tools")
        if(EXISTS "${_cand}/vtkcompiletools-config.cmake")
            set(_plot3d_vtk_compiletools_dir "${_cand}")
            set(_plot3d_vcpkg_host_prefix "${VCPKG_INSTALLED_DIR}/${_t}")
            break()
        endif()
    endforeach()

    if(NOT _plot3d_vtk_compiletools_dir STREQUAL "")
        # Seed both a cache and non-cache hint to avoid stale paths in VTK's config.
        if(NOT DEFINED VTKCompileTools_DIR OR VTKCompileTools_DIR STREQUAL "VTKCompileTools_DIR-NOTFOUND"
            OR NOT EXISTS "${VTKCompileTools_DIR}/vtkcompiletools-config.cmake")
            set(VTKCompileTools_DIR "${_plot3d_vtk_compiletools_dir}" CACHE PATH "VTK compile tools (vcpkg host triplet)" FORCE)
        endif()
        set(VTKCompileTools_DIR "${_plot3d_vtk_compiletools_dir}")
        message(STATUS "[Plot3D] Found vtk-compile-tools: ${VTKCompileTools_DIR}")

        # Make host-only packages discoverable via prefix search.
        list(PREPEND CMAKE_PREFIX_PATH "${_plot3d_vcpkg_host_prefix}")
        message(STATUS "[Plot3D] Prepended to CMAKE_PREFIX_PATH: ${_plot3d_vcpkg_host_prefix}")
    endif()

    unset(_plot3d_vtk_compiletools_dir)
    unset(_plot3d_vcpkg_host_prefix)
    unset(_plot3d_host_triplet_candidates)
    unset(_plot3d_derived_host_triplet)
    unset(_t)
    unset(_cand)
endif()
