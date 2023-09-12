add_executable(QuartzDB_example
    main.cpp
)

target_link_libraries(QuartzDB_example
    PRIVATE
        quartzdb
)

if(QUARTZDB_COMPILE_OPTIONS)
    target_compile_options(QuartzDB_example PRIVATE ${QUARTZDB_COMPILE_OPTIONS})
endif()
