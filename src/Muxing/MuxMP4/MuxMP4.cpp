/*
 * This program mux a video bitstream(H.264) and a audio bitstream(AAC) together
 * into a MP4 format file.
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavformat/avformat.h"
};

int main(int argc, char* argv[])
{
	/*
		Step 1: Init FFmepg 
	*/
	av_register_all();

	/*
		Step 2: Set up input Audio stream and Video stream.
	*/
	AVFormatContext *pInFmtCtxV = NULL; 
	AVFormatContext *pInFmtCtxA = NULL;
	AVFormatContext *pOutFmtCtx = NULL;

	const char *pInFileNameV = "c://work//cuc_ieschool.h264";
	const char *pInFileNameA = "c://work//gowest.aac";
	const char *pOutFileName = "d://cuc_ieschool.mp4"; 

	int ret = 1; 
	if ((ret = avformat_open_input(&pInFmtCtxV, pInFileNameV, 0, 0)) < 0) 
	{
		printf( "Can't open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(pInFmtCtxV, 0)) < 0) 
	{
		printf( "Failed to retrieve input stream information.");
		goto end;
	}

	if ((ret = avformat_open_input(&pInFmtCtxA, pInFileNameA, 0, 0)) < 0) 
	{
		printf( "Can't open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(pInFmtCtxA, 0)) < 0) 
	{
		printf( "Failed to retrieve input stream information.");
		goto end;
	}

	/*
		Step 3: Copy input A/V stream parameter to AVCodecContext of the output.
	*/
	printf("===========Input Information==========\n");
	av_dump_format(pInFmtCtxV, 0, pInFileNameV, 0);
	av_dump_format(pInFmtCtxA, 0, pInFileNameA, 0);
	printf("======================================\n");
	avformat_alloc_output_context2(&pOutFmtCtx, NULL, NULL, pOutFileName);
	if (!pOutFmtCtx) 
	{
		printf( "Can't create output context.\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}

	int videoIndexV = -1, videoIndexOut = -1;
	int audioIndexA = -1, audioIndexOut = -1;

	// Copy input video stream parameter to AVCodecContext of the output.
	for (int i = 0; i < pInFmtCtxV->nb_streams; i++)
	{
		//Create output AVStream according to input AVStream
		if (pInFmtCtxV->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			AVStream *pInStream = pInFmtCtxV->streams[i];
			AVStream *pOutStream = avformat_new_stream(pOutFmtCtx, pInStream->codec->codec);
			videoIndexV = i;
			if (!pOutStream) 
			{
				printf( "Failed allocating output stream\n");
				ret = AVERROR_UNKNOWN;
				goto end;
			}
			videoIndexOut=pOutStream->index;
			//Copy the settings of AVCodecContext
			if (avcodec_copy_context(pOutStream->codec, pInStream->codec) < 0)
			{
				printf( "Failed to copy context from input to output stream codec context.\n");
				goto end;
			}
			pOutStream->codec->codec_tag = 0;
			if (pOutFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
			{
				pOutStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			}
			break;
		}
	}

	// Copy input audio stream parameter to AVCodecContext of the output.
	for (int i = 0; i < pInFmtCtxA->nb_streams; i++) 
	{
		//Create output AVStream according to input AVStream
		if(pInFmtCtxA->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
			AVStream *in_stream = pInFmtCtxA->streams[i];
			AVStream *out_stream = avformat_new_stream(pOutFmtCtx, in_stream->codec->codec);
			audioIndexA=i;
			if (!out_stream) 
			{
				printf( "Failed allocating output stream\n");
				ret = AVERROR_UNKNOWN;
				goto end;
			}
			audioIndexOut=out_stream->index;
			//Copy the settings of AVCodecContext
			if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0)
			{
				printf( "Failed to copy context from input to output stream codec context\n");
				goto end;
			}
			out_stream->codec->codec_tag = 0;
			if (pOutFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
			{
				out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			}
			break;
		}
	}

	printf("==========Output Information==========\n");
	av_dump_format(pOutFmtCtx, 0, pOutFileName, 1);
	printf("======================================\n");

	/*
		Step 4: Write file header.
	*/
	//Open output file
	AVOutputFormat *ofmt = NULL;
	ofmt = pOutFmtCtx->oformat;
	if (!(ofmt->flags & AVFMT_NOFILE))
	{
		if (avio_open(&pOutFmtCtx->pb, pOutFileName, AVIO_FLAG_WRITE) < 0) 
		{
			printf( "Could not open output file '%s'", pOutFileName);
			goto end;
		}
	}
	//Write file header
	if (avformat_write_header(pOutFmtCtx, NULL) < 0) 
	{
		printf( "Error occurred when opening output file\n");
		goto end;
	}

	/*
		Step 5: Get the AVPakcet from A/V stream and write into the output file.
	*/
	AVPacket packet;
	int frame_index = 0;
	int64_t cur_pts_v = 0, cur_pts_a = 0;

	AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb"); 
	AVBitStreamFilterContext* aacbsfc =  av_bitstream_filter_init("aac_adtstoasc"); 

	while (1)
	{
		AVFormatContext *ifmt_ctx;
		int stream_index = 0;
		AVStream *in_stream = NULL, *out_stream = NULL;

		//Get an AVPacket
		if(av_compare_ts(cur_pts_v,pInFmtCtxV->streams[videoIndexV]->time_base,
			cur_pts_a,pInFmtCtxA->streams[audioIndexA]->time_base) <= 0)
		{
			ifmt_ctx=pInFmtCtxV;
			stream_index=videoIndexOut;

			if(av_read_frame(ifmt_ctx, &packet) >= 0)
			{
				do{
					in_stream  = ifmt_ctx->streams[packet.stream_index];
					out_stream = pOutFmtCtx->streams[stream_index];

					if(packet.stream_index == videoIndexV)
					{
						//FIX£ºNo PTS (Example: Raw H.264)
						//Simple Write PTS
						if(packet.pts == AV_NOPTS_VALUE)
						{
							//Write PTS
							AVRational time_base1 = in_stream->time_base;
							//Duration between 2 frames (us)
							int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
							//Parameters
							packet.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
							packet.dts = packet.pts;
							packet.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
							frame_index++;
						}

						cur_pts_v=packet.pts;
						break;
					}
				} while (av_read_frame(ifmt_ctx, &packet) >= 0);
			}
			else
			{
				break;
			}
		}else{
			ifmt_ctx=pInFmtCtxA;
			stream_index=audioIndexOut;
			if(av_read_frame(ifmt_ctx, &packet) >= 0)
			{
				do{
					in_stream  = ifmt_ctx->streams[packet.stream_index];
					out_stream = pOutFmtCtx->streams[stream_index];

					if(packet.stream_index == audioIndexA)
					{
						//FIX£ºNo PTS
						//Simple Write PTS
						if(packet.pts == AV_NOPTS_VALUE)
						{
							//Write PTS
							AVRational time_base1=in_stream->time_base;
							//Duration between 2 frames (us)
							int64_t calc_duration=(double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
							//Parameters
							packet.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
							packet.dts = packet.pts;
							packet.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
							frame_index++;
						}
						cur_pts_a = packet.pts;
						break;
					}
				}while(av_read_frame(ifmt_ctx, &packet) >= 0);
			}
			else
			{
				break;
			}

		}


		av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &packet.data,
									&packet.size, packet.data, packet.size, 0);
		av_bitstream_filter_filter(aacbsfc, out_stream->codec, NULL, &packet.data, 
										&packet.size, packet.data, packet.size, 0);


		//Convert PTS/DTS
		packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, 
									out_stream->time_base,
									(AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base,
									out_stream->time_base, 
									(AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		packet.duration = av_rescale_q(packet.duration, in_stream->time_base, 
									out_stream->time_base);
		packet.pos = -1;
		packet.stream_index=stream_index;

		printf("Write 1 Packet. size:%5d\tpts:%lld\n", packet.size, packet.pts);
		//Write
		if (av_interleaved_write_frame(pOutFmtCtx, &packet) < 0)
		{
			printf( "Error muxing packet\n");
			break;
		}
		av_free_packet(&packet);
	}
	/*
		Step 6: Write file tailer.
	*/
	av_write_trailer(pOutFmtCtx);

	/*
		Step 7: Clean up.
	*/
	av_bitstream_filter_close(h264bsfc);
	av_bitstream_filter_close(aacbsfc);

end:
	avformat_close_input(&pInFmtCtxV);
	avformat_close_input(&pInFmtCtxA);
	
	if (pOutFmtCtx && !(ofmt->flags & AVFMT_NOFILE))
	{
		avio_close(pOutFmtCtx->pb);
	}
	avformat_free_context(pOutFmtCtx);
	
	if (ret < 0 && ret != AVERROR_EOF) 
	{
		printf( "Error occurred.\n");
		return -1;
	}

	return 0;
}
