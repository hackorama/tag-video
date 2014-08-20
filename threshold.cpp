#include "threshold.h"

#ifdef PTHREAD
/* c function and struct for pthread */
struct thread_data {
	int id;
	void *threshold;
};

void* 
threadWorker(void *arg) 
{
	struct thread_data *task = (struct thread_data*) arg;
	((Threshold *)task->threshold)->scheduleWork(task->id);
	return NULL;
}
#endif

Threshold::Threshold(Config *_config, Tagimage *_tagimage)
{
	config = _config;
	tagimage = _tagimage;

	init();

	if(tagimage->isValid()) { 
		resolveImageScaling();
		max_rgb = tagimage->maxRGB();
	}

	pixdebug = config->CHECK_VISUAL_DEBUG();
	if(pixdebug) dbgpixmap = config->DBGPIXMAP;
	else         dbgpixmap = NULL;
}

Threshold::Threshold(Config *_config, unsigned char *frame, int _frame_width, int _frame_height)
{
	config = _config;
	init();
	frame_width  = _frame_width;
	frame_height = _frame_height;
	if( frame != NULL ){
		tagframe = frame;
		resolveFrameScaling();
		max_rgb = config->WIN_DSHOW_MAXRGB;
	}	
	VIDEO = true;
}

Threshold::~Threshold()
{
}

void
Threshold::init()
{
	width   = 0;
	height  = 0;
	scale   = 1;
	span    = 0;
	max_rgb = 0;
	frame_width = 0;
	frame_height = 0;
	edgemap = NULL;
	multi_threaded = false;
	VIDEO = false;
	pixdebug = false;
#ifndef PTHREAD
    config->THREADS = 1; //if no pthread force to single thread
#endif
}

float
Threshold::getScale()
{
	return scale;
}

int
Threshold::mapSize()
{
	return width*height ;
}

void
Threshold::resolveImageScaling()
{
	int tag_width  = tagimage->getWidth();
	int tag_height = tagimage->getHeight();
	assert(tag_width > config->THRESHOLD_WINDOW_SIZE && tag_height > config->THRESHOLD_WINDOW_SIZE);

	if( tag_width < config->PIXMAP_SCALE_SIZE && 
	    tag_height < config->PIXMAP_SCALE_SIZE ) {
		width  = tag_width;
        height = tag_height;
	} else if(config->PIXMAP_NATIVE_SCALE || 
		config->PIXMAP_SCALE_SIZE < config->THRESHOLD_WINDOW_SIZE) {
			width  = tag_width;
			height = tag_height;
	}else{
		resolveScaling(tag_width, tag_height);
	}
	initEdgemap();
}

void
Threshold::resolveFrameScaling()
{
	assert(frame_width > config->THRESHOLD_WINDOW_SIZE && frame_height > config->THRESHOLD_WINDOW_SIZE);
	if( frame_width < config->PIXMAP_SCALE_SIZE && 
	    frame_height < config->PIXMAP_SCALE_SIZE ) {
		width  = frame_width;
        height = frame_height;
	} else if( config->PIXMAP_SCALE_SIZE < config->THRESHOLD_WINDOW_SIZE) {
		width  = frame_width;
		height = frame_height;
	}else{
		//FIXME VIDEO SCALING DISABLED
		resolveScaling(frame_width, frame_height);
		//width  = frame_width;
		//height = frame_height;
	}
	initEdgemap();
}

void
Threshold::resolveScaling(int tagwidth, int tagheight)
{
	scale  = tagwidth > tagheight ? 
		(float)tagwidth/(float)config->PIXMAP_SCALE_SIZE : 
		(float)tagheight/(float)config->PIXMAP_SCALE_SIZE ;
	width  = (int)((float)tagwidth/scale);
	height = (int)((float)tagheight/scale);
	if(scale > 1) { 
		//calculate span here once 
		//instead of everytime at getpixel()
		span = (int)(scale/2.0); 
		//do scaling end edge correction here
		//innstead of bounds checks every getPixel() calls
		//scaling begin edge correction is at getPixel()
		width-=span; 
		height-=span;
	}
	if(config->TAG_DEBUG) cout << " tag_width=" << tagwidth << " tag_height=" << tagheight  << endl;
}

void
Threshold::initEdgemap()
{
	if(config->TAG_DEBUG) cout << "SCALE: native_scale=" << config->PIXMAP_NATIVE_SCALE 
		<< " jpeg_scale=" << config->JPG_SCALE
		<< " fast_scale=" << config->PIXMAP_FAST_SCALE
		<< " scale=" << scale << " span=" << span 
		//<< " tag_width=" << tag_width << " tag_height=" << tag_height 
		<< " width=" << width << " height=" << height 
		<< " window=" << config->THRESHOLD_WINDOW_SIZE << endl;

	if(config->EDGE_MAP != NULL ){
		delete [] config->EDGE_MAP;
		config->EDGE_MAP = NULL;
	}
	config->EDGE_MAP = new bool[width*height]; 
	edgemap = config->EDGE_MAP;
	config->GRID_WIDTH = width;
	config->GRID_HEIGHT = height;
}

int
Threshold::framePixel(int x, int y)
{
	if( scale != 1 ) {
		y =(int) ((float)y * scale);
		x =(int) ((float)x * scale);
	}
	if( x <= 0 || y <= 0 ) return 0;
	if( x >= frame_width || y >= frame_height ) return 0;
	int i = ( (frame_height-y) * frame_width) + x; //FIXME VIDEO proper frame orientation check
	RGBBYTE *thispixel = (RGBBYTE*) tagframe + i;
	if(thispixel) return ( thispixel->R + thispixel->G + thispixel->B );
	return 0;
}
	

//NOTE: Removed tagimage::getPixel() bounds check for fast common case (scale==1)
// So make sure all  bounds check are done before calling tagimage::getPixel()
int
Threshold::getPixel(int x, int y)
{
	if(VIDEO) return framePixel(x,y);

	//NOTE: tagimage->getPixel(x,y) is same as config->PIXBUF[(y * width) + x];

	//no scaling, fastest
	if(scale == 1) return config->PIXBUF[(y * width) + x];
	//scaling by skipping, faster 
	if(config->PIXMAP_FAST_SCALE)   
		return tagimage->getPixel((int)((float)x*scale), (int)((float)y*scale) );	
	if( x == 0 || y == 0 ) //begin edge bounds check
		return tagimage->getPixel((int)((float)x*scale), (int)((float)y*scale) );	
	//scaling by averaging, slower
	int pixel = 0, count = 0;
	int ix = (int)((float)x*scale);
	int iy = (int)((float)y*scale);
	for(int i=0; i<=span*2; i++) { 
		pixel+=tagimage->getPixel(ix-span+i, iy);
		pixel+=tagimage->getPixel(ix, iy-span+i);
		count++;
	}
	assert( count == (span*2)+1 );
	return (pixel/count);
}


// parallel access from threads 
// do not modify class variable values here without mutex 
void
Threshold::scheduleWork(int id) 
{
	int offset = config->THRESHOLD_OFFSET * tagimage->COLORS * config->THRESHOLD_RGB_FACTOR;
	if(id == 0) 	 computeEdgemap(config->THRESHOLD_WINDOW_SIZE, offset, 0, height/2);
	else if(id == 1) computeEdgemap(config->THRESHOLD_WINDOW_SIZE, offset, height/2, height);
}

void
Threshold::computeEdgemap()
{
	ta = new bool[width*height]; //thresholded pixel on/off array //TODO  moving window of (3*width)
	for(int x = 0; x < (width*height); x++) edgemap[x] = false; 
#ifdef PTHREAD
	if( config->THREADS == 2 ){
		multi_threaded = true;
		pthread_t threads[2];
		struct thread_data t_data[2];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
   		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		for(int i = 0; i<2; i++){
			t_data[i].id = i;
			t_data[i].threshold = this;
			if (pthread_create(&threads[i], &attr, threadWorker, (void *)&t_data[i]) != 0) return;
		}
		pthread_attr_destroy(&attr);
		for(int i = 0; i<2; i++){
			if (pthread_join(threads[i], NULL) != 0) return;
		}
		fillEdgemap(); //for multi thread, do it after thresholding loop
	} else {
		int offset = config->THRESHOLD_OFFSET * tagimage->COLORS * config->THRESHOLD_RGB_FACTOR;
		if(scale == 1) computeEdgemapOpt(config->THRESHOLD_WINDOW_SIZE, offset);
		else           computeEdgemap(config->THRESHOLD_WINDOW_SIZE, offset, 0, height);
	}
#else
		int offset = 0;
		if(VIDEO)
			offset = config->THRESHOLD_OFFSET * config->WIN_DSHOW_COLORS * config->THRESHOLD_RGB_FACTOR;
		else
			offset = config->THRESHOLD_OFFSET * tagimage->COLORS * config->THRESHOLD_RGB_FACTOR;
		if(scale == 1) computeEdgemapOpt(config->THRESHOLD_WINDOW_SIZE, offset);
		else           computeEdgemap(config->THRESHOLD_WINDOW_SIZE, offset, 0, height);
#endif
	delete [] ta;
}

/* 
* Optimised Local Adapaive Thresholding using the MEAN value 
* of pixels in the local neighborhood (block) as the threshold
* and binarizes to on or off pixels based on threshold
* 
* Applies edge marking also in the same loop, based on the 
* thresholded on/off pixel values
* 
* Blocks are selected rows (width wise) starting from top left 
* 
* x,i,width - are in x plane    y,j,height - are in y plane 
*
* Neighborhood sums are calculated based on stored deltas from 
* the last neighborhood on left and last row of blocks above 
*
* All border pixels (x < width, y < w, x > width-radius, y > width-radis) 
* have partial neighborhood block size
*
* NOTE: All condition checks are optmised and is order dependent 
* 
* TODO : Verify if the diagonal above check in edge marking can be removed 
* ( check border tracing )
*/

/*
 computeEdgemap	   
	- Extra step of getPixel() function call to access  Config->PIXBUF[]
	- getPixel() handles the span and scale
 computeEdgemapOpt 
	- Directly Access Config->PIXBUF[]
	- only if not using span or scale
	- Saves only 0.01 second in performance

Identical versions, one for slightly better performance 
*** has to maintain both versions as exactly the same ****

*/ 
void 
Threshold::computeEdgemapOpt(int size, int offset)
{
	if(VIDEO) return computeEdgemap(size, offset, 0, height); //No optimization yet for video frames

	long threshold = 0, lastdelta = 0, di = 0;
	bool thispixel = false, epixel = false; 
	int ex = 0, ey = 0, ei = 0;
	int blocksize = size*size, radius = size/2, half_block = blocksize/2;

	unsigned char* pixbuf = config->PIXBUF;

	int *td = new int[width*height]; //threshold deltas //TODO moving window of (size*width)
	int *ts = new int[width];   //threshold sums

	for(int x = 0;  x < (width*height); x++) td[x] = 0;
    for(int x = 0; x < width; x++)  ts[x] = 0;

	for(int y = 0; y < height; y++){ 
		for(int x = 0; x < width; x++){
			if( y >= (height-radius) ){ //partial: bottom rows
				ts[x] -= td[((y-radius)*width)+x];
				threshold = ts[x] / (size*(radius+height-y)); 
			}else if( y > radius && x == 0 ){ //normal: very first 
				di = ((y+radius)*width) + x;
				for(int i = 0; i < radius; i++) td[di]+=pixbuf[i + width * (y+radius)]; 
				ts[x] += td[di] - td[((y-radius-1)*width)+x];
				threshold = ts[x] /half_block; 
				lastdelta = td[di];
			}else if( y > radius && x >= width - radius ){//normal: partial end 
				di = ((y+radius)*width) + x;
				td[di] = lastdelta - pixbuf[(x-radius-1) + width * (y+radius)];
				ts[x] += td[di] - td[((y-radius)*width)+x];
				threshold = ts[x] / ((width-x+radius)*size);
				lastdelta = td[di];
			}else if( y > radius && x <=radius ){ //normal: partial begin 
				di = ((y+radius)*width) + x;
				td[di]=lastdelta + pixbuf[(x+radius) + width * (y+radius)];
				ts[x]+= td[di] - td[((y-radius)*width)+x];
				threshold = ts[x] / ((x+radius)*size);
				lastdelta = td[di];
			}else if( y > radius ){ //normal: all full (90% of all)
				di = ((y+radius)*width) + x;
				td[di] = lastdelta + pixbuf[(x+radius) + width * (y+radius)] 
					- pixbuf[(x-radius-1) + width * (y+radius)];
				ts[x] += td[di] - td[((y-radius)*width)+x];
				threshold = ts[x] / blocksize;
				lastdelta = td[di];
			}else if( x == 0 && y == 0 ){ //first top left block 
				for(int j = 0; j < radius; j++){
					di = j*width;
					for(int i = 0; i < radius; i++) td[di] += pixbuf[i + width *  j];
					ts[x] += td[di];
				}
				threshold = ts[x]  /(blocksize/4); 
			}else if( y == 0 ){ //partial: topmost row all
				for(int j = 0; j < radius; j++){
					di = (j*width) + x;
					td[di] = td[di-1];
					if(x > radius)    td[di] -= pixbuf[(x-radius-1) + width *  j];
					if(x <= width-radius) td[di] += pixbuf[(x+radius) + width *  j];
					ts[x] += td[di];
				}
				if(x <= radius) threshold = ts[x] / (radius*(x+radius));
				else if(x <= width-radius) threshold = ts[x] / half_block;
				else            threshold = ts[x] / ((width-x+radius)*radius);
			}else if( y <= radius && x <= radius){ //partial: top row begin
				di = ((y+radius)*width) + x;
				for(int i = 0; i < x+radius; i++) td[di] 
				+= pixbuf[i + width * (y+radius)]; 
				ts[x] += td[di];
				threshold = ts[x] / ((x+radius)*(y+radius)); 
			}else if( y <= radius && x >= width -radius){ //partial: top row end
				di = ((y+radius)*width) + x;
				for(int i = width; i > x-radius; i--) td[di] 
				+= pixbuf[i + width * (y+radius)];
				ts[x] += td[di];
				threshold = ts[x] / ((width-x+radius)*(y+radius)); 
			}else if( y <= radius ){ //partial: top row all
				di = ((y+radius)*width) + x;
				for(int i = 0; i < size; i++) td[di] 
				+= pixbuf[(x-radius+i) + width * (y+radius)]; 
				ts[x] += td[di];
				threshold = ts[x] / (size*(y+radius)); 
			}
			threshold-=offset;
			thispixel = pixbuf[x + width *  y] < threshold ? true : false;
			ta[(y*width)+x] = thispixel;
			if(pixdebug) thispixel ? d_setPixelFilled(x, y) : d_setPixelBlank(x, y);
			if( ! multi_threaded  ){ //single thread do it in the thresholding loop
				if( y > 2){ //edge marking based on ta[]
					ex = x; ey = y-2;
					ei = ((ey)*width)+ex;
					epixel = ta[ei];
					if( epixel ){
						if( ex > 0 && ex < width ){
							if( epixel ^ ta[(ey*width)+ex-1]
							|| epixel ^ ta[(ey*width)+ex+1]
							|| epixel ^ ta[((ey-1)*width)+ex]
							|| epixel ^ ta[((ey-1)*width)+ex+1]
							|| epixel ^ ta[((ey-1)*width)+ex-1]
							|| epixel ^ ta[((ey+1)*width)+ex]
							|| epixel ^ ta[((ey+1)*width)+ex+1]
							|| epixel ^ ta[((ey+1)*width)+ex-1] ){
								edgemap[ei] = true;		
								if(pixdebug) d_setPixelMarked(ex, ey);	
							}
						}
					}
				}
			}
			//close out borders on bottom edge
			//if((y == height-1) && thispixel) d_setPixelMarked(x, y); 
		}
		//close out borders on right edge
		//if((x == width) && thispixel) d_setPixelMarked(x-1, y); 
	}

	delete [] ts;
	delete [] td;
}

// parallel access from threads 
// do not modify class variable values here without mutex 
void 
Threshold::computeEdgemap(int size, int offset, int y1, int y2)
{
	long threshold = 0, lastdelta = 0, di = 0;
	bool thispixel = false, epixel = false; 
	int ex = 0, ey = 0, ei = 0;
	int blocksize = size*size, radius = size/2, half_block = blocksize/2;

	//overlapping segment correction 
	if( y1 > 0 ) 	  y1-=size; //all segments beginning in middle of tagimage
	if( y2 < height ) y2+=size; //all segments ending in the middle of tagimage

	int new_radius = radius + y1; 

	int *td = new int[width*height]; //threshold deltas //TODO moving window of (size*width)
	int *ts = new int[width];   //threshold sums

	for(int x = 0;  x < (width*height); x++) td[x] = 0;
    for(int x = 0; x < width; x++)  ts[x] = 0;

	for(int y = y1; y < y2; y++){ 
		if(config->TAG_DEBUG) { //TODO: Remove once threads are final 
			if(y1 == 0) printf("*");
			if(y1 > 0) printf("-");
		}
		for(int x = 0; x < width; x++){
			if( y >= (height-radius) ){ //partial: bottom rows
				ts[x] -= td[((y-radius)*width)+x];
				threshold = ts[x] / (size*(radius+height-y)); 
			}else if( y > radius && x == 0 ){ //normal: very first 
				di = ((y+radius)*width) + x;
				for(int i = 0; i < radius; i++) td[di]+=getPixel(i, y+radius); 
				ts[x] += td[di] - td[((y-radius-1)*width)+x];
				threshold = ts[x] /half_block; 
				lastdelta = td[di];
			}else if( y > radius && x >= width - radius ){//normal: partial end 
				di = ((y+radius)*width) + x;
				td[di] = lastdelta - getPixel(x-radius-1,y+radius);
				ts[x] += td[di] - td[((y-radius)*width)+x];
				threshold = ts[x] / ((width-x+radius)*size);
				lastdelta = td[di];
			}else if( y > radius && x <=radius ){ //normal: partial begin 
				di = ((y+radius)*width) + x;
				td[di]=lastdelta + getPixel(x+radius, y+radius);
				ts[x]+= td[di] - td[((y-radius)*width)+x];
				threshold = ts[x] / ((x+radius)*size);
				lastdelta = td[di];
			}else if( y > radius ){ //normal: all full (90% of all)
				di = ((y+radius)*width) + x;
				td[di] = lastdelta + getPixel(x+radius, y+radius) 
					- getPixel(x-radius-1,y+radius);
				ts[x] += td[di] - td[((y-radius)*width)+x];
				threshold = ts[x] / blocksize;
				lastdelta = td[di];
			}else if( x == 0 && y == 0 ){ //first top left block 
				for(int j = y1; j < radius; j++){
					di = j*width;
					for(int i = 0; i < radius; i++) td[di] += getPixel(i, j);
					ts[x] += td[di];
				}
				threshold = ts[x]  /(blocksize/4); 
			}else if( y == y1 ){ //partial: topmost row all
				for(int j = y1; j < radius; j++){
					di = (j*width) + x;
					td[di] = td[di-1];
					if(x > radius)    td[di] -= getPixel(x-radius-1, j);
					if(x <= width-radius) td[di] += getPixel(x+radius, j);
					ts[x] += td[di];
				}
				if(x <= radius) threshold = ts[x] / (radius*(x+radius));
				else if(x <= width-radius) threshold = ts[x] / half_block;
				else            threshold = ts[x] / ((width-x+radius)*radius);
			}else if( y <= new_radius && x <= radius){ //partial: top row begin
				di = ((y+radius)*width) + x;
				for(int i = 0; i < x+radius; i++) td[di] 
				+= getPixel(i, y+radius); 
				ts[x] += td[di];
				threshold = ts[x] / ((x+radius)*(y+radius)); 
			}else if( y <= new_radius && x >= width -radius){ //partial: top row end
				di = ((y+radius)*width) + x;
				for(int i = width; i > x-radius; i--) td[di] 
				+= getPixel(i, y+radius);
				ts[x] += td[di];
				threshold = ts[x] / ((width-x+radius)*(y+radius)); 
			}else if( y <= new_radius ){ //partial: top row all
				di = ((y+radius)*width) + x;
				for(int i = 0; i < size; i++) td[di] 
				+= getPixel((x-radius)+i, y+radius); 
				ts[x] += td[di];
				threshold = ts[x] / (size*(y+radius)); 
			}

			//skip overlap only for beginning on the middle 
			//will be covered by the one above ( applies to multi thread only)
			if(  y1 == 0 || y > y1+size){ 
				threshold-=offset;
				thispixel = getPixel(x, y) < threshold ? true : false;
				ta[(y*width)+x] = thispixel;
				if(pixdebug) thispixel ? d_setPixelFilled(x, y) : d_setPixelBlank(x, y);
			}

			if( config->THREADS < 2 ){ //single thread do it in the thresholding loop
				if( y > 2){ //edge marking based on ta[]
					ex = x; ey = y-2;
					ei = ((ey)*width)+ex;
					epixel = ta[ei];
					if( epixel ){
						if( ex > 0 && ex < width ){
							if( epixel ^ ta[(ey*width)+ex-1]
							|| epixel ^ ta[(ey*width)+ex+1]
							|| epixel ^ ta[((ey-1)*width)+ex]
							|| epixel ^ ta[((ey-1)*width)+ex+1]
							|| epixel ^ ta[((ey-1)*width)+ex-1]
							|| epixel ^ ta[((ey+1)*width)+ex]
							|| epixel ^ ta[((ey+1)*width)+ex+1]
							|| epixel ^ ta[((ey+1)*width)+ex-1] ){
								edgemap[ei] = true;		
								if(pixdebug || config->V_D_PIXDEBUG) d_setPixelMarked(ex, ey);	
							}else{
								if(pixdebug || config->V_D_PIXDEBUG) d_setPixelBlank(ex, ey);	
							}
						}
					}else{
						//if(VIDEO) d_setPixelBlank(ex, ey);	
					}
				}
			}
			//close out borders on bottom edge
			//if((y == height-1) && thispixel) d_setPixelMarked(x, y); 
		}
		//close out borders on right edge
		//if((x == width) && thispixel) d_setPixelMarked(x-1, y); 
	}

	delete [] ts;
	delete [] td;
}

void 
Threshold::fillEdgemap()
{
	int ex = 0, ey = 0, ei = 0; 
	bool epixel = false;

	for(int y = 0; y < height; y++){ 
		for(int x = 0; x < width; x++){
			if( y > 2){ //edge marking based on ta[]
				ex = x; ey = y-2;
				ei = ((ey)*width)+ex;
				epixel = ta[ei];
				if( epixel ){
					if( ex > 0 && ex < width ){
						if( epixel ^ ta[(ey*width)+ex-1]
						|| epixel ^ ta[(ey*width)+ex+1]
						|| epixel ^ ta[((ey-1)*width)+ex]
						|| epixel ^ ta[((ey-1)*width)+ex+1]
						|| epixel ^ ta[((ey-1)*width)+ex-1]
						|| epixel ^ ta[((ey+1)*width)+ex]
						|| epixel ^ ta[((ey+1)*width)+ex+1]
						|| epixel ^ ta[((ey+1)*width)+ex-1] ){
							edgemap[ei] = true;		
							if(pixdebug) d_setPixelMarked(ex, ey);	
						}
					}
				}
			}
		}
	}
}

void
Threshold::d_setPixelMarked(int x, int y) //GREEN
{
	if(VIDEO){
		y =(int) ((float)y * scale);
		x =(int) ((float)x * scale);
		int i = ( (frame_height-y) * frame_width) + x; //FIXME VIDEO proper frame orientation check
		RGBBYTE *thispixel = (RGBBYTE*) tagframe + i;
		thispixel->R = 0;
		thispixel->G = 255;
		thispixel->B = 0;
	}else{
		dbgpixmap->setPixel((int)((float)x*scale), (int)((float)y*scale), 0, max_rgb, 0);	
	}
}

void
Threshold::d_setPixelBlank(int x, int y) //WHITE 
{
	if(VIDEO){
		y =(int) ((float)y * scale);
		x =(int) ((float)x * scale);
		int i = ( (frame_height-y) * frame_width) + x; //FIXME VIDEO proper frame orientation check
		RGBBYTE *thispixel = (RGBBYTE*) tagframe + i;
		thispixel->R = 255;
		thispixel->G = 0;
		thispixel->B = 0;
	}else{
		dbgpixmap->setPixel((int)((float)x*scale), (int)((float)y*scale), max_rgb, max_rgb, max_rgb);	
	}
}

void
Threshold::d_setPixelFilled(int x, int y) //BLACK
{
	if(VIDEO){
		y =(int) ((float)y * scale);
		x =(int) ((float)x * scale);
		int i = ( (frame_height-y) * frame_width) + x; //FIXME VIDEO proper frame orientation check
		RGBBYTE *thispixel = (RGBBYTE*) tagframe + i;
		thispixel->R = 0;
		thispixel->G = 0;
		thispixel->B = 0;

	}else{
		dbgpixmap->setPixel((int)((float)x*scale), (int)((float)y*scale), 0, 0, 0);	
	}
}

