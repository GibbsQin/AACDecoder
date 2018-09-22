/*android specific headers*/
#include <jni.h>
#include <android/log.h>

#include <stdlib.h>

#include "player.h"


#define LOG_TAG "player"
#define LOG_LEVEL 10
#define LOGI(level, ...) if (level <= LOG_LEVEL) {__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);}
#define LOGE(level, ...) if (level <= LOG_LEVEL) {__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);}

jboolean Java_com_gibbs_faaddemo_JNI_isNeon(JNIEnv *env, jobject thiz) {
	uint64_t features;

	LOGE(1, "jni test!");

	return 1;
}

