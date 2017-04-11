/**
 * This program encode YUV420P data to H.264 bit stream.
 * It's the simplest video encoding software based on FFmpeg. 
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};

int FlushEncoder(AVFormatContext *fmt_ctx, unsigned int stream_index)
{
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & CODEC_CAP_DELAY))
	{
		return 0; 
	}

	while (1) 
	{
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
		{
			break; 
		}

		if (!got_frame)
		{
			ret=0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",enc_pkt.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &enc_pkt);
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
		Step 1: 
	*/
	av_register_all();
	AVFormatContext* pFormatCtx = NULL; 
	pFormatCtx = avformat_alloc_context();
	int inW = 640, inH= 480;     

	/*
		Step 2:
	*/
	// Get the output format.
	AVOutputFormat* pOutFormat = NULL; 
	const char* out_file = "ds.h264";
	pOutFormat = av_guess_format(NULL, out_file, NULL);
	pFormatCtx->oformat = pOutFormat;

	// Create and initialize a AVIOContext for accessing the resource indicated by url.
	if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0)
	{
		printf("Failed to open output file! \n");
		return -1;
	}

	// Add a new stream to a media file.
	AVStream* videoStream = NULL; 
	videoStream = avformat_new_stream(pFormatCtx, 0); 
	if (videoStream == NULL)
	{
		return -1;
	}

	//Param that must set
	AVCodecContext* pCodecCtx = NULL; 
	pCodecCtx = videoStream->codec;
	pCodecCtx->codec_id = pOutFormat->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = inW;  
	pCodecCtx->height = inH;
	pCodecCtx->bit_rate = 400000;  
	pCodecCtx->gop_size = 250;

	pCodecCtx->time_base.num = 1;  
	pCodecCtx->time_base.den = 25;  

	// H264
	pCodecCtx->qmin = 10;
	pCodecCtx->qmax = 51;

	// Optional Param
	pCodecCtx->max_b_frames=3;

	// Set Option
	AVDictionary *param = NULL;
	// H.264
	if (pCodecCtx->codec_id == AV_CODEC_ID_H264)
	{
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
	}

	// Print detailed information about the input or output format
	av_dump_format(pFormatCtx, 0, out_file, 1);

	/*
		Step 3:
	*/
	// Find a registered encoder with a matching codec ID.
	AVCodec* pCodec = NULL; 
	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec)
	{
		printf("Can't find encoder! \n");
		return -1;
	}

	if (avcodec_open2(pCodecCtx, pCodec, &param) < 0)
	{
		printf("Failed to open encoder! \n");
		return -1;
	}

	AVFrame* pFrame = NULL; 
	pFrame = av_frame_alloc();

	int pictureSize = 0;
	// Return the size in bytes of the amount of data required to store an image.
	pictureSize = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	uint8_t* pictureBuf = NULL; 
	pictureBuf = (uint8_t *)av_malloc(pictureSize);
	// Setup the data pointers and linesizes
	avpicture_fill((AVPicture *)pFrame, pictureBuf, pCodecCtx->pix_fmt, 
					pCodecCtx->width, pCodecCtx->height);

	/*
		Step 4:
	*/
	// Write the stream header to an output media file.
	avformat_write_header(pFormatCtx, NULL);

	AVPacket pPacket; 
	av_new_packet(&pPacket, pictureSize);
	int ySize = 0; 
	ySize = pCodecCtx->width * pCodecCtx->height;

	int framNum = 100, frameCnt = 0; 
	FILE *pInFile = fopen("output.yuv", "rb");   
	for (int i = 0; i < framNum; i++)
	{
		//Read raw YUV data
		if (fread(pictureBuf, 1, ySize * 3 / 2, pInFile) <= 0)
		{
			printf("Failed to read raw data! \n");
			return -1;
		}
		else if (feof(pInFile))
		{
			break;
		}
		pFrame->data[0] = pictureBuf;                 // Y
		pFrame->data[1] = pictureBuf+ ySize;          // U 
		pFrame->data[2] = pictureBuf+ ySize * 5 / 4;  // V
		pFrame->pts = i * (videoStream->time_base.den) / ((videoStream->time_base.num) * 25);
		int gotPicture = 0;
		// Encode a frame of video
		int ret = avcodec_encode_video2(pCodecCtx, &pPacket, pFrame, &gotPicture);
		if (ret < 0)
		{
			printf("Failed to encode! \n");
			return -1;
		}

		if (gotPicture == 1)
		{
			printf("Succeed to encode frame: %5d\tsize:%5d\n", frameCnt, pPacket.size);
			frameCnt++;
			pPacket.stream_index = videoStream->index;
			// Write a packet to an output media file.
			ret = av_write_frame(pFormatCtx, &pPacket);
			av_free_packet(&pPacket);
		}
	}

	//Flush Encoder.
	int ret = FlushEncoder(pFormatCtx, 0);
	if (ret < 0) 
	{
		printf("Flushing encoder failed\n");
		return -1;
	}

	// Write the stream trailer to an output media file.
	av_write_trailer(pFormatCtx);

	/*
		Step 5:
	*/
	// Clean up.
	if (videoStream)
	{
		avcodec_close(videoStream->codec);
		av_free(pFrame);
		av_free(pictureBuf);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	fclose(pInFile);

	return 0;
}