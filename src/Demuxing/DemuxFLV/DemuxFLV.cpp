/*
 * This software split a media file (in Container such as MKV, FLV, AVI...)
 * to video and audio bitstream.
 * In this example, it demux a MPEG2TS file to H.264 bitstream
 * and AAC bitstream.
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavformat/avformat.h"
};
/*
	FIX: H.264 in some container format (FLV, MP4, MKV etc.) need "h264_mp4toannexb" 
	bitstream filter (BSF). 
		Add SPS,PPS in front of IDR frame
		Add start code ("0,0,0,1") in front of NALU
	H.264 in some container (MPEG2TS) don't need this BSF.
*/

// '1': Use H.264 Bitstream Filter 
#define USE_H264BSF 1

int main(int argc, char* argv[])
{
	/*
		Step 1: FFmpeg init. 
	*/
	av_register_all();

	/*
		Step 2: Set up A/V input and output content and format.
	*/
	AVOutputFormat *pOutFmtA = NULL, *pOoutFmtV = NULL;
	AVFormatContext *pInFmtCtx = NULL, *pOutFmtCtxA = NULL, *pOutFmtCtxV = NULL;

	const char *pInFile  = "c://work//cuc_ieschool.flv";
	const char *pOutFileV = "c://work//cuc_ieschool.h264";
	const char *pOutFileA = "c://work//cuc_ieschool.aac";


	// Input
	int ret = 0; 
	if ((ret = avformat_open_input(&pInFmtCtx, pInFile, 0, 0)) < 0) 
	{
		printf( "Can't open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(pInFmtCtx, 0)) < 0)
	{
		printf( "Failed to retrieve input stream information.");
		goto end;
	}

	// Output
	avformat_alloc_output_context2(&pOutFmtCtxV, NULL, NULL, pOutFileV);
	if (!pOutFmtCtxV) 
	{
		printf( "Can't create output context.\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	pOoutFmtV = pOutFmtCtxV->oformat;

	avformat_alloc_output_context2(&pOutFmtCtxA, NULL, NULL, pOutFileA);
	if (!pOutFmtCtxA) 
	{
		printf( "Could not create output context.\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	pOutFmtA = pOutFmtCtxA->oformat;

	/*
		Step 3: Create output A/V stream.
	*/
	int videoIndex = -1, audioIndex = -1;
	for (int i = 0; i < pInFmtCtx->nb_streams; i++) 
	{
		// Create output AVStream according to input AVStream.
		AVFormatContext *ofmt_ctx = NULL;
		AVStream *in_stream = pInFmtCtx->streams[i];
		AVStream *out_stream = NULL;
			
		if (pInFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoIndex = i;
			out_stream = avformat_new_stream(pOutFmtCtxV, in_stream->codec->codec);
			ofmt_ctx = pOutFmtCtxV;
		}
		else if (pInFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioIndex = i;
			out_stream = avformat_new_stream(pOutFmtCtxA, in_stream->codec->codec);
			ofmt_ctx = pOutFmtCtxA;
		}
		else
		{
			break;
		}
			
		if (!out_stream) 
		{
			printf( "Failed allocating output stream.\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		// Copy the settings of AVCodecContext.
		if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0)
		{
			printf( "Failed to copy context from input to output stream codec context.\n");
			goto end;
		}
		out_stream->codec->codec_tag = 0;

		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		{
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	// Dump Format------------------
	printf("\n============== Input Video =============\n");
	av_dump_format(pInFmtCtx, 0, pInFile, 0);
	printf("\n============== Output Video ============\n");
	av_dump_format(pOutFmtCtxV, 0, pOutFileV, 1);
	printf("\n============== Output Audio ============\n");
	av_dump_format(pOutFmtCtxA, 0, pOutFileA, 1);
	printf("\n======================================\n");

	/*
		Step 4: Open output file.
	*/
	if (!(pOoutFmtV->flags & AVFMT_NOFILE)) 
	{
		if (avio_open(&pOutFmtCtxV->pb, pOutFileV, AVIO_FLAG_WRITE) < 0) 
		{
			printf( "Can't open output file '%s'. ", pOutFileV);
			goto end;
		}
	}

	if (!(pOutFmtA->flags & AVFMT_NOFILE)) 
	{
		if (avio_open(&pOutFmtCtxA->pb, pOutFileA, AVIO_FLAG_WRITE) < 0) 
		{
			printf( "Can't open output file '%s'. ", pOutFileA);
			goto end;
		}
	}

	/*
		Step 5: Write file header to output file.
	*/
	if (avformat_write_header(pOutFmtCtxV, NULL) < 0) 
	{
		printf( "Error occurred when opening video output file. \n");
		goto end;
	}
	if (avformat_write_header(pOutFmtCtxA, NULL) < 0) 
	{
		printf( "Error occurred when opening audio output file. \n");
		goto end;
	}
	
	/*
		Step 5: Write packets to output file
	*/
#if USE_H264BSF
	AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb"); 
#endif

	AVPacket pkt; 
	int frameIndex = 0;
	while (1)
	{
		AVFormatContext *ofmt_ctx;
		AVStream *in_stream, *out_stream;
		// Get an AVPacket
		if (av_read_frame(pInFmtCtx, &pkt) < 0)
		{
			break;
		}

		in_stream  = pInFmtCtx->streams[pkt.stream_index];

		if (pkt.stream_index == videoIndex)
		{
			out_stream = pOutFmtCtxV->streams[0];
			ofmt_ctx=pOutFmtCtxV;
			printf("Write Video Packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts);
#if USE_H264BSF
			av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
		}
		else if (pkt.stream_index == audioIndex)
		{
			out_stream = pOutFmtCtxA->streams[0];
			ofmt_ctx = pOutFmtCtxA;
			printf("Write Audio Packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts);
		}
		else
		{
			continue;
		}

		// Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
									(AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, 
									(AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index = 0;
		// Write
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) 
		{
			printf( "Error muxing packet\n");
			break;
		}
		// printf("Write %8d frames to output file\n",frame_index);
		av_free_packet(&pkt);
		frameIndex++;
	}

#if USE_H264BSF
	av_bitstream_filter_close(h264bsfc);  
#endif
	/*
		Step 6: Write file trailer
	*/
	//Write file trailer
	av_write_trailer(pOutFmtCtxA);
	av_write_trailer(pOutFmtCtxV);

	/*
		Step 7: Clean up.
	*/
end:
	avformat_close_input(&pInFmtCtx);
	/* close output */
	if (pOutFmtCtxA && !(pOutFmtA->flags & AVFMT_NOFILE))
	{
		avio_close(pOutFmtCtxA->pb);
	}

	if (pOutFmtCtxV && !(pOoutFmtV->flags & AVFMT_NOFILE))
	{
		avio_close(pOutFmtCtxV->pb);
	}

	avformat_free_context(pOutFmtCtxA);
	avformat_free_context(pOutFmtCtxV);


	if (ret < 0 && ret != AVERROR_EOF) 
	{
		printf( "Error occurred.\n");
		return -1;
	}

	return 0;
}


