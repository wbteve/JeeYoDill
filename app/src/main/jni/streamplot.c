#include "streamplot.h"
#include "platform.h"

#include <jni.h>
#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define Nmax 10000
#define EPS 1e-6
#define SCALE_HI_THRESH 0.8
#define SCALE_LO_THRESH 0.2

int N = 1600; // Number of data points
int ptr = 0; // pointer to line
float lastYval = 0;

GLfloat gLineEnds[2*(2*Nmax)]; // 2*N lines, and 2*(2*N) line endings
GLfloat gMVPMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 5.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};
float scaleY = 1.0f;

static const char gVertexShader[] =
    "uniform mat4 u_MVPMatrix;\n"
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = u_MVPMatrix * vPosition;\n"
    "}\n";

static const char gFragmentShader[] =
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(1.0, 0.26, 0.26, 1.0);\n"
    "}\n";

GLuint gProgram;
GLuint gLineHandle;
GLuint gMVPHandle;

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");

        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");

        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

void setupGraphics(int w, int h) {
    LOGI("setupGraphics(%d, %d)", w, h);

    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return;
    }

    gLineHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");

    gMVPHandle = glGetUniformLocation(gProgram, "u_MVPMatrix");
    checkGlError("glGetUniformLocation");

    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
            gLineHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");

    int i;
    for(i = 0;i < N;i++) {
        gLineEnds[4*i] = -1.0f + (i * 2.0f) / N;
        gLineEnds[4*i + 2] = -1.0f + ((i+1) * 2.0f) / N;
    }
}



void addDataPoint(float val) {
    //LOGI("new-val: %d", val);
    int max = 2*(2*N);

    // ptr is pointing to t0 in [(t0, y0), (t1, y1)]
    gLineEnds[ptr+1] = lastYval;
    gLineEnds[ptr+3] = val;

    lastYval = val;
    ptr = ptr + 4;
    if(ptr >= max) {
        ptr = ptr - max;
    }
}

void setYScale() {
    int i;
    float maxVal = -INFINITY, minVal = INFINITY, val;
    for(i = 0; i < N; i++) {
        val = gLineEnds[4*i + 1];
        if(val > maxVal)
            maxVal = val;
        if(val < minVal)
            minVal = val;
    }
    maxVal = abs(maxVal);
    minVal = abs(minVal);

    float tempScaleYPlus = scaleY, tempScaleYMinus = scaleY;
    if(((maxVal * scaleY) > SCALE_HI_THRESH) || ((maxVal * scaleY) < SCALE_LO_THRESH)) {
        tempScaleYPlus = 0.5f/(maxVal + EPS);
    }
    if(((minVal * scaleY) > SCALE_HI_THRESH) || ((minVal * scaleY) < SCALE_LO_THRESH)) {
        tempScaleYMinus = 0.5f/(minVal + EPS);
    }
    scaleY = (tempScaleYPlus < tempScaleYMinus) ? tempScaleYPlus : tempScaleYMinus;
    gMVPMatrix[5] = scaleY;
}

void renderFrame() {
    setYScale();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    checkGlError("glClearColor");

    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glLineWidth(5.0f);
    checkGlError("glLineWidth");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gLineHandle, 2, GL_FLOAT, GL_FALSE, 0, gLineEnds);
    checkGlError("glVertexAttribPointer");

    glEnableVertexAttribArray(gLineHandle);
    checkGlError("glEnableVertexAttribArray");

    glUniformMatrix4fv(gMVPHandle, 1, GL_FALSE, gMVPMatrix);
    checkGlError("glUniformMatrix4fv");

    glDrawArrays(GL_LINES, 0, 2*N);
    checkGlError("glDrawArrays");
}


