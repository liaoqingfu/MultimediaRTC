#include <stdio.h>  

#define __STDC_CONSTANT_MACROS  

extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"  
#include "libavfilter/avfilter.h"  
};


int main(int argc, char* argv[])
{
	/*
		Step 1: Do some initialize work.
	*/
	// Register device. 
	avdevice_register_all();
	// Initialize libavformat and register all the muxers, demuxers and protocols.
	av_register_all(); 
	//Do global initialization of network components.
	avformat_network_init(); 

	/*
		Step 2: Open the device.
	*/
	// Allocate an AVFormatContext.
	AVFormatContext* pFormatCtx = NULL; 
	pFormatCtx = avformat_alloc_context(); 

	AVDictionary* options = NULL; 
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat* pInputFormat = NULL; 
	pInputFormat = av_find_input_format("dshow"); 
	printf("==============Device Info==============\n"); 
	//avformat_open_input(&pFormatContext, "Video=Logitech HD Webcam C310",pInputFormat, &options); 
	avformat_open_input(&pFormatCtx, "video=dummy", pInputFormat, &options); 
	printf("=======================================\n"); 

	/*
		Step 3: Get stream information and get the codec information.
	*/
	//Read packets of a media file to get stream information. 
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Can't find stream information.\n"); 
	}
	int videoIndex = -1; 
	for (int i = 0; i < pFormatCtx->nb_streams; ++i)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoIndex = i; 
			break; 
		}
	}
	if (videoIndex == -1)
	{
		printf("Can't find a video stream.\n"); 
		return -1; 
	}

	AVCodecContext* pCodecCtx = NULL; 
	AVCodec* pCodec = NULL; 
	pCodecCtx = pFormatCtx->streams[videoIndex]->codec; 
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id); 
	if (pCodec == NULL)
	{
		printf("Codec not found.\n"); 
		return -1; 
	}
	// Initialize the AVCodecContext to use the given AVCodec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Can't open codec.\n"); 
		return -1; 
	}

	/*
		Step 4: Get raw frames video data of the video captured from the camera 
		Then write the raw frames video data to a YUV420 type local file. 
	*/
	AVFrame* pFrame = NULL; 
	pFrame = av_frame_alloc();
	AVFrame* pFrameYUV = NULL; 
	pFrameYUV = av_frame_alloc(); 

	AVPacket* pPacket = av_packet_alloc(); 

	FILE* fpYUV = fopen("output.yuv", "wb+"); 

	int got_picture = 0; 

	for (;;)
	{
		if (av_read_frame(pFormatCtx, pPacket) >= 0)
		{
			if (pPacket->stream_index == videoIndex)
			{
				if (avcodec_decode_video2(pCodecCtx, pFrameYUV, &got_picture, pPacket) < 0)
				{
					printf("Decode Error.\n"); 
					return -1; 
				}
			}

			int y_size = pCodecCtx->width*pCodecCtx->height;
			fwrite(pFrameYUV->data[0], 1, y_size, fpYUV);	   //Y   
			fwrite(pFrameYUV->data[1], 1, y_size / 4, fpYUV);  //U  
			fwrite(pFrameYUV->data[2], 1, y_size / 4, fpYUV);  //V  

		}
		else 
		{
			break; 
		}
	}

	/*
		Step 5: Read the raw frames video data(YUV420), encode(H.264), formated(mp4) 
	*/
	   
	                                                   


	/*
		Step 6: Free the resource.
	*/

	fclose(fpYUV); 
	av_packet_free(&pPacket); 
	av_frame_free(&pFrame); 
	avcodec_free_context(&pCodecCtx); 
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx); 
	avformat_free_context(pFormatCtx); 


	return 0;
}


