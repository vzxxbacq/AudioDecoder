cmake_minimum_required(VERSION 3.16)
project(AudioDecoder)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(FFmpeg) #add here the list of ffmpeg components required
find_package(GLOG)
find_package(GTest)
message(${GTEST_LIBRARIES})
if(GTEST_FOUND)
    message("GTEST include:" ${GTEST_INCLUDE_DIRS})
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

if(FFMPEG_FOUND)
    #  FFMPEG_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
    #  FFMPEG_LIBRARIES     - Link these to use the required ffmpeg components.
    include_directories(${FFMPEG_INCLUDE_DIRS})
endif()

if(GLOG_FOUND)
    include_directories(${GLOG_INCLUDE_DIRS})
endif()

add_subdirectory(src)
add_subdirectory(test)