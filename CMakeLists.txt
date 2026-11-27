cmake_minimum_required(VERSION 3.22...4.3)

project(EcoMathProto VERSION 0.1.0 LANGUAGES C CXX)

# Enforce ISO C++20 portably across GCC, Clang, and MSVC
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Skip unnecessary C++20 module dependency scanning (faster builds on CMake 3.28+ / Ninja / VS)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Default single-config generators (Ninja, Unix/MinGW Makefiles) to Release if unspecified
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

option(ECOMATH_ENABLE_IPO "Enable Link-Time Optimization (IPO/LTO) in Release builds when supported" ON)
option(ECOMATH_MINGW_STATIC_RUNTIME "Statically link libgcc/libstdc++ on Windows MinGW builds" ON)

set(_ecomath_ipo_supported FALSE)
if(ECOMATH_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _ecomath_ipo_supported OUTPUT _ecomath_ipo_error)
    if(NOT _ecomath_ipo_supported)
        message(STATUS "IPO/LTO not supported by current toolchain: ${_ecomath_ipo_error}")
    endif()
endif()

enable_testing()

# ------------------------------------------------------------------------------
# Raylib 5.5 Acquisition (find_package first, FetchContent fallback)
# ------------------------------------------------------------------------------
find_package(raylib 5.5 QUIET)

if(NOT raylib_FOUND)
    message(STATUS "raylib 5.5 not found locally; configuring FetchContent fallback...")

    # Prevent CMake 4.x errors if any subproject CMakeLists sets policy < 3.5
    if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
        set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    endif()

    # Keep subsequent offline reconfigures from hitting github.com after initial clone
    set(FETCHCONTENT_UPDATES_DISCONNECTED_RAYLIB ON CACHE BOOL
        "Skip remote git updates for pinned raylib tag after initial fetch" FORCE)

    # Raylib 5.5 build options (static Desktop build, no examples/games)
    set(PLATFORM "Desktop" CACHE STRING "Target platform for raylib" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "Build raylib games" FORCE)
    if(NOT DEFINED BUILD_SHARED_LIBS)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
    endif()

    # Linux Wayland/X11 header-safe detection (ONLY on Linux/UNIX, never on Windows/MSYS/Cygwin/Apple)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT WIN32 AND NOT MSYS AND NOT CYGWIN)
        find_path(ECOMATH_XINERAMA_INCLUDE_DIR NAMES X11/extensions/Xinerama.h)
        find_path(ECOMATH_XCURSOR_INCLUDE_DIR  NAMES X11/Xcursor/Xcursor.h)
        find_path(ECOMATH_XINPUT2_INCLUDE_DIR  NAMES X11/extensions/XInput2.h)

        find_path(ECOMATH_WAYLAND_INCLUDE_DIR   NAMES wayland-client.h)
        find_path(ECOMATH_XKBCOMMON_INCLUDE_DIR NAMES xkbcommon/xkbcommon.h)
        find_program(ECOMATH_WAYLAND_SCANNER    NAMES wayland-scanner)

        if(ECOMATH_XINERAMA_INCLUDE_DIR AND ECOMATH_XCURSOR_INCLUDE_DIR AND ECOMATH_XINPUT2_INCLUDE_DIR)
            set(_ecomath_has_x11_headers TRUE)
        else()
            set(_ecomath_has_x11_headers FALSE)
        endif()

        if(ECOMATH_WAYLAND_INCLUDE_DIR AND ECOMATH_XKBCOMMON_INCLUDE_DIR AND ECOMATH_WAYLAND_SCANNER)
            set(_ecomath_has_wayland_deps TRUE)
        else()
            set(_ecomath_has_wayland_deps FALSE)
        endif()

        if(NOT _ecomath_has_x11_headers AND _ecomath_has_wayland_deps)
            message(STATUS "Missing X11 extension headers (Xinerama/Xcursor/XInput2); enabling GLFW Wayland-only backend.")
            if(NOT DEFINED GLFW_BUILD_X11)
                set(GLFW_BUILD_X11 OFF CACHE BOOL "Build bundled GLFW with X11 support" FORCE)
            endif()
            if(NOT DEFINED GLFW_BUILD_WAYLAND)
                set(GLFW_BUILD_WAYLAND ON CACHE BOOL "Build bundled GLFW with Wayland support" FORCE)
            endif()
        elseif(NOT _ecomath_has_x11_headers AND NOT _ecomath_has_wayland_deps)
            message(FATAL_ERROR
                "Neither full X11 dev headers (X11/extensions/Xinerama.h, X11/Xcursor/Xcursor.h, X11/extensions/XInput2.h) "
                "nor Wayland dev tools (wayland-client.h, xkbcommon/xkbcommon.h, wayland-scanner) were found.")
        endif()
    endif()

    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

# ------------------------------------------------------------------------------
# Project Warning Helper (applied ONLY to project targets, never to Raylib)
# ------------------------------------------------------------------------------
function(ecomath_configure_target target_name)
    target_compile_features(${target_name} PUBLIC cxx_std_20)
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        CXX_SCAN_FOR_MODULES OFF
    )
    if(_ecomath_ipo_supported)
        set_target_properties(${target_name} PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
            INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON
        )
    endif()
    target_compile_definitions(${target_name} PRIVATE
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS>
    )
    target_compile_options(${target_name} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- /utf-8 /MP>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall -Wextra -Wpedantic>
    )
endfunction()

# ------------------------------------------------------------------------------
# Target 1: game_core (Pure C++20 simulation/math — ZERO Raylib dependencies)
# ------------------------------------------------------------------------------
add_library(game_core STATIC
    src/core/player.cpp
    src/core/ecosystem.cpp
    src/core/tilemap.cpp
    src/core/structure_puzzles.cpp
    src/core/physics2d.cpp
    src/core/machine.cpp
    src/core/npc.cpp
    src/core/collectible.cpp
    src/core/collectible_content.cpp
    src/core/simulation.cpp
    src/core/story.cpp
    src/core/story_content.cpp
    src/core/save_data.cpp
)
target_include_directories(game_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
ecomath_configure_target(game_core)

# ------------------------------------------------------------------------------
# Target 2: eco_proto (Raylib presentation + fixed-timestep game loop)
# ------------------------------------------------------------------------------
# Do NOT pass WIN32 so int main(argc, argv) and --headless-e2e/--smoke-frames CLI flags work on Windows.
add_executable(eco_proto
    src/app/main.cpp
    src/app/game_state_manager.cpp
    src/app/raylib_input.cpp
    src/app/calibration_ui.cpp
    src/app/overworld_state.cpp
    src/app/structure_state.cpp
    src/app/title_state.cpp
    src/app/intro_state.cpp
    src/app/story_ui.cpp
    src/app/pause_settings_menu.cpp
    src/app/render/ui_style.cpp
    src/app/render/procedural_art.cpp
    src/app/render/particles.cpp
    src/app/render/world_entities.cpp
    src/app/audio/audio_engine.cpp
    src/app/gpu_selector.cpp
)
target_link_libraries(eco_proto
    PRIVATE
        game_core
        raylib
        $<$<PLATFORM_ID:Linux>:${CMAKE_DL_LIBS}>
)
ecomath_configure_target(eco_proto)

if(WIN32 AND MINGW AND ECOMATH_MINGW_STATIC_RUNTIME)
    target_link_options(eco_proto PRIVATE -static-libgcc -static-libstdc++)
endif()

# Stage UI font next to executable so both Linux and Windows (including MSVC Release/Debug) find it reliably
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/ui_font.ttf")
    add_custom_command(TARGET eco_proto POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:eco_proto>/assets/fonts"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/ui_font.ttf"
            "$<TARGET_FILE_DIR:eco_proto>/assets/fonts/ui_font.ttf"
        COMMENT "Staging UI font asset next to eco_proto"
    )
endif()

# Windows executable icon (.ico embedded via .rc resource)
if(WIN32)
    target_sources(eco_proto PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/eco_proto.rc"
    )
endif()

# Linux desktop integration (executable + font asset + .desktop + hicolor icon)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT WIN32)
    include(GNUInstallDirs)
    install(TARGETS eco_proto RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/ui_font.ttf")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/ui_font.ttf"
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/eco_proto/assets/fonts")
    endif()
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/eco_proto.desktop"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/eco_proto.png"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps")
endif()

# ------------------------------------------------------------------------------
# Target 3: eco_tests (Zero-dependency CTest suite — links game_core ONLY, NO Raylib)
# ------------------------------------------------------------------------------
add_executable(eco_tests
    tests/test_main.cpp
    src/app/game_state_manager.cpp
)
target_link_libraries(eco_tests
    PRIVATE
        game_core
)
ecomath_configure_target(eco_tests)

add_test(NAME eco_tests COMMAND eco_tests)
add_test(NAME headless_e2e COMMAND eco_proto --headless-e2e)
