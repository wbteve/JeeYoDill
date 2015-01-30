#include <jni.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>

#ifndef _Included_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper
#define _Included_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper
#ifdef __cplusplus
extern "C" {
#endif


#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

void printGLString(const char *name, GLenum s);
void checkGlError(const char* op);

JNIEXPORT void JNICALL Java_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper_add(JNIEnv*, jclass, jint v);

/*
 * Class:     com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper
 * Method:    on_surface_created
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper_on_1surface_1created
  (JNIEnv *, jclass);

/*
 * Class:     com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper
 * Method:    on_surface_changed
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper_on_1surface_1changed
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper
 * Method:    on_draw_frame
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper_on_1draw_1frame
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif
