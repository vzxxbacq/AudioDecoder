# add_executable(Decode decode.cpp)
add_library(decode SHARED decode.cpp stft.cpp)
#target_link_libraries(Decode ${FFMPEG_LIBRARIES} ${GLOG_LIBRARIES})
target_link_libraries(decode ${FFMPEG_LIBRARIES} ${GLOG_LIBRARIES} ${FFTW_LIBRARIES})