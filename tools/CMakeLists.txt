cmake_minimum_required(VERSION 3.14)

add_executable(quartzdb_verify quartzdb_verify.cpp)
target_link_libraries(quartzdb_verify PRIVATE quartzdb)

if(QUARTZDB_COMPILE_OPTIONS)
    target_compile_options(quartzdb_verify PRIVATE ${QUARTZDB_COMPILE_OPTIONS})
endif()

install(TARGETS quartzdb_verify RUNTIME DESTINATION bin)
