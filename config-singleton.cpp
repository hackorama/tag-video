#include "config.h"

Config* Config::instance = NULL;

const char Config::V_TESTFILE[9] = "test.avi";

Config::Config()
  : TAG_LENGTH(12),
	MAX_ANCHORS(12),
	MAX_SHAPES(48),
	PLATFORM_CPP(false),
	PLATFORM_CPP_MAGICK(false),
	PLATFORM_CPP_SYMBIAN(false),
	PLATFORM_CPP_SYMBIAN_S60(false)
{
	LOGGING = false;

	THREADS = 1; //single threaded by default

	THRESHOLD_WINDOW_SIZE = 48;
	THRESHOLD_OFFSET = 10;
	THRESHOLD_RGB_FACTOR = 1; //JPEG=1 IMAGEMAGIC=256 JSE=1 JME=1 DIRECTSHOW=1

	WIN_DSHOW_MAXRGB = 256;
	WIN_DSHOW_COLORS = 3;

	PIXMAP_SCALE_SIZE = 320;   //must be > THRESHOLD_WINDOW_SIZE
	PIXMAP_NATIVE_SCALE = false; //defaults to false,
	PIXMAP_FAST_SCALE = true;  //defaults to true, not effective when PIXMAP_NATIVE_SCALE == true

	JPG_SCALE = true; //defaults to true

	ANCHOR_BOX_FLEX_PERCENT = 30;
	SHAPE_BOX_FLEX_PERCENT = 30;

	GRID_WIDTH = 0;
	GRID_HEIGHT = 0;

	PESSIMISTIC_ROTATION = true;  //defaults to true

	V_WIN_WIDTH = 280;
	V_WIN_HEIGHT = 280;
	V_WIN_BORDER = 4;
	V_MSG_STRIP_SIZE = 30;
	V_FRAME_DELAY = 0;
	V_SKIP_FRAMES = 10;
	V_GRID_WIDTH = 0;
	V_GRID_HEIGHT = 0;
	V_TESTING = true;


	DBGPIXMAP = NULL;

	PIXBUF = NULL;
	EDGE_MAP = NULL;
	V_MAP = NULL;

	TAG_IMAGE_FILE = "";

	TAG_DEBUG     = false;
	VISUAL_DEBUG  = false;
	ANCHOR_DEBUG  = false;

	ARGS_OK = false;
}

Config::~Config() //singleton destructor will be called no need for "delete config;"
{
	if(DBGPIXMAP != NULL) delete	DBGPIXMAP;
	if(PIXBUF != NULL)    delete [] PIXBUF;
	if(EDGE_MAP != NULL)  delete [] EDGE_MAP;
	if(V_MAP != NULL)	  delete []	V_MAP;
	//if(instance != NULL)  delete    instance; //FIXME : verify sigleton deletion without leaks
}

Config*
Config::getInstance()
{
	if(instance == NULL) instance = new Config();
	return instance;
}

void
Config::freePixbuf() //free from border 
{
	if(PIXBUF != NULL) { 
		delete [] PIXBUF;
		PIXBUF = NULL;
	}
}

void
Config::freeEdgemap() //free from border 
{
	if(EDGE_MAP != NULL) { 
		delete [] EDGE_MAP;
		EDGE_MAP = NULL;
	}
}


bool
Config::CHECK_VISUAL_DEBUG()
{
	if(DBGPIXMAP == NULL) return false;
	return VISUAL_DEBUG;
}

void
Config::setDebugPixmap(Pixmap* _pixmap)
{
	//DBGPIXMAP = _pixmap;
}

bool
Config::checkArgs(int argc, char **argv)
{
	if( argc < 2 ) {
		cerr << endl;
		cerr << "Usage:" << endl;
		cerr << "\t" << argv[0] << " imagefile.jpg [thread count] [l|v|d|t] [threshold]" << endl ;
		cerr << "\t\t\t[scaletype] [scalesize] [windowsize]" << endl;
		cerr << endl;
		cerr << "\tl: debug log" << endl ;
		cerr << "\tv: visual debug" << endl;
		cerr << "\td: debug log and visual debug" << endl;
		cerr << "\tt: performance data" << endl;
		cerr << "\tscaletype: 1 = slower more accurate" << endl;
		cerr << "\tscaletype: 2 = native image lib scale" << endl;
		cerr << "\tscaletype: Default is fast scale" << endl;
		cerr << endl;
		return false;
	}

	TAG_IMAGE_FILE = argv[1];

	if(argc >= 3)                         THREADS               = atoi(argv[2]);
	if(argc >= 4){
		string option = string(argv[3]);
		if( option == string("l") ){
			TAG_DEBUG = true;
			cout << "Debug Logging enabled" << endl ;
		}
		if( option == string("v") ){
			VISUAL_DEBUG = true;
			cout << "Visual Debug enabled" << endl ;
		}
		if( option == string("a") ){
			ANCHOR_DEBUG = true;
			cout << "Anchor Debug enabled" << endl ;
		}
		if( option == string("d") ){
			TAG_DEBUG = true;
			cout << "Debug Logging enabled" << endl ;
			VISUAL_DEBUG = true;
			cout << "Visual Debug enabled" << endl ;
		}
	}
	int type = 0;
	if(argc >= 5)                         THRESHOLD_OFFSET      = atoi(argv[4]);
	if(argc >= 6)                         type                  = atoi(argv[5]);
	if(argc >= 7) { if(atoi(argv[6]) > 0) PIXMAP_SCALE_SIZE     = atoi(argv[6]); }
	if(argc >= 8) { if(atoi(argv[7]) > 0) THRESHOLD_WINDOW_SIZE = atoi(argv[7]); }
	if(type == 2) PIXMAP_NATIVE_SCALE = true;
	if(type == 1) PIXMAP_FAST_SCALE   = false;

	ARGS_OK = true;

	return true;

}
