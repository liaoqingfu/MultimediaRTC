#define __STDC_CONSTANT_MACROS
#include <stdio.h>

#include "FFmpegInit.h"
#include "DeviceInfo.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "SDL/SDL.h"
};

const char* deviceName = "video=Logitech HD Webcam C310"; 
//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

#define OUTPUT_YUV420P 1

int threadExit=0;

int SfpRefreshThread(void *opaque)
{
	threadExit=0;
	while (!threadExit) 
	{
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}

	threadExit=0;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}

int main(int argc, char* argv[])
{
	/*
		Step 1: Do some initialize work.
	*/
	InitFFmepg(); 
	ShowDShowDevice(); 
	ShowDShowDeviceOption(); 

	/*
		Step 2: Open the camera device.
	*/
	AVFormatContext	*pFormatCtx = NULL;
	pFormatCtx = avformat_alloc_context();
	AVInputFormat *pInputFormat = av_find_input_format("dshow");
	if (!pInputFormat)
	{
		printf("Can't find the dshow device!!\n"); 
	}

	if (avformat_open_input(&pFormatCtx, deviceName, pInputFormat, NULL) != 0)
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
		
	int	videoIndex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++) 
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

	// Initialize the pCodecCtx(AVCodecContext) to use pCodec(AVCodec)
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Can't open codec.\n");
		return -1;
	}

	/*
		Step 4: SDL
	*/
	//SDL----------------------------
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) 
	{  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	} 
	int screenW = 0,screenH = 0;
	SDL_Surface *pScreen; 
	screenW = pCodecCtx->width;
	screenH = pCodecCtx->height;
	printf("The YUV pixel width is: %d\n", pCodecCtx->width); 
	printf("The YUV pixel height is: %d\n", pCodecCtx->height); 

	pScreen = SDL_SetVideoMode(screenW, screenH, 0, 0);

	if (!pScreen) 
	{  
		printf("SDL: could not set video mode - exiting:%s\n", SDL_GetError());  
		return -1;
	}

	SDL_Overlay *displayOverlay = NULL; 
	displayOverlay = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY, pScreen); 

	SDL_Rect rect;
	rect.x = 0;    
	rect.y = 0;    
	rect.w = screenW;    
	rect.h = screenH;  
	// SDL End------------------------

	/*
		Step 5: Get raw frames video data.
	*/
	AVPacket* pPacket = av_packet_alloc(); 
	AVFrame	*pFrame = NULL, *pFrameYUV = NULL;
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	int ret = -1, gotPicture = 0;

	struct SwsContext *imgConvertCtx;
	imgConvertCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, 
				pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, 
				AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
	//------------------------------
	SDL_Thread *videoThread = SDL_CreateThread(SfpRefreshThread, NULL);
	SDL_WM_SetCaption("MultimediaRTC", NULL);
	// Event Loop
	SDL_Event event;

#if OUTPUT_YUV420P 
	FILE *pFileYUV=fopen("output.yuv","wb+");  
#endif  

	for ( ; ; ) 
	{
		// Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT)
		{
			// Return the next frame of a stream
			if (av_read_frame(pFormatCtx, pPacket) >= 0)
			{
				if (pPacket->stream_index == videoIndex)
				{
					// Decode the video frame of size pPacket->size from pPacket->data into pFrame.
					ret = avcodec_decode_video2(pCodecCtx, pFrame, &gotPicture, pPacket);
					if (ret < 0)
					{
						printf("Decode Error.\n");
						return -1;
					}
					if (gotPicture)
					{
						SDL_LockYUVOverlay(displayOverlay);
						pFrameYUV->data[0] = displayOverlay->pixels[0];
						pFrameYUV->data[1] = displayOverlay->pixels[2];
						pFrameYUV->data[2] = displayOverlay->pixels[1];     
						pFrameYUV->linesize[0] = displayOverlay->pitches[0];
						pFrameYUV->linesize[1] = displayOverlay->pitches[2];   
						pFrameYUV->linesize[2] = displayOverlay->pitches[1];
						// Scale the image slice in pFrame and put the resulting 
						// scaled slice in the image in pFrameYUV
						sws_scale(imgConvertCtx, 
							(const unsigned char* const*)pFrame->data, 
							pFrame->linesize, 0, pCodecCtx->height, 
							pFrameYUV->data, pFrameYUV->linesize);

#if OUTPUT_YUV420P  
						int y_size=pCodecCtx->width*pCodecCtx->height;    
						fwrite(pFrameYUV->data[0], 1, y_size, pFileYUV);  //Y   
						fwrite(pFrameYUV->data[1], 1, y_size/4, pFileYUV);  //U  
						fwrite(pFrameYUV->data[2], 1, y_size/4, pFileYUV);  //V  
#endif 
						SDL_UnlockYUVOverlay(displayOverlay); 	
						SDL_DisplayYUVOverlay(displayOverlay, &rect); 

					}
				}
				av_free_packet(pPacket);
			}
			else
			{
				//Exit Thread
				threadExit = 1;
			}
		}
		else if (event.type == SDL_QUIT)
		{
			threadExit = 1;
		}
		else if(event.type == SFM_BREAK_EVENT)
		{
			break;
		}

	}
	sws_freeContext(imgConvertCtx);

	/*
		Step 5: Encode the raw frames video data into packet(h.264).
	*/


	/*
		Step 6: Format the packet data into mp4. 
	*/


	/*
		Step 7: Do some clean up work.
	*/
	SDL_Quit();
#if OUTPUT_YUV420P 
	fclose(pFileYUV);
#endif 
	av_packet_free(&pPacket); 
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}
