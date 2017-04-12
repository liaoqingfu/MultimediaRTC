set(ENCODEAAC_SRC EncodeAAC.cpp)

set(ENCODEAAC_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib) 

add_executable(EncodeAAC ${ENCODEAAC_SRC})
target_link_libraries(EncodeAAC ${ENCODEAAC_DEP_LIB})