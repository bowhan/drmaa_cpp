add_executable(drmaa_test test.cpp)
target_link_libraries(drmaa_test drmaa_cpp)

install(TARGETS drmaa_test
        RUNTIME DESTINATION bin
        )
