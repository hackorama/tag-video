#ifndef _CONFIG_H_INCLUDED
#define _CONFIG_H_INCLUDED

#include <string>

#include "common.h"
#include "config.h"
#include "pixmap.h"

class Config
{

public:
	~Config();
	static Config *getInstance(); //singleton

	static Config* instance;

	unsigned char	*PIXBUF;
	bool 			*EDGE_MAP; 		//border map created in Threshold parsed in Border
	Pixmap			*DBGPIXMAP;
	string 			TAG_IMAGE_FILE;	//image filename 

	int THRESHOLD_WINDOW_SIZE; 	//Adapative thresholdng window size(lower the faster)
	int THRESHOLD_OFFSET;		//threshold offset adjustment
	int THRESHOLD_RGB_FACTOR;	//RGB range multiplication factor 

	int WIN_DSHOW_MAXRGB;
	int WIN_DSHOW_COLORS;

	int  PIXMAP_SCALE_SIZE;         //fix pixmap to this bounding box size 
	int  PIXMAP_MINIMUM_SCALE_SIZE; //minimum valid value for PIXMAP_SCALE_FACTOR
	bool PIXMAP_FAST_SCALE;         //scale by skipping(FAST) or by averaging(SLOW)
	bool PIXMAP_NATIVE_SCALE;  //scale using platform specific external libraray
	bool JPG_SCALE;			   //scale by 2/4/8 on IJG JPEG lib decompress 
	//NATIVE_SCALE requires no further scaling, JPG_SCALE may need further scaling

	int ANCHOR_BOX_FLEX_PERCENT;    //allowed flexibility for box width and height 
	int SHAPE_BOX_FLEX_PERCENT;	//allowed flexibility for box width and height

	int GRID_WIDTH;			//image width
	int GRID_HEIGHT;		//image height

	bool PESSIMISTIC_ROTATION;	//resizing the grid for rotated shapes

	int THREADS;

	int V_GRID_WIDTH;
	int V_GRID_HEIGHT;
	int  V_WIN_WIDTH;
	int  V_WIN_HEIGHT;
	int  V_WIN_BORDER;
	int  V_MSG_STRIP_SIZE;
	int  V_FRAME_DELAY;
	int  V_SKIP_FRAMES;
	bool *V_MAP; 		//tag map, initted in Decoder and filled up in Border
	bool V_TESTING;
	static const char V_TESTFILE[9];
	

	bool CHECK_VISUAL_DEBUG();
	void setDebugPixmap(Pixmap* pixmap);
	bool checkArgs(int argc, char **argv);
	void freeEdgemap();
	void freePixbuf();

	bool LOGGING;
	bool TAG_DEBUG;
	bool VISUAL_DEBUG;
	bool ANCHOR_DEBUG;

	bool ARGS_OK;

	const int TAG_LENGTH;
	const int MAX_ANCHORS;
	const int MAX_SHAPES;
	const bool PLATFORM_CPP;
	const bool PLATFORM_CPP_MAGICK;
	const bool PLATFORM_CPP_SYMBIAN;
	const bool PLATFORM_CPP_SYMBIAN_S60;

	/*
	static final boolean PLATFORM_JAVA = true;
	static final boolean PLATFORM_JAVA_ME = false;
	static final boolean PLATFORM_JAVA_SE = true;
	static final boolean PLATFORM_JAVA_ANDROID = false;
	*/

private: //singleton
	Config();

};

#endif /* _CONFIG_H_INCLUDED */
