cmake_minimum_required(VERSION 3.14)
project(QuartzDB VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(QUARTZDB_BUILD_TESTS     "Build unit tests"         ON)
option(QUARTZDB_BUILD_EXAMPLES  "Build example programs"   ON)
option(QUARTZDB_BUILD_FUZZ      "Build fuzz targets"       OFF)
option(QUARTZDB_BUILD_BENCHMARKS "Build benchmarks"        OFF)
option(QUARTZDB_BUILD_TOOLS       "Build CLI tools"          ON)

option(QUARTZDB_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
# Catch2 is vendored at third_party/Catch2 for fully offline builds
option(QUARTZDB_USE_VENDORED_CATCH2 "Use vendored Catch2" ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Compiler warnings (applied to QuartzDB targets only; not third-party deps)
set(QUARTZDB_COMPILE_OPTIONS)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    list(APPEND QUARTZDB_COMPILE_OPTIONS
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
        -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Woverloaded-virtual -Wformat=2)
    if(QUARTZDB_WARNINGS_AS_ERRORS)
        list(APPEND QUARTZDB_COMPILE_OPTIONS -Werror)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    list(APPEND QUARTZDB_COMPILE_OPTIONS /W4 /utf-8)
    if(QUARTZDB_WARNINGS_AS_ERRORS)
        list(APPEND QUARTZDB_COMPILE_OPTIONS /WX)
    endif()
endif()

# Visibility
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Library sources
set(QUARTZDB_SOURCES
    src/common/Assertions.cpp
    src/common/Version.cpp
    src/common/Status.cpp
    src/common/Logger.cpp
    src/util/FileUtils.cpp
    src/storage/Page.cpp
    src/storage/DatabaseFile.cpp
    src/storage/PageAllocator.cpp
    src/serialization/Buffer.cpp
    src/serialization/BufferView.cpp
    src/serialization/VariableLengthInteger.cpp
    src/serialization/BinaryReader.cpp
    src/serialization/BinaryWriter.cpp
    src/serialization/Serializer.cpp
    src/format/FeatureFlags.cpp
    src/format/ObjectId.cpp
    src/format/FormatValidator.cpp
    src/pages/BasePage.cpp
    src/pages/HeaderPage.cpp
    src/pages/FreeListPage.cpp
    src/pages/DataPage.cpp
    src/pages/IndexPage.cpp
    src/pages/OverflowPage.cpp
    src/pages/MetadataPage.cpp
    src/pages/PageFactory.cpp
    src/pages/PageValidator.cpp
    src/pages/PageStatistics.cpp
    src/space/Extent.cpp
    src/space/AllocationPolicy.cpp
    src/space/FreeSpaceMap.cpp
    src/space/ExtentAllocator.cpp
    src/space/SpaceStatistics.cpp
    src/space/SpaceManager.cpp
    src/space/FragmentationAnalyzer.cpp
    src/btree/Key.cpp
    src/btree/BTreeNode.cpp
    src/btree/LeafNode.cpp
    src/btree/InternalNode.cpp
    src/btree/Cursor.cpp
    src/btree/SearchPath.cpp
    src/btree/NodeValidator.cpp
    src/btree/BTreeStatistics.cpp
    src/btree/SplitMergePlanner.cpp
    src/btree/BTree.cpp
    src/btree/Rebalance.cpp
    src/btree/TreeValidator.cpp
    src/wal/LogSequenceNumber.cpp
    src/wal/LogRecord.cpp
    src/wal/LogStatistics.cpp
    src/wal/LogBuffer.cpp
    src/wal/LogFile.cpp
    src/wal/LogWriter.cpp
    src/wal/LogReader.cpp
    src/wal/LogValidator.cpp
    src/wal/LogManager.cpp
    src/wal/BTreeWalAdapter.cpp
    src/recovery/CheckpointRecord.cpp
    src/recovery/LogReplayer.cpp
    src/recovery/CheckpointManager.cpp
    src/recovery/RecoveryValidator.cpp
    src/recovery/RecoveryManager.cpp
    src/core/Database.cpp
    src/diagnostics/DiagnosticReport.cpp
    src/diagnostics/ConsistencyChecker.cpp
    src/diagnostics/IntegrityScanner.cpp
    src/maintenance/BulkLoader.cpp
    src/maintenance/Vacuum.cpp
    src/metadata/Catalog.cpp
    src/metadata/VersionCompatibility.cpp
    src/instrumentation/Timer.cpp
    src/instrumentation/Counter.cpp
    src/instrumentation/Profiler.cpp
    src/instrumentation/MemoryStats.cpp
)

# Static library
add_library(quartzdb STATIC ${QUARTZDB_SOURCES})

target_include_directories(quartzdb
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(quartzdb
    PUBLIC
        $<$<CONFIG:Debug>:QUARTZDB_DEBUG>
)

if(QUARTZDB_COMPILE_OPTIONS)
    target_compile_options(quartzdb PRIVATE ${QUARTZDB_COMPILE_OPTIONS})
endif()

# std::filesystem support library (required on GCC < 9 and some Clang configurations)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(quartzdb PUBLIC stdc++fs)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(quartzdb PUBLIC c++fs)
endif()

# Subdirectories
if(QUARTZDB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(QUARTZDB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(QUARTZDB_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(QUARTZDB_BUILD_FUZZ)
    add_subdirectory(fuzz)
endif()

if(QUARTZDB_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS quartzdb
    EXPORT QuartzDBTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/quartz
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT QuartzDBTargets
    FILE QuartzDBTargets.cmake
    NAMESPACE QuartzDB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QuartzDB
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/QuartzDBConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/QuartzDBConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QuartzDB
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/QuartzDBConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/QuartzDBConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/QuartzDBConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QuartzDB
)
