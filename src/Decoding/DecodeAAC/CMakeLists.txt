set(DECODEAAC_SRC DecodeAAC.cpp) 
set(DECODEAAC_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

add_executable(DecodeAAC ${DECODEAAC_SRC}) 
target_link_libraries(DecodeAAC ${DECODEAAC_DEP_LIB})
