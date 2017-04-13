set (DEMUXFLV_SRC DemuxFLV.cpp)

set (DEMUXFLV_DEP_LIB avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib postproc.lib swresample.lib swscale.lib)

add_executable(DemuxFLV ${DEMUXFLV_SRC})

target_link_libraries(DemuxFLV ${DEMUXFLV_DEP_LIB})

