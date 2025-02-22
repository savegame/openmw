file(GLOB OPENCS_TESTS_SRC_FILES
    main.cpp
    model/world/testinfocollection.cpp
)

source_group(apps\\openmw-cs-tests FILES ${OPENCS_TESTS_SRC_FILES})

openmw_add_executable(openmw-cs-tests ${OPENCS_TESTS_SRC_FILES})

target_include_directories(openmw-cs-tests SYSTEM PRIVATE ${GTEST_INCLUDE_DIRS})
target_include_directories(openmw-cs-tests SYSTEM PRIVATE ${GMOCK_INCLUDE_DIRS})

target_link_libraries(openmw-cs-tests PRIVATE
    openmw-cs-lib
    GTest::GTest
    GMock::GMock
)

if (UNIX AND NOT APPLE)
    target_link_libraries(openmw-cs-tests PRIVATE ${CMAKE_THREAD_LIBS_INIT})
endif()

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(openmw-cs-tests PRIVATE --coverage)
    target_link_libraries(openmw-cs-tests PRIVATE gcov)
endif()

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16 AND MSVC)
    target_precompile_headers(openmw-cs-tests PRIVATE
        <gtest/gtest.h>
    )
endif()
