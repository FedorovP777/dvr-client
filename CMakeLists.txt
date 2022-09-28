cmake_minimum_required(VERSION 3.16)
project(libav_test)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
        /lib
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/local/lib64
        /usr/lib/x86_64-linux-gnu)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_C_FLAGS_RELEASE "-O3")


find_package(yaml-cpp)
find_package(fmt)
find_package(AWSSDK REQUIRED COMPONENTS s3)
find_package(CURL REQUIRED)

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)
#
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)

find_path(LIBSWSCALE_INCLUDE_DIR libswscale/swscale.h)
find_library(SWSCALE_LIBRARY swscale)
find_library(AVUTIL_LIBRARY avutil)
set(SRC_LIST
        src/main.cpp
        src/EventService.h
        src/Context.h
        src/Logger.h
        src/Packet.h
        src/Options.h
        src/InputStream.h
        src/OutputStream.h
        src/utils.h
        src/ConfigService.h
        src/Rescale.h
        src/FormatFilename.h
        src/StreamWorker.h
        src/S3Client.h
        src/S3Service.h
        src/CurlHttpClient.h
        src/ObjectPool.h
        src/StreamHealthChecker.h)
add_executable(libav_test ${SRC_LIST})
set(LINK_LIBS
        yaml-cpp
        fmt
        uv
        pthread
        ${AVCODEC_LIBRARY}
        ${AVFORMAT_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${SWSCALE_LIBRARY}
        ${CURL_LIBRARIES}
        ${AWSSDK_LINK_LIBRARIES}
        )
set(INCLUDE_DIR ${AVCODEC_INCLUDE_DIR} ${LIBSWSCALE_INCLUDE_DIR} ${CURL_INCLUDE_DIRS})
target_include_directories(libav_test PRIVATE ${INCLUDE_DIR})
target_link_libraries(libav_test PRIVATE ${LINK_LIBS})
################################
# PVS-Studio
################################
#if(NOT DEFINED ENV{PROD_BUILD})
#    include(./PVS-Studio.cmake)
#    pvs_studio_add_target(TARGET pvs-studio ALL
#            OUTPUT FORMAT errorfile
#            ANALYZE ${PROJECT_NAME}
#            MODE GA:1,2 OP
#            LOG target.err)
#endif()
#

################################
# GoogleTest
################################
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()
add_executable(
        run_tests
        tests/config_file_test.cpp
)
target_link_libraries(
        run_tests
        gtest_main
        ${LINK_LIBS}
)
target_include_directories(run_tests PRIVATE ${INCLUDE_DIR})
include(GoogleTest)
gtest_discover_tests(run_tests)

################################
# Benchmark
################################
FetchContent_Declare(googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG main)

FetchContent_MakeAvailable(
        googlebenchmark
)
add_executable(
        run_tests_benchmark
        tests/benchmark.cpp
)

target_link_libraries(
        run_tests_benchmark
        benchmark::benchmark
)
target_include_directories(run_tests_benchmark PRIVATE ${INCLUDE_DIR})
################################
# ClangTidy
################################
project(clang_tidy)
set(CMAKE_CXX_CLANG_TIDY
        clang-tidy;
        -header-filter=.;
        -checks=*;)

add_executable(clang_tidy ${SRC_LIST})
target_include_directories(clang_tidy PRIVATE ${INCLUDE_DIR})
target_link_libraries(clang_tidy PRIVATE ${LINK_LIBS})