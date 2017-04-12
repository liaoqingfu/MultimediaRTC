set(MUXMP4_SRC MuxMP4.cpp) 
set(MUXMP4_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

add_executable(MuxMP4 ${MUXMP4_SRC}) 
target_link_libraries(MuxMP4 ${MUXMP4_DEP_LIB})
