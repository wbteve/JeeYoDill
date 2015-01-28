#include "streamplot_jni.h"
#include "streamplot.h"

JNIEXPORT void JNICALL
Java_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper_on_1surface_1created(JNIEnv *env, jclass this) {
    on_surface_created();
}


JNIEXPORT void JNICALL
Java_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper_on_1surface_1changed(JNIEnv *env, jclass this, jint w, jint h) {
    on_surface_changed();
}

JNIEXPORT void JNICALL
Java_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper_on_1draw_1frame(JNIEnv *env, jclass this) {
    on_draw_frame();
}