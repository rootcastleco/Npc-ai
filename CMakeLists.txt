cmake_minimum_required(VERSION 3.20)
project(rootcastle_npc_ai VERSION 0.1.0 LANGUAGES C CXX)

option(RC_NPC_BUILD_TESTS "Build deterministic regression tests" ON)
option(RC_NPC_BUILD_EXAMPLES "Build console and engine-adapter examples" ON)
option(RC_NPC_BUILD_BENCH "Build a local load measurement executable" ON)
option(BUILD_SHARED_LIBS "Build a shared C ABI library (for Unity/FFI)" OFF)

add_library(rootcastle_npc
    src/npc.cpp
    src/npc_c.cpp
    src/task_hooks.cpp)
add_library(rootcastle::npc ALIAS rootcastle_npc)
target_compile_features(rootcastle_npc PUBLIC cxx_std_20)
target_include_directories(rootcastle_npc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
set_target_properties(rootcastle_npc PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1)
# The shared runtime exposes a C ABI. C++ consumers link statically by default.
if(BUILD_SHARED_LIBS)
    set_target_properties(rootcastle_npc PROPERTIES CXX_VISIBILITY_PRESET default)
    target_compile_definitions(rootcastle_npc PUBLIC RC_NPC_SHARED)
    target_compile_definitions(rootcastle_npc PRIVATE RC_NPC_EXPORTS)
endif()
if(MSVC)
    target_compile_options(rootcastle_npc PRIVATE /W4 /permissive-)
else()
    target_compile_options(rootcastle_npc PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

include(CTest)
if(BUILD_TESTING AND RC_NPC_BUILD_TESTS)
    add_executable(npc_tests tests/test_npc.cpp)
    target_link_libraries(npc_tests PRIVATE rootcastle::npc)
    add_test(NAME rootcastle_npc_regression COMMAND npc_tests)
endif()
if(RC_NPC_BUILD_EXAMPLES)
    add_executable(npc_console examples/console/main.cpp)
    target_link_libraries(npc_console PRIVATE rootcastle::npc)
    add_executable(npc_adapter_demo examples/engine_adapter/main.cpp)
    target_link_libraries(npc_adapter_demo PRIVATE rootcastle::npc)
endif()
if(RC_NPC_BUILD_BENCH)
    add_executable(npc_bench bench/main.cpp)
    target_link_libraries(npc_bench PRIVATE rootcastle::npc)
endif()

include(GNUInstallDirs)
install(TARGETS rootcastle_npc EXPORT RootcastleNPCTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/rootcastle DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT RootcastleNPCTargets
    NAMESPACE rootcastle::
    FILE RootcastleNPCTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RootcastleNPC)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/RootcastleNPCConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/RootcastleNPCConfig.cmake"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/RootcastleNPCTargets.cmake\")\n")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/RootcastleNPCConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/RootcastleNPCConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RootcastleNPC)
