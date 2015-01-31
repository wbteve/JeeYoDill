#ifndef __STREAMPLOT_H__
#define __STREAMPLOT_H__

#ifdef __cplusplus
extern "C" {
#endif


void renderFrame();
void setupGraphics(int w, int h);
void addDataPoint(float val);


#ifdef __cplusplus
}
#endif
#endif