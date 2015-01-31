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
