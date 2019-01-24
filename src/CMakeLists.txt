add_library(drmaa_cpp SHARED
        drmaa.cpp
        watcher.cpp
        )

target_include_directories(drmaa_cpp
        PUBLIC
        $<BUILD_INTERFACE:${DRMAA_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
        )

set_target_properties(drmaa_cpp
        PROPERTIES
        PUBLIC_HEADER
        "${DRMAA_INCLUDE_DIR}/drmaa/drmaa.h;"
        "${DRMAA_INCLUDE_DIR}/drmaa/drmaa.hpp;"
        "${DRMAA_INCLUDE_DIR}/drmaa/watcher.hpp"
        )

target_link_libraries(drmaa_cpp PUBLIC drmaa ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS drmaa_cpp
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/drmaa
        )
