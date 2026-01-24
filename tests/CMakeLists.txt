# GoogleTest配置
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

# 单元测试
file(GLOB UNIT_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp")

if(UNIT_TEST_SOURCES)
    add_executable(unit_tests ${UNIT_TEST_SOURCES})
    target_link_libraries(unit_tests
        gtest_main
        yiboserver_lib
    )

    include(GoogleTest)
    gtest_discover_tests(unit_tests)
endif()
