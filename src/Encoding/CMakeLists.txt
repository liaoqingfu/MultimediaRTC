set(ENCODING_SRC Encoding.cpp)

set(ENCODING_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

add_executable(Encoding ${ENCODING_SRC})

add_subdirectory(EncodeH264)
add_subdirectory(EncodeAAC)

target_link_libraries(Encoding ${ENCODING_DEP_LIB})
