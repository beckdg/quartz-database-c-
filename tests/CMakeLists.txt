if(QUARTZDB_USE_VENDORED_CATCH2)
    if(NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/Catch2/CMakeLists.txt)
        message(FATAL_ERROR "Vendored Catch2 requested but not found at third_party/Catch2. "
                            "Run: git clone --branch v3.4.0 https://github.com/catchorg/Catch2.git third_party/Catch2")
    endif()
    set(CATCH2_DIR ${CMAKE_SOURCE_DIR}/third_party/Catch2)
    add_subdirectory(${CATCH2_DIR} ${CMAKE_BINARY_DIR}/_deps/catch2)
    list(APPEND CMAKE_MODULE_PATH ${CATCH2_DIR}/extras)
else()
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

# Collect test sources
file(GLOB TEST_SOURCES "*.cpp")

add_executable(QuartzDB_tests ${TEST_SOURCES})

target_link_libraries(QuartzDB_tests
    PRIVATE
        quartzdb
        Catch2::Catch2WithMain
)

target_compile_definitions(QuartzDB_tests
    PRIVATE
        $<$<CONFIG:Debug>:QUARTZDB_DEBUG>
)

if(QUARTZDB_COMPILE_OPTIONS)
    target_compile_options(QuartzDB_tests PRIVATE ${QUARTZDB_COMPILE_OPTIONS})
endif()

include(CTest)
if(COMMAND catch_discover_tests)
    catch_discover_tests(QuartzDB_tests)
endif()
