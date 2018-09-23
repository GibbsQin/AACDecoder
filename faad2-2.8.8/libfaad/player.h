#ifndef NATIVETESTER_H_
#define NATIVETESTER_H_

static const char *nativetester_class_path_name = "com/gibbs/faaddemo/JNI";

jboolean Java_com_gibbs_faaddemo_JNI_isNeon(JNIEnv *env, jobject thiz);
jboolean Java_com_gibbs_faaddemo_JNI_raw2wav(JNIEnv *env, jobject thiz);


static JNINativeMethod nativetester_methods[] = {
		{"isNeon", "()Z", (void*) Java_com_gibbs_faaddemo_JNI_isNeon},
		{"raw2wav", "()Z", (void*) Java_com_gibbs_faaddemo_JNI_raw2wav},
};

#endif /* NATIVETESTER_H_ */