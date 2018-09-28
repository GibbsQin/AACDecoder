#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <android/log.h>
#include "faad.h"

#define LOG_TAG "door_bell_decode_aac"
#define LOG_LEVEL 10
#define LOGI(level, ...) if (level <= LOG_LEVEL) {__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);}
#define LOGE(level, ...) if (level <= LOG_LEVEL) {__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);}

NeAACDecHandle aac_decode_init()
{
	NeAACDecHandle hDecoder = NeAACDecOpen();
	
	NeAACDecConfigurationPtr pDecodeConfig = NeAACDecGetCurrentConfiguration(hDecoder);
	
	pDecodeConfig->defSampleRate	= 8000;
	pDecodeConfig->defObjectType	= LC;
	pDecodeConfig->outputFormat 	= FAAD_FMT_16BIT;
	pDecodeConfig->downMatrix		= 0;
	pDecodeConfig->useOldADTSFormat = 0;

	NeAACDecSetConfiguration(hDecoder, pDecodeConfig);

	LOGE(1, "aac_decode_init.");

    return hDecoder;
}

int aac_decode(NeAACDecHandle hDecoder, short *pInputBuf, uint32_t dwInputSize, short* pOutputBuf, uint32_t* dwOutputSize)
{
	long lRealUse =0;

	unsigned long lRealSampleRate	;
	unsigned char ucRealChans		;

	if ((lRealUse = NeAACDecInit(hDecoder, (const unsigned char*)pInputBuf,
			dwInputSize, &lRealSampleRate, &ucRealChans)) < 0)
	{
		/* If some error initializing occured, skip the file */
		LOGE(1, "Error initializing decoder library.\n");
		NeAACDecClose(hDecoder);
		return 1;
	}
			
	dwInputSize -= lRealUse;

	NeAACDecFrameInfo frameInfo;

	do
	{
		LOGE(1, "start decode aac.");
		pOutputBuf = (short*)NeAACDecDecode(hDecoder, &frameInfo, (const unsigned char*)pInputBuf, dwInputSize);
		dwInputSize -= frameInfo.bytesconsumed;
	} while (dwInputSize <= 0);

	LOGE(1, "Decode Raw Aac Success!\n");
	getchar();
	return 0;
}


