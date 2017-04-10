#include "FFmpegInit.h"

#define __STDC_CONSTANT_MACROS  
extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
};

void InitFFmepg()
{
	// Initialize libavformat and register all the muxers, demuxers and protocols.
	av_register_all(); 
	avformat_network_init();

	// Register devices
	avdevice_register_all(); 
}