add_executable(Decode decode.cpp)
target_link_libraries(Decode ${FFMPEG_LIBRARIES})
target_link_libraries(Decode ${GLOG_LIBRARIES})