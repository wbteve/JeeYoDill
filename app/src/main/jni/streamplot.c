#include <jni.h>
#include <string.h>
#include <android/log.h>

#define DEBUG_TAG "NDKSetupActivity"

JNIEXPORT void JNICALL
Java_com_jeeyo_sagar_jeeyodill_MainActivity_printLogHelloString(JNIEnv *env, jobject this) {
    __android_log_print(ANDROID_LOG_ERROR, DEBUG_TAG, "NDK: %s", "Hi logcat from native!");
}

JNIEXPORT jstring JNICALL
Java_com_jeeyo_sagar_jeeyodill_MainActivity_getHelloString(JNIEnv *env, jobject this) {
    return (*env)->NewStringUTF(env, "Hello world from NDK in C!");
}