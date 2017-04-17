#define __STDC_CONSTANT_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CaptureDevices.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libswresample/swresample.h"
#include "SDL/SDL.h"
};
#include <string>
using namespace std;

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)


#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
int threadExit=0;

string WConverToUTF8(wstring szText); 

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
	av_register_all(); 
	avformat_network_init();
	avdevice_register_all(); 

	/*
		Step 2: Open the microphone device.
	*/
	AVFormatContext	*pFormatCtx = NULL;
	pFormatCtx = avformat_alloc_context();
	AVInputFormat *pInputFormat = av_find_input_format("dshow");
	if (!pInputFormat)
	{
		printf("Can't find the dshow device!!\n"); 
	}

	CaptureDevices capDevice; 
	vector<wstring> audioDevices; 

	capDevice.GetAudioDevices(&audioDevices);
	string str = WConverToUTF8(audioDevices[0]); 
	string audiodev = "audio=" + str; 
	
	
	if (avformat_open_input(&pFormatCtx, audiodev.c_str(), pInputFormat, NULL) != 0)
	{
		printf("Can't open input audio stream.\n");
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
		
	int	audioIndex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++) 
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioIndex = i;
			break;
		}
	}
	if (audioIndex == -1)
	{
		printf("Can't find a audio stream.\n");
		return -1;
	}

	AVCodecContext* pCodecCtx = NULL;
	AVCodec* pCodec = NULL;
	pCodecCtx = pFormatCtx->streams[audioIndex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Can't open codec.\n");
		return -1;
	}

	/*
		Step 4: SDL
	*/
	//SDL----------------------------
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) 
	{  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	} 
	//------------------------------
	SDL_Thread *audioThread = SDL_CreateThread(SfpRefreshThread, NULL);
	SDL_WM_SetCaption("Capture Microphone", NULL);

	/*
		Step 5: Decode audio stream and write to output file(PCM).
	*/
	AVPacket *packet = NULL;
	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(packet);
	int ret = -1, gotPicture = 0;
	// Out Audio Param.
	uint64_t outChannelLayout = AV_CH_LAYOUT_STEREO;
	// nb_samples: AAC-1024 MP3-1152
	int outNSamples = pCodecCtx->frame_size;
	AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_S16;
	int outSampleRate = 44100;
	int outChannels = av_get_channel_layout_nb_channels(outChannelLayout);
	// Out Buffer Size.
	int outBufferSize = av_samples_get_buffer_size(NULL, outChannels, 
		outNSamples, outSampleFmt, 1);

	// FIX: Some Codec's Context Information is missing.
	int64_t inChannelLayout = 0;
	inChannelLayout = av_get_default_channel_layout(pCodecCtx->channels);

	// Swr
	struct SwrContext *auConvertCtx = NULL;
	auConvertCtx = swr_alloc();
	auConvertCtx = swr_alloc_set_opts(auConvertCtx, outChannelLayout, outSampleFmt,
		outSampleRate, inChannelLayout, pCodecCtx->sample_fmt,
		pCodecCtx->sample_rate, 0, NULL);
	swr_init(auConvertCtx);

	uint8_t	*outBuffer = NULL;
	outBuffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
	AVFrame	*pFrame = NULL;
	pFrame = av_frame_alloc();



	FILE *pFilePCM=fopen("c://work//output.pcm","wb+");  
	// Event Loop
	int index = 0; 
	SDL_Event event;
	for ( ; ; ) 
	{
		// Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT)
		{
			// Return the next frame of a stream
			if (av_read_frame(pFormatCtx, packet) >= 0)
			{
				if (packet->stream_index == audioIndex)
				{
					// Decode the video frame of size pPacket->size from pPacket->data into pFrame.
					ret = avcodec_decode_audio4(pCodecCtx, pFrame, &gotPicture, packet);
					if (ret < 0)
					{
						printf("Error in decoding audio frame.\n");
						return -1;
					}

					if ( gotPicture > 0 )
					{
						swr_convert(auConvertCtx, &outBuffer, MAX_AUDIO_FRAME_SIZE,
							(const uint8_t **)pFrame->data , pFrame->nb_samples);

						printf("index:%5d\t pts:%lld\t packet size:%d\n", index, packet->pts, packet->size);
						// Write PCM
						fwrite(outBuffer, 1, outBufferSize, pFilePCM);
						index++;
					}
				}
				av_free_packet(packet);
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
	/*
		Step 6: Clean up.
	*/
	swr_free(&auConvertCtx); 
	SDL_Quit();

	fclose(pFilePCM);
	av_free(outBuffer); 
	av_packet_free(&packet); 
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}

string WConverToUTF8(wstring szText)
{
	WCHAR *chBuf;
	char *p;
	int iLen;

	// 	iLen = MultiByteToWideChar(CP_ACP, 0, szText.c_str(), -1 , NULL, 0);
	// 	chBuf = new WCHAR[iLen + 1];
	// 	ZeroMemory(chBuf, iLen * sizeof(WCHAR));
	// 	iLen = MultiByteToWideChar(CP_ACP, 0, szText.c_str(), -1, chBuf, iLen + 1);

	iLen = WideCharToMultiByte(CP_UTF8, 0, szText.c_str(), -1 , NULL, 0, NULL, NULL);
	p = new CHAR[iLen + 1];
	ZeroMemory(p, iLen * sizeof(CHAR));
	iLen = WideCharToMultiByte(CP_UTF8, 0, szText.c_str(), -1,  p, iLen + 1, NULL, NULL);

	//delete []chBuf;
	string re(p);
	delete []p;
	return re;
}