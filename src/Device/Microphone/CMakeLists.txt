set(MICRO_SRC CaptureDevices.h CaptureDevices.cpp Microphone.cpp) 

set(MICRO_FFMPEG_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)
set(MICRO_SDL_DEP_LIB SDL.lib SDLmain.lib)

add_executable(Microphone ${MICRO_SRC}) 
target_link_libraries(Microphone ${MICRO_FFMPEG_DEP_LIB} ${MICRO_SDL_DEP_LIB})
