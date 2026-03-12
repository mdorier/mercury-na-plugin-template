cmake_minimum_required(VERSION 3.15)
project(na_plugin_abc C)

# Find Mercury's NA library (provides na_types.h, etc.)
find_package(mercury REQUIRED)

# na_plugin.h and na_error.h are not installed by Mercury but are required to
# build an NA plugin.  Download them from the matching release tag.
set(_na_hdr_dir "${CMAKE_CURRENT_BINARY_DIR}/na_headers")
set(_na_hdr_base_url
    "https://raw.githubusercontent.com/mercury-hpc/mercury/v${MERCURY_VERSION_FULL}/src/na")
foreach(_hdr na_plugin.h na_error.h)
    if(NOT EXISTS "${_na_hdr_dir}/${_hdr}")
        message(STATUS "Downloading ${_hdr} for Mercury ${MERCURY_VERSION_FULL}")
        file(DOWNLOAD
            "${_na_hdr_base_url}/${_hdr}"
            "${_na_hdr_dir}/${_hdr}"
            STATUS _dl_status
        )
        list(GET _dl_status 0 _dl_code)
        if(NOT _dl_code EQUAL 0)
            list(GET _dl_status 1 _dl_msg)
            message(FATAL_ERROR "Failed to download ${_hdr}: ${_dl_msg}")
        endif()
    endif()
endforeach()

# Build the plugin as a shared MODULE library.
# MODULE produces a .so that is dlopen'd at runtime rather than linked directly.
add_library(na_plugin_abc MODULE na_abc.c)

# Link against Mercury's "na" target.  Because na publicly links mercury_util,
# this transitively provides the include paths for na_plugin.h, na_error.h,
# mercury_log.h, mercury_queue.h, etc.
target_link_libraries(na_plugin_abc PRIVATE mercury)
target_include_directories(na_plugin_abc PRIVATE "${_na_hdr_dir}")

# Strip the default "lib" prefix so the output is exactly "libna_plugin_abc.so"
# (CMake MODULE libraries already get the "lib" prefix on Linux, so this is a
#  no-op there, but explicit for clarity).
set_target_properties(na_plugin_abc PROPERTIES PREFIX "lib")

# Installation -----------------------------------------------------------------
# By default, install into the same directory as libna.so so that the plugin is
# found automatically.  Override NA_PLUGIN_INSTALL_DIR at configure time if you
# prefer a different location (the user would then set NA_PLUGIN_PATH at runtime).
if(NOT NA_PLUGIN_INSTALL_DIR)
    # GNUInstallDirs gives us CMAKE_INSTALL_LIBDIR (usually "lib" or "lib64").
    include(GNUInstallDirs)
    set(NA_PLUGIN_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
endif()

install(TARGETS na_plugin_abc
    LIBRARY DESTINATION ${NA_PLUGIN_INSTALL_DIR}
)

# Tests
enable_testing()
add_subdirectory(test)
