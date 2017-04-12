/*
 * This program encode PCM data to AAC bitstream.
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};

int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index)
{
	int ret;
	int gotFrame;
	AVPacket encPkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
		CODEC_CAP_DELAY))
	{
		return 0;
	}

	while (1) 
	{
		encPkt.data = NULL;
		encPkt.size = 0;
		av_init_packet(&encPkt);
		ret = avcodec_encode_audio2 (fmt_ctx->streams[stream_index]->codec, &encPkt,
			NULL, &gotFrame);
		av_frame_free(NULL);
		if (ret < 0)
		{
			break;
		}
		if (!gotFrame)
		{
			ret=0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",encPkt.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &encPkt);
		if (ret < 0)
		{
			break;
		}
	}

	return ret;
}

int main(int argc, char* argv[])
{
	/*
		Step 1: FFmpeg Init.
	*/
	av_register_all();

	/*
		Step 2: Set up input audio stream.
	*/
	FILE *pInFile = NULL;
	pInFile= fopen("c://work//tdjm.pcm", "rb");
	const char* pOutFile = "c://work//tdjm.aac";

	// Method 1.
	AVFormatContext* pFormatCtx = NULL;
	pFormatCtx = avformat_alloc_context();
	AVOutputFormat* pOutputFmt = NULL;
	pOutputFmt = av_guess_format(NULL, pOutFile, NULL);
	pFormatCtx->oformat = pOutputFmt;

	// Method 2.
	// avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, pOutFile);
	// fmt = pFormatCtx->oformat;

	if (avio_open(&pFormatCtx->pb, pOutFile, AVIO_FLAG_READ_WRITE) < 0)
	{
		printf("Failed to open output file!\n");
		return -1;
	}

	AVStream* pAStream = NULL;
	pAStream = avformat_new_stream(pFormatCtx, 0);
	if (pAStream == NULL)
	{
		printf("Filed to add a new stream!\n"); 
		return -1;
	}

	/*
		Step 3: Setup codec 
	*/
	
	AVCodecContext* pCodecCtx = NULL;
	pCodecCtx = pAStream->codec;
	pCodecCtx->codec_id = pOutputFmt->audio_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
	pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
	pCodecCtx->sample_rate = 44100;
	pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
	pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
	pCodecCtx->bit_rate = 64000;  

	// Show some information
	av_dump_format(pFormatCtx, 0, pOutFile, 1);

	AVCodec* pCodec = NULL;
	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec)
	{
		printf("Can not find encoder!\n");
		return -1;
	}

	if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0)
	{
		printf("Failed to open encoder!\n");
		return -1;
	}

	/*
		Step 4: Encode pcm to aac
	*/
	AVFrame* pFrame = NULL;
	pFrame = av_frame_alloc();
	pFrame->nb_samples = pCodecCtx->frame_size;
	pFrame->format = pCodecCtx->sample_fmt;

	int gotFrame = 0;

	int size = 0;
	size = av_samples_get_buffer_size(NULL, pCodecCtx->channels, pCodecCtx->frame_size,
										pCodecCtx->sample_fmt, 1);

	uint8_t* framBuf = NULL;
	framBuf = (uint8_t *)av_malloc(size);
	avcodec_fill_audio_frame(pFrame, pCodecCtx->channels, pCodecCtx->sample_fmt,
							(const uint8_t*)framBuf, size, 1);
	
	avformat_write_header(pFormatCtx, NULL);
	
	AVPacket packet;
	av_new_packet(&packet, size);

	int framNum = 1000;   
	int ret = 0;
	for (int i = 0; i < framNum; i++)
	{
		// Read PCM
		if (fread(framBuf, 1, size, pInFile) <= 0)
		{
			printf("Failed to read raw data! \n");
			return -1;
		}
		else if(feof(pInFile))
		{
			break;
		}
		pFrame->data[0] = framBuf;
		pFrame->pts = i * 100;
		gotFrame=0;
		// Encode
		ret = avcodec_encode_audio2(pCodecCtx, &packet, pFrame, &gotFrame);
		if(ret < 0)
		{
			printf("Failed to encode!\n");
			return -1;
		}
		if (gotFrame == 1)
		{
			printf("Succeed to encode 1 frame! \tsize:%5d\n", packet.size);
			packet.stream_index = pAStream->index;
			ret = av_write_frame(pFormatCtx, &packet);
			av_free_packet(&packet);
		}
	}
	
	ret = flush_encoder(pFormatCtx, 0);
	if (ret < 0)
	{
		printf("Flushing encoder failed\n");
		return -1;
	}

	av_write_trailer(pFormatCtx);

	/*
		Step 5: Clean up.
	*/
	if (pAStream)
	{
		avcodec_close(pAStream->codec);
		av_free(pFrame);
		av_free(framBuf);
	}

	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	fclose(pInFile);

	return 0;
}