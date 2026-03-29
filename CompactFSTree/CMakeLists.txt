cmake_minimum_required(VERSION 3.16)
project(CompactFSTree VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

# ---------------------------------------------------------------------------
# Static library
# ---------------------------------------------------------------------------

set(CFST_SOURCES
    src/core/node.h
    src/core/fs_traits.h
    src/core/compact_fs_tree.h
    src/core/compact_fs_tree.cpp
    src/builder/fs_tree_builder.h
    src/builder/fs_tree_builder.cpp
    src/scanner/fs_scanner.h
    src/scanner/fs_scanner.cpp
    src/scanner/fat_scanner.h
    src/scanner/fat_scanner.cpp
    src/scanner/apfs_scanner.h
    src/scanner/apfs_scanner.cpp
    src/scanner/hfsplus_scanner.h
    src/scanner/hfsplus_scanner.cpp
    src/scanner/ntfs_scanner.h
    src/scanner/ntfs_scanner.cpp
    src/scanner/posix_scanner.h
    src/scanner/posix_scanner.cpp
    src/scanner/scan_orchestrator.h
    src/scanner/scan_orchestrator.cpp
    src/serialization/fs_tree_serializer.h
    src/serialization/fs_tree_serializer.cpp
)

add_library(CompactFSTree STATIC ${CFST_SOURCES})

target_include_directories(CompactFSTree
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(CompactFSTree
    PUBLIC
        Qt6::Core
)

# MSVC: silence C4244 (uint64_t → uint32_t narrowing, intentional in some places).
if(MSVC)
    target_compile_options(CompactFSTree PRIVATE /W4 /wd4244 /wd4267)
else()
    target_compile_options(CompactFSTree PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# CLI test harness
# ---------------------------------------------------------------------------

add_executable(cfst_cli src/main.cpp)
target_link_libraries(cfst_cli PRIVATE CompactFSTree)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

enable_testing()

function(cfst_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE CompactFSTree)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

cfst_test(test_compact_fs_tree tests/test_compact_fs_tree.cpp)
cfst_test(test_builder         tests/test_builder.cpp)
cfst_test(test_synthetic_tree  tests/test_synthetic_tree.cpp)

# Serializer test uses QTemporaryFile so needs its own QCoreApplication.
add_executable(test_serializer tests/test_serializer.cpp)
target_link_libraries(test_serializer PRIVATE CompactFSTree)
add_test(NAME test_serializer COMMAND test_serializer)
