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
package com.jeeyo.sagar.jeeyodill;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by sagar on 28/1/15.
 */
public class RendererWrapper implements GLSurfaceView.Renderer {

    public boolean allowNewVals = true;

    ConcurrentLinkedQueue<Float> mQueue = new ConcurrentLinkedQueue<>();

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        StreamplotJNIWrapper.on_surface_created();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        StreamplotJNIWrapper.on_surface_changed(width, height);
    }
    int mN = 0;
    float scale = 3.0f;
    @Override
    public void onDrawFrame(GL10 gl) {

        while(true) {
            Float d = mQueue.poll();
            if(d == null) {
                break;
            }
            if(allowNewVals)
                StreamplotJNIWrapper.add(d);
        }
        /*
        for(int i = 0; i < 6; i++) {
            if (allowNewVals)
                StreamplotJNIWrapper.add(mN * scale/500);
            mN = (mN + 1) % 253;
            if(mN == 0) {
                if(scale == 1.0f) {
                    scale = 0.6f;
                } else {
                    scale = 1.0f;
                }
            }
        } */
        StreamplotJNIWrapper.on_draw_frame();
    }

    public void addDataPoint(int val) {
        float v;
        if(val < 8192 || val >= 65530) {
            v = (float) val;
        } else {
            v = (float) -val;
        }
        if(v < 65530) {
            mQueue.add(v);
        }
    }
}
