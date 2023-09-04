cmake_minimum_required(VERSION 3.14)

add_executable(bench_btree btree_bench.cpp)
add_executable(bench_wal wal_bench.cpp)
add_executable(bench_recovery recovery_bench.cpp)

foreach(target bench_btree bench_wal bench_recovery)
    target_link_libraries(${target} PRIVATE quartzdb)
    if(QUARTZDB_COMPILE_OPTIONS)
        target_compile_options(${target} PRIVATE ${QUARTZDB_COMPILE_OPTIONS})
    endif()
endforeach()
