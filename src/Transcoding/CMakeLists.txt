set(TRANSCODING_SRC Transcoding.cpp)

set(TRANSCODING_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

add_executable(Transcoding ${TRANSCODING_SRC})

target_link_libraries(Transcoding ${TRANSCODING_DEP_LIB})
