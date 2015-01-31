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

    ConcurrentLinkedQueue<Integer> mQueue = new ConcurrentLinkedQueue<>();

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        StreamplotJNIWrapper.on_surface_created();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        StreamplotJNIWrapper.on_surface_changed(width, height);
    }

    int mI = 0;
    int nI = 253;
    float amp = 2;

    @Override
    public void onDrawFrame(GL10 gl) {
        /*
        while(true) {
            Integer d = mQueue.poll();
            if(d == null) {
                break;
            }
            StreamplotJNIWrapper.add(d);
        }
        */
        for(int i = 0; i < 6; i++) {
            mI = (mI + 1) % 253;
            if(allowNewVals)
                StreamplotJNIWrapper.add(amp*mI);
            if(mI == 0 && amp > 0.11) {
                amp = amp + 0.1f;
            }
        }
        StreamplotJNIWrapper.on_draw_frame();
    }

    public void addDataPoint(int val) {
        mQueue.add(val);
    }
}
