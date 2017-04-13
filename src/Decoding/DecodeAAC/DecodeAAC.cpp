/*
 * This program decode audio streams (AAC,MP3 ...) to PCM data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
};

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

int main(int argc, char* argv[])
{
	/*
		Step 1: FFmpeg Init.
	*/
	av_register_all();
	avformat_network_init();

	/*
		Step 2: Open input.
	*/



    int ret;
	uint32_t len = 0;

	int index = 0;



	FILE *pOutFile = fopen("c://work//output.pcm", "wb");
	const char* pOutFileName = "c://work//cuc_ieschool.aac";

	AVFormatContext	*pFormatCtx = NULL;
	pFormatCtx = avformat_alloc_context();

	// Open
	if (avformat_open_input(&pFormatCtx, pOutFileName, NULL, NULL) != 0)
	{
		printf("Can't open input stream.\n");
		return -1;
	}

	/*
		Step 3: Prepare codec for the stream to decode.
	*/
	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Can't find stream information.\n");
		return -1;
	}
	// Dump valid information onto standard error
	av_dump_format(pFormatCtx, 0, pOutFileName, false);

	// Find the first audio stream
	int audioStream = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioStream = i;
			break;
		}
	}

	if (audioStream == -1)
	{
		printf("Can't find a audio stream.\n");
		return -1;
	}

	// Get a pointer to the codec context for the audio stream.
	AVCodecContext *pCodecCtx = NULL;
	pCodecCtx = pFormatCtx->streams[audioStream]->codec;
	
	// Find the decoder for the audio stream.
	AVCodec	*pCodec = 0;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}

	// Open codec.
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Can't open codec.\n");
		return -1;
	}

	/*
		Step 4: Decode audio stream and write to output file(PCM).
	*/
	AVPacket *packet = NULL;
	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(packet);

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
	int gotPicture = 0;
	while (av_read_frame(pFormatCtx, packet) >= 0)
	{
		if (packet->stream_index == audioStream)
		{
			ret = avcodec_decode_audio4( pCodecCtx, pFrame, &gotPicture, packet);
			if ( ret < 0 ) 
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
				fwrite(outBuffer, 1, outBufferSize, pOutFile);
				index++;
			}
		}
		av_free_packet(packet);
	}

	/*
		Step 5: Clean up.
	*/
	swr_free(&auConvertCtx);

	fclose(pOutFile);

	av_free(outBuffer);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}
