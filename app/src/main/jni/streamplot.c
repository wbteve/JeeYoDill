/*
Copyright (c) 2015, Sagar Gubbi <sagar.writeme@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of JeeYo nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "streamplot.h"
#include "platform.h"

#include <jni.h>
#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define Nmax 10000
#define MAX_TEXTURES 10
#define EPS 1e-6

#define SCALE_HI_THRESH 0.99f
#define SCALE_LO_THRESH 0.25f
#define SCALE_TARG_THRESH 0.75f

int N = 1600; // Number of data points
int ptr = 0; // pointer to line
float lastYval = 0;

GLfloat gLineEnds[2*(2*Nmax)]; // 2*N lines, and 2*(2*N) line endings
GLfloat gMVPMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};
GLfloat gPoint[] = { -1.0f, 0.0f };
float scaleY = 1.0f;
float tranY = 0.0f;

static const char gVertexShader[] =
    "uniform mat4 u_MVPMatrix;\n"
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = u_MVPMatrix * vPosition;\n"
    "  gl_PointSize = 20.0f;\n"
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

    gPoint[0] = gLineEnds[ptr+2];
    gPoint[1] = val;

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

    float range = maxVal - minVal;
    float avg = (maxVal + minVal) / 2.0f;

    if( ((maxVal * scaleY) > SCALE_HI_THRESH) ||
        ((minVal * scaleY) < -SCALE_HI_THRESH) ||
        ((range*scaleY) < (2.0f * SCALE_LO_THRESH)))
    {
        scaleY = 2.0f * SCALE_TARG_THRESH / (range + EPS);
        tranY = -1.0f * scaleY * avg;
    }
    gMVPMatrix[5] = scaleY;
    gMVPMatrix[13] = tranY;
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

    glVertexAttribPointer(gLineHandle, 2, GL_FLOAT, GL_FALSE, 0, gPoint);
    checkGlError("glVertexAttribPointer");

    glEnableVertexAttribArray(gLineHandle);
    checkGlError("glEnableVertexAttribArray");

    glUniformMatrix4fv(gMVPHandle, 1, GL_FALSE, gMVPMatrix);
    checkGlError("glUniformMatrix4fv");

    glDrawArrays(GL_POINTS, 0, 1);
    checkGlError("glDrawArrays");

}


