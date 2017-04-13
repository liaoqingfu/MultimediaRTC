set(DECODING_SRC decoding.cpp)

set(DECODING_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

add_subdirectory(DecodeAAC)
add_executable(Decoding ${DECODING_SRC})

target_link_libraries(Decoding ${DECODING_DEP_LIB})
