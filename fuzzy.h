#ifndef _FUZZY_H_INCLUDED
#define _FUZZY_H_INCLUDED

#include "common.h"

/* 
  I am Fuzzy

  My objective is to minimize CPU utilization 
  by  analyze the video frames and only do 
  tag scanning when there is a new tag 
  to be scanned in the video frames stream.

  I also confirm tag scan values, by caching
  tag scan results and weeding out false positives 

 */

class Fuzzy 
{
public:
    Fuzzy();
    ~Fuzzy();

	bool selectFrame(int skip_frames);
	bool trackFrame();
	bool motionDetect(unsigned char *frame_data, int frame_width, int frame_height);
	bool validateTag(int *tag);
	void countFrame();

	void sizeFrame(int *resx, int* resy, int wx, int wy, int vx, int vy, int  mode);

	//fuzzy tag aging
	bool tagIsBrandNew();
	bool tagIsNew();
	bool tagIsOld();
	bool tagIsVeryOld();
	bool tagIsActive(); //used for markCode() duration
	void setDebug(bool debug);

	int valid_for;

private:
	//predefined sizes : tweak based on obseravation 
	const int MD_WIN_WIDTH;
	const int MD_WIN_HEIGHT;
	const int MD_THRESHOLD_PERCENT;
	const int MD_HISTORY_SIZE;
	const int MD_OFFSET;
	const int TAG_HISTORY_SIZE;
	const int TAG_LENGTH;
	const int OLD_TAG;
	const int VERY_OLD_TAG;
	const int VALID_TAG_REPEATS;
	const int TAG_ACTIVE_FOR;

	long real_frame_count;
	long used_frame_count;
	bool *md_lastframe; 	//size MD_WIN_WIDTH * MD_WIN_HEIGHT
	int  *md_frame_deltas; 	//size MD_HISTORY_SIZE
	int  *tag_backstore; 	//size TAG_HISTORY_SIZE * TAG_LENGTH

	int md_win_x1;
	int md_win_x2;
	int md_win_y1;
	int md_win_y2;

	int md_index;
	int tag_index;

	int last_frame_width;
	int last_frame_height;

	int md_pixel_threshold;

	bool debug;
	bool d_videodebug;

	void checkFrameStores(int frame_width, int frame_height);	
	int  computeDelta(unsigned char *frame_data, int frame_width, int frame_height);
	void calculateMDWindow(int frame_width, int frame_height);

	//fuzzy tag validation
	bool allValidTag(int *tag);    //1
	bool closeToValidTag(int *tag);//0.9
	bool barelyValidTag(int *tag); //0.7-0.8
	bool inValidTag(int *tag);	   //0.1-0.6
	bool allInValidTag(int *tag);  //0.0

	void storeTag(int *tag);
	bool equalTag(int *tag, int index);
	bool equalTag(int index1, int index2);

	void d_printTagStore();
	
};

#endif /* _FUZZY_H_INCLUDED */

