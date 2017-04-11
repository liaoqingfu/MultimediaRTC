set(CONVERT_RD_SRC ConverRawData.cpp)

set(CONVERT_RD_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

add_executable(ConvertRawData ${CONVERT_RD_SRC})

target_link_libraries(ConvertRawData ${CONVERT_RD_DEP_LIB})
