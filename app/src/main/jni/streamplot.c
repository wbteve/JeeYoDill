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



typedef struct Streamplot {
    GLfloat color[4];
    GLfloat thickness;
    GLfloat data[4*STREAMPLOT_N_MAX_POINTS];
} Streamplot;

int nPlots;
int freeze = 0;
int width, height;
int lastEvent;
float initPinchEventDx = 0;
float initPinchEventX = 0;
float lastTranX, lastScaleX;
int startPtr = STREAMPLOT_N_MAX_POINTS/2 - 800, endPtr = STREAMPLOT_N_MAX_POINTS/2 + 800;
//int startPtr = 0, endPtr = STREAMPLOT_N_MAX_POINTS;
int ptr;

Streamplot plots[STREAMPLOT_N_MAX_PLOTS];

GLfloat gMVPMatrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };


GLuint gProgram;

GLuint gLineHandle;
GLuint gMVPHandle;
GLuint gColorHandle;


static const char gVertexShader[] =
    "uniform mat4 u_MVPMatrix;\n"
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = u_MVPMatrix * vPosition;\n"
    "  gl_PointSize = 20.0f;\n"
    "}\n";

static const char gFragmentShader[] =
    "precision mediump float;\n"
    "uniform vec4 u_Color;\n"
    "void main() {\n"
    "  gl_FragColor = u_Color;\n"
    "}\n";

static GLuint loadShader(GLenum shaderType, const char* pSource) {
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

static GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
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

static void setScale() {
    int i, j;
    float maxVal = -INFINITY, minVal = INFINITY, val, scaleY, tranY;

    for(i = 0;i < nPlots; i++) {
        for(j = startPtr; j < endPtr; j++) {
            val = plots[i].data[4*j + 1];
            if(val > maxVal)
                maxVal = val;
            if(val < minVal)
                minVal = val;
        }
    }

    float range = maxVal - minVal;
    float avg = (maxVal + minVal) / 2.0f;

    scaleY = gMVPMatrix[5];
    tranY = gMVPMatrix[13];

    float maxY = maxVal*scaleY + tranY;
    float minY = minVal*scaleY + tranY;

    if( (maxY > STREAMPLOT_SCALE_HI_THRESH) ||
        (minY < -STREAMPLOT_SCALE_HI_THRESH) ||
        ((range*scaleY) < (2.0f * STREAMPLOT_SCALE_LO_THRESH)))
    {
        scaleY = 2.0f * STREAMPLOT_SCALE_TARG_THRESH / (range + STREAMPLOT_EPS);
        tranY = -1.0f * scaleY * avg;
    }
    gMVPMatrix[5] = scaleY;
    gMVPMatrix[13] = tranY;
}

static void clearScreen() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    checkGlError("glClearColor");

    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");
}

static void renderPlots() {
    int i;

    for(i = 0; i < nPlots; i++) {
        // Draw the line

        glLineWidth(plots[i].thickness);
        checkGlError("glLineWidth");

        glUseProgram(gProgram);
        checkGlError("glUseProgram");

        glVertexAttribPointer(gLineHandle, 2, GL_FLOAT, GL_FALSE, 0, plots[i].data);
        checkGlError("glVertexAttribPointer");

        glEnableVertexAttribArray(gLineHandle);
        checkGlError("glEnableVertexAttribArray");

        glUniformMatrix4fv(gMVPHandle, 1, GL_FALSE, gMVPMatrix);
        checkGlError("glUniformMatrix4fv");

        glUniform4fv(gColorHandle, 1, plots[i].color);
        checkGlError("glUniform4fv");

        glDrawArrays(GL_LINES, 0, 2*STREAMPLOT_N_MAX_POINTS);
        checkGlError("glDrawArrays");

        // Draw the marker point
        GLfloat pointMarker[2] = {plots[i].data[ptr*4 + 2], plots[i].data[ptr*4 + 3]};
        glVertexAttribPointer(gLineHandle, 2, GL_FLOAT, GL_FALSE, 0, pointMarker);
        checkGlError("glVertexAttribPointer");

        glEnableVertexAttribArray(gLineHandle);
        checkGlError("glEnableVertexAttribArray");

        glUniformMatrix4fv(gMVPHandle, 1, GL_FALSE, gMVPMatrix);
        checkGlError("glUniformMatrix4fv");

        glDrawArrays(GL_POINTS, 0, 1);
        checkGlError("glDrawArrays");
    }
}

void StreamplotInit(int numPlots, StreamplotType* plotTypes, int screenWidth, int screenHeight) {
    int i, j;
    int w = screenWidth;
    int h = screenHeight;

    width = w;
    height = h;

    nPlots = numPlots;
    ptr = startPtr;

    float scaleX = (float)STREAMPLOT_N_MAX_POINTS * 1.0f / (endPtr - startPtr);
    gMVPMatrix[0] = scaleX;

    for(i = 0;i < nPlots; i++) {
        for(j = 0; j < 4; j++) {
            plots[i].color[j] = plotTypes[i].color[j];
        }
        plots[i].thickness = plotTypes[i].thickness;

        for(j = 0;j < STREAMPLOT_N_MAX_POINTS;j++) {
            plots[i].data[4*j] = -1.0f + (j * 2.0f) / STREAMPLOT_N_MAX_POINTS;
            plots[i].data[4*j + 1] = 0.0f;

            plots[i].data[4*j + 2] = -1.0f + ((j+1) * 2.0f) / STREAMPLOT_N_MAX_POINTS;
            plots[i].data[4*j + 3] = 0.0f;
        }
    }

    LOGI("StreamplotInit(%d, %d)", w, h);

    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return;
    }

    gLineHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");

    gMVPHandle = glGetUniformLocation(gProgram, "u_MVPMatrix");
    checkGlError("glGetUniformLocation");

    gColorHandle = glGetUniformLocation(gProgram, "u_Color");
    checkGlError("glGetUniformLocation");

    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
            gLineHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
}

static void incrementPtr() {
    ptr = ptr + 1;
    if(ptr >= endPtr) {
        ptr = startPtr;
    }
}

void StreamplotMainLoop(int nDataPoints, float* data, StreamplotEvent evt)
{
    // Process events, if any.
    if(evt.event != 0) {
        LOGI("Event: %d", evt.event);

        float scaleX = gMVPMatrix[0];
        float tranX = gMVPMatrix[12];
        float eventDx = abs(evt.eventX0 - evt.eventX1);

        // Pinch start
        if(evt.event == STREAMPLOT_EVENT_PINCH && lastEvent != STREAMPLOT_EVENT_PINCH) {
            initPinchEventX = -1 + (evt.eventX0 + evt.eventX1) / width;
            initPinchEventDx = eventDx;
            freeze = 1;
            lastScaleX = scaleX;
            lastTranX = tranX;
            LOGI("Pinch start-X: %f", initPinchEventX);
        }

        // Pinch in progress
        if(evt.event == STREAMPLOT_EVENT_PINCH && lastEvent == STREAMPLOT_EVENT_PINCH) {
            float relDx = (eventDx - initPinchEventDx) / width;
            float scaleFactor = 10.0f;
            if(relDx > 0) {
                scaleX = lastScaleX * (1 + scaleFactor*relDx);
            }
            else {
                scaleX = lastScaleX / (1 - scaleFactor*relDx);
            }
            tranX = initPinchEventX - ((initPinchEventX-lastTranX) / lastScaleX * scaleX);

            float xStart = (-1.0f - tranX) / scaleX;
            float xEnd = (1.0f - tranX) / scaleX;

            if(xStart >= -1.0f && xEnd <= 1.0f && scaleX < STREAMPLOT_MAX_ZOOM) {
                gMVPMatrix[12] = tranX;
                gMVPMatrix[0] = scaleX;
            }
            scaleX = gMVPMatrix[0];
            tranX = gMVPMatrix[12];

            xStart = (-1.0f - tranX) / scaleX;
            xEnd = (1.0f - tranX) / scaleX;

            startPtr = (int)((xStart+1.0f)*STREAMPLOT_N_MAX_POINTS/2);
            endPtr = (int)((xEnd+1.0f)*STREAMPLOT_N_MAX_POINTS/2);

            if(startPtr < 0)
                startPtr = 0;

            if(endPtr > STREAMPLOT_N_MAX_POINTS)
                endPtr = STREAMPLOT_N_MAX_POINTS;

            if(ptr > endPtr || ptr < startPtr)
                ptr = startPtr;


            LOGI("Pinch in Progress: %f", relDx);
        }

        // Pinch end
        if(evt.event == STREAMPLOT_EVENT_UP && lastEvent == STREAMPLOT_EVENT_PINCH) {
            freeze = 0;
            LOGI("Pinch end");
        }

        // Plain touch release
        if(evt.event == STREAMPLOT_EVENT_UP && lastEvent != STREAMPLOT_EVENT_PINCH) {
            freeze = !freeze;
        }
        lastEvent = evt.event;
    }

    // Add new data, if any.
    int i, j;
    int nPoints = nDataPoints / nPlots;

    assert(nPlots*nPoints == nDataPoints);
    if(!freeze) {
        for(i = 0;i < nPoints;i++) {
            int localLastPtr = ptr;
            incrementPtr();
            int localPtr = ptr;
            for(j = 0; j < nPlots; j++) {
                plots[j].data[localPtr*4 + 1] = plots[j].data[localLastPtr*4 + 3];
                plots[j].data[localPtr*4 + 3] = data[i*nPlots + j];
            }
        }
    }

    // Adjust y-axis scaling
    setScale();

    // Draw the plots
    clearScreen();
    renderPlots();
}
