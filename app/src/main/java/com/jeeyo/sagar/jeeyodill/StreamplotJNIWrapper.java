package com.jeeyo.sagar.jeeyodill;

/**
 * Created by sagar on 28/1/15.
 */
public class StreamplotJNIWrapper {
    static {
        System.loadLibrary("streamplot");
    }
    public static native void on_surface_created();

    public static native void on_surface_changed(int width, int height);

    public static native void on_draw_frame();

    public static native void add(int data);
}
