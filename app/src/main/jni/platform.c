#include "platform.h"
#include "streamplot.h"

JNIEXPORT void JNICALL
Java_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper_on_1surface_1created(JNIEnv *env, jclass this) {

}


JNIEXPORT void JNICALL
Java_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper_on_1surface_1changed(JNIEnv *env, jclass this, jint w, jint h) {
    setupGraphics(w, h);
}

JNIEXPORT void JNICALL
Java_com_jeeyo_sagar_jeeyodill_StreamplotJNIWrapper_on_1draw_1frame(JNIEnv *env, jclass this) {
    renderFrame();
}



void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

void checkGlError(const char* op) {
    GLint error;
    for (error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}