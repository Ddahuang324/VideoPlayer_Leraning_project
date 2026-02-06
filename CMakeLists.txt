cmake_minimum_required(VERSION 3.12)
project(YiboServer VERSION 1.0.0 LANGUAGES CXX)

# C++17 标准配置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
add_compile_options(-Wall -Wextra -Wpedantic)

# Debug/Release模式
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 包含目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party
)

# 第三方库配置
# nlohmann/json (单头文件)
include_directories(${PROJECT_SOURCE_DIR}/third_party/nlohmann)

# MySQL
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)
include_directories(${MYSQL_INCLUDE_DIRS})
link_directories(${MYSQL_LIBRARY_DIRS})

# SQLite3
find_package(SQLite3 REQUIRED)
include_directories(${SQLite3_INCLUDE_DIRS})

# OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# 源文件收集
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

# 创建静态库
if(SOURCES)
    add_library(yiboserver_lib STATIC ${SOURCES})
    target_link_libraries(yiboserver_lib
        ${MYSQL_LIBRARIES}
        SQLite::SQLite3
        OpenSSL::SSL
        OpenSSL::Crypto
        pthread
    )
endif()

# 创建可执行文件
add_executable(yiboserver src/main.cpp)

if(SOURCES)
    target_link_libraries(yiboserver yiboserver_lib)
else()
    target_link_libraries(yiboserver
        ${MYSQL_LIBRARIES}
        SQLite::SQLite3
        OpenSSL::SSL
        OpenSSL::Crypto
        pthread
    )
endif()

# 创建独立的日志服务器可执行文件
add_executable(yiboserver_logger src/logger_server_main.cpp)

if(SOURCES)
    target_link_libraries(yiboserver_logger yiboserver_lib)
else()
    target_link_libraries(yiboserver_logger
        ${MYSQL_LIBRARIES}
        SQLite::SQLite3
        OpenSSL::SSL
        OpenSSL::Crypto
        pthread
    )
endif()


# 测试配置
enable_testing()
if(EXISTS ${PROJECT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()

# 安装配置
install(TARGETS yiboserver DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)