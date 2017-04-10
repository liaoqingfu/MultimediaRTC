#include "DeviceInfo.h"

#define __STDC_CONSTANT_MACROS  
extern "C"
{
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
};

void ShowDShowDevice()
{
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary* pOptions = NULL;
	av_dict_set(&pOptions,"list_devices","true",0);
	AVInputFormat *pInputFormat = av_find_input_format("dshow");
	printf("========Device Info=============\n");
	avformat_open_input(&pFormatCtx,"video=dummy",pInputFormat,&pOptions);
	printf("================================\n");

	avformat_close_input(&pFormatCtx); 
}

void ShowDShowDeviceOption()
{
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary* pOptions = NULL;
	av_dict_set(&pOptions,"list_options","true",0);
	AVInputFormat *pInputFormat = av_find_input_format("dshow");
	printf("========Device Option Info======\n");
	avformat_open_input(&pFormatCtx,"video=USB Camera",pInputFormat,&pOptions);
	printf("================================\n");
	
	avformat_close_input(&pFormatCtx); 
}
