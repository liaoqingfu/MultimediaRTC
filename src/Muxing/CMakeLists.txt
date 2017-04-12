set(MUXING_SRC Muxing.cpp)

set(MUXING_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)
add_subdirectory(MuxMP4)
add_executable(Muxing ${MUXING_SRC})
target_link_libraries(Muxing ${MUXING_DEP_LIB})
