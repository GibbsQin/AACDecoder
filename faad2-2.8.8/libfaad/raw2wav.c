/*******************************************************************************
Copyright (c) wubihe Tech. Co., Ltd. All rights reserved.
--------------------------------------------------------------------------------
Date Created:	2014-10-25
Author:			wubihe QQ:1269122125 Email:1269122125@qq.com
Description:	用faad库解码原始AAC音频流，解码结果保存wav
--------------------------------------------------------------------------------
********************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <android/log.h>
#include "faad.h"

//待解码文件
#define  DECODE_FILE_NAME	("/storage/emulated/0/jnifile/huangdun_uadts.aac")
//解码后WAV文件
#define  DECODE_OUTPUT_FILE	("/storage/emulated/0/jnifile/huangdun.wav")
#define  MAX_CHANNELS		(8)
#define  MAXWAVESIZE        (4294967040LU)

#define LOG_TAG "raw2wav"
#define LOG_LEVEL 10
#define LOGI(level, ...) if (level <= LOG_LEVEL) {__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);}
#define LOGE(level, ...) if (level <= LOG_LEVEL) {__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);}

#define  min(a,b)            (((a) < (b)) ? (a) : (b))
typedef struct 
{
	//当前缓存总数据量
	long bytes_into_buffer;
	//当前缓存已经消耗数据量
	long bytes_consumed;
	//整个文件数据使用量
	long file_offset;
	//缓存
	unsigned char *buffer;
	//文件结束标志
	int  at_eof;
	//文件操作句柄
	FILE *infile;
} aac_buffer;

//aac数据缓存  
aac_buffer g_AacBuffer; 

static int fill_buffer(aac_buffer *b)
{
	int bread;
	//解析消耗数据
	if (b->bytes_consumed > 0)
	{
		//有剩余数据 向前面移动
		if (b->bytes_into_buffer)
		{
			memmove((void*)b->buffer, (void*)(b->buffer + b->bytes_consumed),
				b->bytes_into_buffer*sizeof(unsigned char));
		}

		if (!b->at_eof)
		{
			bread = fread((void*)(b->buffer + b->bytes_into_buffer), 1,
				b->bytes_consumed, b->infile);

			if (bread != b->bytes_consumed)
				b->at_eof = 1;

			b->bytes_into_buffer += bread;
		}

		b->bytes_consumed = 0;

		if (b->bytes_into_buffer > 3)
		{
			if (memcmp(b->buffer, "TAG", 3) == 0)
				b->bytes_into_buffer = 0;
		}

		if (b->bytes_into_buffer > 11)
		{
			if (memcmp(b->buffer, "LYRICSBEGIN", 11) == 0)
				b->bytes_into_buffer = 0;
		}

		if (b->bytes_into_buffer > 8)
		{
			if (memcmp(b->buffer, "APETAGEX", 8) == 0)
				b->bytes_into_buffer = 0;
		}
	}

	return 1;
}

static void advance_buffer(aac_buffer *b, int bytes)
{
	b->file_offset += bytes;
	b->bytes_consumed = bytes;
	b->bytes_into_buffer -= bytes;
	if (b->bytes_into_buffer < 0)
		b->bytes_into_buffer = 0;
}

//写WAV文件头 
static int write_wav_header(int iBitPerSample,int iChans,unsigned char ucFormat,int iSampleRate,int iTotalSamples,FILE*pFile)
{
	unsigned char header[44];
	unsigned char* p = header;
	unsigned int bytes = (iBitPerSample + 7) / 8;
	float data_size = (float)bytes * iTotalSamples;
	unsigned long word32;

	*p++ = 'R'; *p++ = 'I'; *p++ = 'F'; *p++ = 'F';

	word32 = (data_size + (44 - 8) < (float)MAXWAVESIZE) ?
		(unsigned long)data_size + (44 - 8)  :  (unsigned long)MAXWAVESIZE;
	*p++ = (unsigned char)(word32 >>  0);
	*p++ = (unsigned char)(word32 >>  8);
	*p++ = (unsigned char)(word32 >> 16);
	*p++ = (unsigned char)(word32 >> 24);

	*p++ = 'W'; *p++ = 'A'; *p++ = 'V'; *p++ = 'E';
	*p++ = 'f'; *p++ = 'm'; *p++ = 't'; *p++ = ' ';
	*p++ = 0x10; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;

	if (ucFormat == FAAD_FMT_FLOAT)
	{
		*p++ = 0x03; *p++ = 0x00;
	} else {
		*p++ = 0x01; *p++ = 0x00;
	}

	*p++ = (unsigned char)(iChans >> 0);
	*p++ = (unsigned char)(iChans >> 8);

	word32 = (unsigned long)(iSampleRate + 0.5);

	*p++ = (unsigned char)(word32 >>  0);
	*p++ = (unsigned char)(word32 >>  8);
	*p++ = (unsigned char)(word32 >> 16);
	*p++ = (unsigned char)(word32 >> 24);

	word32 = iSampleRate * bytes * iChans;

	*p++ = (unsigned char)(word32 >>  0);
	*p++ = (unsigned char)(word32 >>  8);
	*p++ = (unsigned char)(word32 >> 16);
	*p++ = (unsigned char)(word32 >> 24);

	word32 = bytes * iChans;

	*p++ = (unsigned char)(word32 >>  0);
	*p++ = (unsigned char)(word32 >>  8);
	*p++ = (unsigned char)(iBitPerSample >> 0);
	*p++ = (unsigned char)(iBitPerSample >> 8);

	*p++ = 'd'; *p++ = 'a'; *p++ = 't'; *p++ = 'a';

	word32 = data_size < MAXWAVESIZE ?
		(unsigned long)data_size : (unsigned long)MAXWAVESIZE;

	*p++ = (unsigned char)(word32 >>  0);
	*p++ = (unsigned char)(word32 >>  8);
	*p++ = (unsigned char)(word32 >> 16);
	*p++ = (unsigned char)(word32 >> 24);

	return fwrite(header, sizeof(header), 1, pFile);

}

static int write_audio_16bit( void *sample_buffer,unsigned int samples,FILE *pFile)
{
	int ret;
	unsigned int i;
	short *sample_buffer16 = (short*)sample_buffer;
	char *data = (char*)malloc(samples*16*sizeof(char)/8);

	for (i = 0; i < samples; i++)
	{
		data[i*2] = (char)(sample_buffer16[i] & 0xFF);
		data[i*2+1] = (char)((sample_buffer16[i] >> 8) & 0xFF);
	}

	ret = fwrite(data, samples, 16/8, pFile);

	if (data) free(data);

	return ret;
}

int write_audio_file(void *sample_buffer, int samples,int outputFormat,FILE *pFile)
{
	char *buf = (char *)sample_buffer;
	switch (outputFormat)
	{
	case FAAD_FMT_16BIT:
		return write_audio_16bit(buf,samples,pFile);
	case FAAD_FMT_24BIT:
	case FAAD_FMT_32BIT:
	case FAAD_FMT_FLOAT:
	default:
		return 0;
	}

	return 0;
}

int raw2wav()
{
	memset(&g_AacBuffer, 0, sizeof(aac_buffer));
	g_AacBuffer.infile = fopen(DECODE_FILE_NAME, "rb");
	if (g_AacBuffer.infile == NULL)
	{
		/* unable to open file */
		LOGE(1, "Error opening file: %s\n", DECODE_FILE_NAME);
		return 1;
	}

	fseek(g_AacBuffer.infile, 0, SEEK_END);
	double dTotalFileSize = ftell(g_AacBuffer.infile);
	fseek(g_AacBuffer.infile, 0, SEEK_SET);

	if (!(g_AacBuffer.buffer = (unsigned char*)malloc(FAAD_MIN_STREAMSIZE*MAX_CHANNELS)))
	{
		LOGE(1, "Memory allocation error\n");
		return 0;
	}

	memset(g_AacBuffer.buffer, 0, FAAD_MIN_STREAMSIZE*MAX_CHANNELS);

	size_t sRealRead = fread(g_AacBuffer.buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, g_AacBuffer.infile);
	g_AacBuffer.bytes_into_buffer = sRealRead;
	g_AacBuffer.bytes_consumed = 0;
	g_AacBuffer.file_offset	   = 0;

	if (sRealRead != FAAD_MIN_STREAMSIZE*MAX_CHANNELS)
	{
		g_AacBuffer.at_eof = 1;
	}

	FILE *pOutputFile = fopen(DECODE_OUTPUT_FILE, "wb");
	if(pOutputFile == NULL)
	{
		LOGE(1, "Error opening file: %s\n", DECODE_OUTPUT_FILE);
		return 1;
	}

	//打开解码器
	NeAACDecHandle hDecoder = NeAACDecOpen();

	//设置解码器音频参数 该设置对于原始AAC数据是必须的 否则解码器不知道音频封装信息
	//对于ADTS封装的AAC 不需要设置 解码器可以从视频数据中获取
	NeAACDecConfigurationPtr pDecodeConfig = NeAACDecGetCurrentConfiguration(hDecoder);

	pDecodeConfig->defSampleRate	= 44100;
	pDecodeConfig->defObjectType	= LC;
	pDecodeConfig->outputFormat		= FAAD_FMT_16BIT;
	pDecodeConfig->downMatrix		= 0;
	pDecodeConfig->useOldADTSFormat = 0;

	NeAACDecSetConfiguration(hDecoder, pDecodeConfig);

	long lRealUse =0;

	unsigned long lRealSampleRate	;
	unsigned char ucRealChans		;
	unsigned char ucRealFormat = FAAD_FMT_16BIT;
	//数据初始化
	if ((lRealUse = NeAACDecInit(hDecoder, g_AacBuffer.buffer,
			g_AacBuffer.bytes_into_buffer, &lRealSampleRate, &ucRealChans)) < 0)
	{
		/* If some error initializing occured, skip the file */
		LOGE(1, "Error initializing decoder library.\n");
		if (g_AacBuffer.buffer)
		{
			free(g_AacBuffer.buffer);
		}
		NeAACDecClose(hDecoder);
		fclose(g_AacBuffer.infile);
		return 1;
	}

	//抛弃已经使用过的数据
	advance_buffer(&g_AacBuffer, lRealUse);
	//空的缓存填充新的数据
	fill_buffer(&g_AacBuffer);

	NeAACDecFrameInfo frameInfo;
	void *pSampleBuffer = NULL;
	int bFirstTime		= 1;

	int iOldPercent		= 0;
	do
	{
		pSampleBuffer = NeAACDecDecode(hDecoder, &frameInfo,
			g_AacBuffer.buffer, g_AacBuffer.bytes_into_buffer);

		//抛弃解码消耗的缓存
		advance_buffer(&g_AacBuffer, frameInfo.bytesconsumed);

		if (frameInfo.error > 0)
		{
			LOGE(1, "Error: %s\n",NeAACDecGetErrorMessage(frameInfo.error));
		}

		/* open the sound file now that the number of channels are known */
		if (bFirstTime && !frameInfo.error)
		{
			//写WAV头 这里WAV头文件长度值赋值是0但是不影响播放
			write_wav_header(16,frameInfo.channels,ucRealFormat,frameInfo.samplerate,0,pOutputFile);
			bFirstTime = 0;
		}

		int iPercent = min((int)((g_AacBuffer.file_offset*100))/dTotalFileSize, 100);
		if (iPercent > iOldPercent)
		{
			iOldPercent = iPercent;
			LOGE(1, "%d%% decoding %s.\n", iOldPercent, DECODE_FILE_NAME);
		}

		if ((frameInfo.error == 0) && (frameInfo.samples > 0))
		{
			if ( write_audio_file(pSampleBuffer, frameInfo.samples,ucRealFormat,pOutputFile) == 0)
				break;
		}

		/* fill buffer */
		fill_buffer(&g_AacBuffer);

		if (g_AacBuffer.bytes_into_buffer == 0)
		{
			pSampleBuffer = NULL; /* to make sure it stops now */
		}
	} while (pSampleBuffer != NULL);

	NeAACDecClose(hDecoder);

	fclose(pOutputFile);
	fclose(g_AacBuffer.infile);

	if (g_AacBuffer.buffer)
	{
		free(g_AacBuffer.buffer);
	}

	LOGE(1, "Decode Raw Aac Success!\n");
	getchar();
	return 0;
}

