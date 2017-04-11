# include the ffmpeg include dir 
include_directories(${PROJECT_SOURCE_DIR}/inc)

# include the ffmpeg lib dir
link_directories(${PROJECT_SOURCE_DIR}/lib)

# ffmpeg libraries
set(FFMPEG_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)
# SDL libraries
set(SDL_LIB SDL.lib SDLmain.lib)

# ffmpeg libraries path 
set(CMAKE_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/inc)

set(SRC_LIST MultimediaRTC.cpp DeviceInfo.h DeviceInfo.cpp FFmpegInit.h FFmpegInit.cpp)

add_subdirectory(TestCase)

#//////////////////////////////////////
add_subdirectory(Demuxing)
add_subdirectory(Decoding)
add_subdirectory(ConvertRawData)
add_subdirectory(Encoding)
add_subdirectory(Muxing)
add_subdirectory(Transcoding)
#//////////////////////////////////////

add_executable(MultimediaRTC ${SRC_LIST})
target_link_libraries(MultimediaRTC ${FFMPEG_LIB} ${SDL_LIB})


