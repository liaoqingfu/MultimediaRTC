 
#define __STDC_CONSTANT_MACROS  
#include <stdio.h> 

#include "FFmpegInit.h"
#include "DeviceInfo.h"

extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"  
#include "libavformat/avformat.h"  
#include "libavfilter/avfilter.h"  
#include "libswscale/swscale.h"
#include "SDL/SDL.h"
};

#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit=0;

int SfpRefreshThread(void *opaque)
{
	thread_exit=0;
	while (!thread_exit) {
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}
	thread_exit=0;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

int main(int argc, char* argv[])
{
	// Step 1: Do some initialize work.
	InitFFmepg(); 
	
	ShowDShowDevice(); 
	ShowDShowDeviceOption(); 

	/*
		Step 2: Open the camera device.
	*/
	AVFormatContext* pFormatCtx = NULL; 
	pFormatCtx = avformat_alloc_context();
	AVInputFormat* pInputFormat = av_find_input_format("dshow"); 
	if (!pInputFormat)
	{
		printf("Can't find the dshow device!!\n"); 
		return -1; 
	}

	if (avformat_open_input(&pFormatCtx, "video=USB Camera", pInputFormat, NULL) != 0)
	{
		printf("Can't open input stream.\n"); 
		return -1; 
	}
	
	/*
		Step 3: Get stream and codec information.
	*/
	// Read packets of a media file to get stream information.
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		 printf("Can't find stream information.\n");

		 return -1; 
	}
	
	int videoIndex = -1; 
	for( int i = 0; i < pFormatCtx->nb_streams; i++)
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
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id); //codec_id: AV_CODEC_ID_RAWVIDEO
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1; 
	}
	
	//Initialize the pCondecCtx(AVCodecContext) to use the pCodec(AVCodec).
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Can't open codec.\n"); 
		return -1; 
	}
	
	/*
		Step 4: SDL 
	*/
	// SDL ---------------------------------------
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("Can't initialize SLD -%S\n", SDL_GetError()); 
		return -1; 
	}
	int screenW = 0, screenH=0; 
	SDL_Surface* pScreen = NULL; 
	screenW = pCodecCtx->width; 
	screenH = pCodecCtx->height; 
	pScreen = SDL_SetVideoMode(screenW, screenH, 0, 0); 
	if (!pScreen)
	{
		printf("SDL: Can't set video mode - exiting:%s\n", SDL_GetError());
		return -1; 
	}
	SDL_Overlay *bmp; 
	bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY, pScreen); 
	SDL_Rect rect;
	rect.x = 0;    
	rect.y = 0;    
	rect.w = screenW;    
	rect.h = screenH; 
	// SDL end -----------------------------------
	/*
		Step 5: Get raw frames video data. 
	*/
	AVPacket* pPacket = av_packet_alloc(); 
	AVFrame* pFrame = NULL;
	AVFrame* pFrameYUV = NULL; 
	pFrame = av_frame_alloc(); 
	pFrame = av_frame_alloc(); 
	struct SwsContext* imgConverCtx = NULL; 
	imgConverCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, 
					pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
					AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
	SDL_Thread* vodeoThread = SDL_CreateThread(SfpRefreshThread, NULL); 

	SDL_WM_SetCaption("MultimediaRTC", NULL);
	SDL_Event event;
	int ret  = 0, gotPicture = 0; 
	for (;;) 
	{
		//Wait
		SDL_WaitEvent(&event);
		if(event.type==SFM_REFRESH_EVENT)
		{
			//------------------------------
			if(av_read_frame(pFormatCtx, pPacket) >= 0)
			{
				if(pPacket->stream_index==videoIndex)
				{
					ret = avcodec_decode_video2(pCodecCtx, pFrame, &gotPicture, pPacket);
					if(ret < 0)
					{
						printf("Decode Error.\n");
						return -1;
					}
					if(gotPicture)
					{
						SDL_LockYUVOverlay(bmp);
						//pFrameYUV->data[0]=bmp->pixels[0];
						//pFrameYUV->data[1]=bmp->pixels[2];
						//pFrameYUV->data[2]=bmp->pixels[1];     
						//pFrameYUV->linesize[0]=bmp->pitches[0];
						//pFrameYUV->linesize[1]=bmp->pitches[2];   
						//pFrameYUV->linesize[2]=bmp->pitches[1];
						//sws_scale(imgConverCtx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

						SDL_UnlockYUVOverlay(bmp); 
						SDL_DisplayYUVOverlay(bmp, &rect); 
					}
				}
				av_free_packet(pPacket);
			}
			else
			{
				//Exit Thread
				thread_exit=1;
			}
		}
		else if(event.type==SDL_QUIT)
		{
			thread_exit=1;
		}
		else if(event.type==SFM_BREAK_EVENT)
		{
			break;
		}
	}
	sws_freeContext(imgConverCtx);

	SDL_Quit(); 
	/*
		Step 6:
	*/
	av_free(pFrameYUV); 
	av_packet_free(&pPacket); 
	avcodec_close(pCodecCtx); 
	avformat_close_input(&pFormatCtx); 
	
	return 0;
}


