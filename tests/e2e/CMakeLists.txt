cmake_minimum_required(VERSION 3.20)
project(eco_e2e_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(ECO_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(ECO_GAME_BUILD_DIR "${ECO_PROJECT_ROOT}/build" CACHE PATH "Path to main project build directory")
option(E2E_CONTRACT_CHECK_ONLY "Build standalone contract-check runner without libgame_core.a" OFF)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/e2e_paths.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/e2e_paths.hpp"
    @ONLY
)

file(GLOB ECO_E2E_SUITE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/suites/*.cpp"
)

if(E2E_CONTRACT_CHECK_ONLY)
    add_executable(eco_e2e_contract_check
        "${CMAKE_CURRENT_SOURCE_DIR}/e2e_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/e2e_contract_check.cpp"
        ${ECO_E2E_SUITE_SOURCES}
    )
    target_include_directories(eco_e2e_contract_check PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    )
    target_compile_definitions(eco_e2e_contract_check PRIVATE E2E_CONTRACT_MODE=1)
    target_compile_options(eco_e2e_contract_check PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Werror -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    )
else()
    find_library(ECO_GAME_CORE_LIB
        NAMES game_core libgame_core
        PATHS "${ECO_GAME_BUILD_DIR}" "${ECO_GAME_BUILD_DIR}/src/core"
        NO_DEFAULT_PATH
    )
    if(NOT ECO_GAME_CORE_LIB)
        message(FATAL_ERROR
            "libgame_core.a not found in '${ECO_GAME_BUILD_DIR}' or '${ECO_GAME_BUILD_DIR}/src/core'. "
            "Build the main project first or configure with -DE2E_CONTRACT_CHECK_ONLY=ON.")
    endif()

    find_package(Threads REQUIRED)

    set(ECO_E2E_EXTRA_CORE_SOURCES "")
    if(EXISTS "${ECO_PROJECT_ROOT}/src/app/game_state_manager.cpp")
        list(APPEND ECO_E2E_EXTRA_CORE_SOURCES
            "${ECO_PROJECT_ROOT}/src/app/game_state_manager.cpp"
        )
    endif()

    add_executable(eco_e2e_runner
        "${CMAKE_CURRENT_SOURCE_DIR}/e2e_main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/e2e_live_adapter.cpp"
        ${ECO_E2E_EXTRA_CORE_SOURCES}
        ${ECO_E2E_SUITE_SOURCES}
    )
    target_include_directories(eco_e2e_runner PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
        "${ECO_PROJECT_ROOT}/src"
        "${ECO_PROJECT_ROOT}/src/core"
    )
    target_link_libraries(eco_e2e_runner PRIVATE
        "${ECO_GAME_CORE_LIB}"
        Threads::Threads
        $<$<PLATFORM_ID:Linux>:m>
    )
    target_compile_options(eco_e2e_runner PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Werror -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    )
endif()
