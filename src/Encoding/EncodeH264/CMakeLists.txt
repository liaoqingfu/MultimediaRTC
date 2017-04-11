set(ENH264_SRC EncodeH264.cpp)

set(ENH264_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

add_executable(EncodeH264 ${ENH264_SRC})

target_link_libraries(EncodeH264 ${ENH264_DEP_LIB})
