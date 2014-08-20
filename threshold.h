#ifndef _TRANSFORM_H_INCLUDED
#define _TRANSFORM_H_INCLUDED

// #define PTHREAD

#include "tagimage.h"
#include "pixmap.h"
#include "common.h"
#include "config.h"

#ifdef PTHREAD
#include <pthread.h>
#endif

#include <vector>
#include <algorithm>

using namespace std;

class Threshold
{
public:
	Threshold(Config *config, Tagimage *pixin);
	Threshold(Config *config, unsigned char* pixin, int pixw, int pixh);
	~Threshold();
	void setPixmap(Pixmap *pixmap);
	void computeEdgemapOpt(int size, int offset);
	void computeEdgemap(int size, int offset, int y1, int y2);
	void computeEdgemap();
	int  mapSize();
	void scheduleWork(int id);
	float getScale();

private:
	Config   *config;
	Tagimage *tagimage;
	unsigned char *tagframe; //only for video frames
	bool  *edgemap;
	bool  *ta;
	int   width, height;
	int   frame_width, frame_height;
	float scale;
	int   span;
	int   max_rgb;
	bool  multi_threaded;
	bool  VIDEO;

	void init();
	int  getPixel(int x, int y);
	int  framePixel(int x, int y);
	void resolveScaling(int width, int height);
	void resolveImageScaling();
	void resolveFrameScaling();
	void initEdgemap();
	void fillEdgemap();


	//debug only 
	bool pixdebug;
	Pixmap *dbgpixmap;

	void d_setPixelMarked(int i, int j);
	void d_setPixelBlank(int i, int j);
	void d_setPixelFilled(int i, int j);
	//debug only 
};

#endif /* _TRANSFORM_H_INCLUDED */
