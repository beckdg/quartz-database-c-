cmake_minimum_required(VERSION 3.14)

add_executable(fuzz_serialization fuzz_serialization.cpp)
add_executable(fuzz_page fuzz_page.cpp)
add_executable(fuzz_recovery fuzz_recovery.cpp)

set(FUZZ_TARGETS fuzz_serialization fuzz_page fuzz_recovery)

foreach(target ${FUZZ_TARGETS})
    target_link_libraries(${target} PRIVATE quartzdb)
    target_compile_definitions(${target} PRIVATE FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)

    if(DEFINED ENV{LIB_FUZZING_ENGINE})
        target_link_libraries(${target} PRIVATE "$ENV{LIB_FUZZING_ENGINE}")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE -fsanitize=fuzzer,address)
        target_link_options(${target} PRIVATE -fsanitize=fuzzer,address)
    endif()
endforeach()
