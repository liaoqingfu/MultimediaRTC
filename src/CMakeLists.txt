# include the ffmpeg include dir 
include_directories(${PROJECT_SOURCE_DIR}/inc)

# include the ffmpeg lib dir
link_directories(${PROJECT_SOURCE_DIR}/lib)

# ffmpeg libraries
set(FFMPEG_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

# ffmpeg libraries path 
set(CMAKE_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/inc)

add_executable(MultimediaRTC MultimediaRTC.cpp)
target_link_libraries(MultimediaRTC ${FFMPEG_LIB})


