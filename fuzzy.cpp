#include "fuzzy.h"

Fuzzy::Fuzzy() 
	:	MD_WIN_WIDTH(100), 
		MD_WIN_HEIGHT(100),
		MD_THRESHOLD_PERCENT(10),
		MD_HISTORY_SIZE(30),
		MD_OFFSET(382),  //FIXME base on config limits
		TAG_HISTORY_SIZE(6),
		TAG_LENGTH(12),
		OLD_TAG(40),
		VERY_OLD_TAG(80),
		VALID_TAG_REPEATS(3),
		TAG_ACTIVE_FOR(10)
{

	assert(TAG_HISTORY_SIZE >= VALID_TAG_REPEATS);

	debug = false;
	d_videodebug = false;
	md_lastframe = NULL;
	md_frame_deltas = new int[MD_HISTORY_SIZE];
	for( int i = 0; i < MD_HISTORY_SIZE; i++) md_frame_deltas[i] = 0;
	tag_backstore = new int[TAG_LENGTH* TAG_HISTORY_SIZE];
	for( int i = 0; i < TAG_LENGTH* TAG_HISTORY_SIZE; i++) tag_backstore[i] = -1; 

	md_win_x1 = 0;
	md_win_x2 = 0;
	md_win_y1 = 0;
	md_win_y2 = 0;

	md_index = 0;
	tag_index = 0;

	valid_for = -1; //skip first valid

	md_pixel_threshold = 0;
	real_frame_count = 0;
	used_frame_count = -1;
}

Fuzzy::~Fuzzy()
{
	if(md_lastframe!=NULL) 	  	delete [] md_lastframe;
	if(md_frame_deltas!=NULL) 	delete [] md_frame_deltas;
	if(tag_backstore!=NULL) 	delete [] tag_backstore;
	
}

void 
Fuzzy::setDebug(bool _debug)
{
	debug = _debug;
}

bool 
Fuzzy::selectFrame(int skip_frames)
{
	// if(valid_for >= 1) valid_for++;  
	if(real_frame_count++ % skip_frames == 0)  { 
		used_frame_count++;
		return true;
	}
	return false;
}

/*
bool 
Fuzzy::trackFrame() //DEL
{
	if(valid_for>=1) return true;  
	return false;
}

void 
Fuzzy::countFrame() //DEL
{
	frame_count++;
	//only if last one was a valid frame
	if(valid_for >= 1) valid_for++;
}
*/

bool 
Fuzzy::motionDetect(unsigned char *frame_data, int frame_width, int frame_height)
{
	checkFrameStores(frame_width, frame_height);
	int d = computeDelta(frame_data, frame_width, frame_height);
	if( d > md_pixel_threshold ){ //exceeds max threshold frame to frame
		if(debug) printf( "Fuzzy::motionDetect [%ld] d=%d > t=%d Exit\n", used_frame_count, d, md_pixel_threshold );
		return true;
	}else{ //or exceeds average delta of the last few frames
		int sum = 0;
		for(int i=0; i<MD_HISTORY_SIZE; i++) sum+=md_frame_deltas[i];
		if(d > 2*(sum/MD_HISTORY_SIZE) ) { 
			if(debug) printf("Fuzzy::motionDetect [%ld] d=%d  a=%d Exit\n", used_frame_count, d, sum/MD_HISTORY_SIZE);
			return true;
		}
	}
	return false;
}

void 
Fuzzy::checkFrameStores(int frame_width, int frame_height)
{
	if( frame_width == last_frame_width && frame_height == last_frame_height ) return;

	//relocate the md sub window 
	calculateMDWindow(frame_width, frame_height);

	//allocate first time or reallocate new size frame
	if( md_lastframe == NULL ){
		/*md_lastframe = new bool[(md_win_x2-md_win_x1) * (md_win_y2-md_win_y1)];
		for(int i=0; i< (md_win_x2-md_win_x1) * (md_win_y2-md_win_y1) ; i++) md_lastframe[i] = false;*/
		//FIXME use smaller storage as above and change indexing in computeDelta()
		md_lastframe = new bool[frame_width*frame_height];
		for(int i=0; i< frame_width*frame_height; i++) md_lastframe[i] = false;
	}else{
		delete [] md_lastframe;
		md_lastframe = NULL;
	}
	md_pixel_threshold = ( (md_win_x2-md_win_x1) * (md_win_y2-md_win_y1) ) * MD_THRESHOLD_PERCENT / 100;
	last_frame_width = frame_width;
	last_frame_height = frame_height;
}

int 
Fuzzy::computeDelta(unsigned char *frame_data, int frame_width, int frame_height)
{
 	int i = 0;
	RGBBYTE *rgb;
	bool pixel = false;
	int delta = 0;
	for(int y = md_win_y1; y < md_win_y2; y++){
		for(int x = md_win_x1; x < md_win_x2; x++){
			i = ((frame_height-y) *frame_width) + x;
			rgb = (RGBBYTE*) frame_data + i;			
			pixel = (rgb->R + rgb->G + rgb->B)  > MD_OFFSET ? true : false ;
			if(used_frame_count) if(md_lastframe[i] != pixel) delta++;	 //not first frame
			md_lastframe[i] = pixel; //FIXME  change indexing 
			if(d_videodebug) {
				if(pixel){
					rgb->R = 255;
					rgb->G = 255;
					rgb->B = 255;
				}else{
					rgb->R = 0;
					rgb->G = 0;
					rgb->B = 0;
				}
			}
		}
	}
	md_frame_deltas[used_frame_count % MD_HISTORY_SIZE] = delta; //not first frame
	return delta;
}

bool
Fuzzy::tagIsActive()
{
	if(valid_for >= 1 && valid_for <= TAG_ACTIVE_FOR) return true;
	return false;
}

void 
Fuzzy::calculateMDWindow(int width, int height)
{
	//define centered subwindow MD_WIN_WIDTH * MD_WIN_HEIGHT
	//adjusts subwindow size if frame is smaller than subwindow
	if( width > MD_WIN_WIDTH ) {
		md_win_x1 = (width/2) - (MD_WIN_WIDTH/2);
		md_win_x2 = (width/2) + (MD_WIN_WIDTH/2);
	}else{
		md_win_x1 = 0;
		md_win_x2 = width;
	}
	if( height > MD_WIN_HEIGHT ) {
		md_win_y1 = (height/2) - (MD_WIN_HEIGHT/2);
		md_win_y2 = (height/2) + (MD_WIN_HEIGHT/2);
	}else{
		md_win_y1 = 0;
		md_win_y2 = height;
	}
}

bool 
Fuzzy::tagIsBrandNew()
{
	if(valid_for == 1) return true;
	return false;
}

bool 
Fuzzy::tagIsNew()
{
	if(valid_for < OLD_TAG) return true;
	return false;
}

bool 
Fuzzy::tagIsOld()
{
	if(valid_for >= OLD_TAG) return true;
	return false;
}

bool 
Fuzzy::tagIsVeryOld()
{
	if(valid_for > VERY_OLD_TAG) return true;
	return false;
}

bool 
Fuzzy::validateTag(int *tag)
{
	storeTag(tag);
	bool result = allValidTag(tag);
	if(result) {
		printf("checking tag valid_for = %d\n", valid_for );
		for(int i=0; i<VALID_TAG_REPEATS; i++){
			//TODO now storing skipped tags, optimize with separate counter
			if(!equalTag(tag, i))  result = false;
			
		}
	}
	if(result) valid_for++;
	else 	   valid_for=0;
	//else if(valid_for >= 0) valid_for=0;
	d_printTagStore();
	return result;
}

void 
Fuzzy::storeTag(int *tag)
{
	int index = (used_frame_count % TAG_HISTORY_SIZE)*TAG_LENGTH;
	for(int i = 0; i < TAG_LENGTH; i++) tag_backstore[index+i] =  tag[i];
}

bool 
Fuzzy::equalTag(int *tag, int backindex)
{
	/*assert(tagindex <= TAG_HISTORY_SIZE);
	int storeindex = tagindex-- > 0 ? tagindex : TAG_HISTORY_SIZE + tagindex + 1;
	int index = storeindex * TAG_LENGTH;	
	*/
	assert(backindex <= TAG_HISTORY_SIZE);
	int index = ( (used_frame_count-backindex) % TAG_HISTORY_SIZE)*TAG_LENGTH;
	printf("%d -> %d (%d) : ", backindex, index , used_frame_count);
	for(int i = 0; i < TAG_LENGTH; i++) { 
		printf("%d=%d ", tag_backstore[index+i],  tag[i]);
		if(tag_backstore[index+i] !=  tag[i]) {
			printf(" != \n");
			d_printTagStore();
			return false;
		}
	}
	printf(" == \n");
	return true;
}

bool 
Fuzzy::equalTag(int backindex1, int backindex2)
{
	assert(backindex1 <= TAG_HISTORY_SIZE);
	assert(backindex2 <= TAG_HISTORY_SIZE);
	int index1 = (used_frame_count-backindex1 % TAG_HISTORY_SIZE)*TAG_LENGTH;
	int index2 = (used_frame_count-backindex2 % TAG_HISTORY_SIZE)*TAG_LENGTH;
	for(int i = 0; i < TAG_LENGTH; i++) { 
		if(tag_backstore[index1+i] != tag_backstore[index2+i]  ) return false;
	}
	return true;
}


bool 
Fuzzy::allValidTag(int *tag)    //Truth Degree 1.0 (12)
{
	int valid = 0;
	for(int i=0; i<TAG_LENGTH; i++) { if(tag[i] >= 0) valid++; }
	if(valid == TAG_LENGTH) return true;
	return false;
}

bool 
Fuzzy::closeToValidTag(int *tag)//Truth Degree 0.9  (11)
{
	int valid = 0;
	for(int i=0; i<TAG_LENGTH; i++) { if(tag[i] >= 0) valid++; }
	if(valid == TAG_LENGTH-1) return true;
	return false;
}

bool 
Fuzzy::barelyValidTag(int *tag) //Truth Degree 0.7-0.8 (6-10)
{
	int valid = 0;
	for(int i=0; i<TAG_LENGTH; i++) { if(tag[i] >= 0) valid++; }
	if(valid > (TAG_LENGTH/2)+1 || valid < TAG_LENGTH-1) return true;
	return false;
}

bool 
Fuzzy::inValidTag(int *tag)     //Truth Degree 0.1-0.6 (1-5)
{
	int valid = 0;
	for(int i=0; i<TAG_LENGTH; i++) { if(tag[i] >= 0) valid++; }
	if(valid > 0 && valid <= (TAG_LENGTH/2)) return true;
	return false;
}

bool 
Fuzzy::allInValidTag(int *tag)  //Truth Degree 0.0   (0)
{
	int valid = 0;
	for(int i=0; i<TAG_LENGTH; i++) { if(tag[i] >= 0) valid++; }
	if(valid == 0) return true;
	return false;
}

//fuzzy sizing of the video source to ideal window width range
//with priority given to squares of 2 sizing if possible
//result sizes will be rounded to next multiple of two
//very large sized video sources will be just scaled to fit

void
Fuzzy::sizeFrame(int *resx, int* resy, int wx, int wy, int vx, int vy, int mode)
{
    if(debug) printf("v %d,%d ->  w %d,%d ", vx, vy, wx, wy);
    int w = vx > vy ? wx : wy;
    int v = vx > vy ? vx : vy;
    if( v <= w ) {
        *resx = wx;
        *resy = wy;
   	   	if(debug) printf(" = %d,%d (mode=%d)\n", *resx, *resy, mode);
        return;
    }
	if(mode = 0){
		if(v/2 >= w) {
			*resx = vx/2;
	        *resy = vy/2;
   	     	if(debug) printf(" = %d,%d (mode=%d)\n", *resx, *resy, mode);
        	return;
		}
	}
    int f = (int)((float)v/(float)w);
    if(f > 8) { //too big
        if(f%2) f++;
    }else{
        f = 16;
        while(v/f < w) f = f/2;
    }
    // f2 larger  bigger than w size of v
    // f1 smaller smaller than w size of v
    // f2 is preffered,
    // unless f1 is closer to w than half of f2 closeness to w
    int d1 =  vx > vy ? vx/f : vy/f ;
    int d2 =  vx > vy ? vx/(f*2) : vy/(f*2) ;
    f = abs(w-d2) < (abs(w-d1)/2) ? f*2 : f;

    *resx = vx/f;
    *resy = vy/f;

    if(*resx%2) *resx = (*resx)+1;
    if(*resy%2) *resy = (*resy)+1;
   	if(debug) printf(" = %d,%d (mode=%d)\n", *resx, *resy, mode);
}

void
Fuzzy::d_printTagStore()
{
	//if(!debug) return;

	int top = (used_frame_count % TAG_HISTORY_SIZE)*TAG_LENGTH;
	int index = 0;
	printf("[------- %d -------\n", used_frame_count);
	for(int i=0; i< TAG_HISTORY_SIZE; i++){
		printf("%d ", i);
		for(int j = 0; j< TAG_LENGTH; j++) printf("%d", tag_backstore[index++]);
		if(i*TAG_LENGTH == top) printf(" *");
		printf("\n");
	}
	printf("------- %d -------]\n", used_frame_count);
}

