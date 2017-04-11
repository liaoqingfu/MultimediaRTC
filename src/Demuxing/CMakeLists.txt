set(DEMUXING_SRC demuxing.cpp)

set(DEMUXING_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

add_executable(Demuxing ${DEMUXING_SRC})

target_link_libraries(Demuxing ${DEMUXING_DEP_LIB})
