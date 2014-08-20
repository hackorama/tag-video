#include "config.h"

/*Config* Config::instance = NULL;*/

const char Config::V_TESTFILE[9] = "test.avi";
const char Config::V_LOGFILE[8] = "log.txt";

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

	V_WIN_WIDTH = 320;
	V_WIN_HEIGHT = 320;
	V_WIN_BORDER = 4;
	V_MSG_STRIP_SIZE = 30;
	V_FRAME_DELAY = 0;
	V_SKIP_FRAMES = 4;
	V_GRID_WIDTH = 0;
	V_GRID_HEIGHT = 0;

	// [ All should be false for Production
	V_D_TESTING 	= false;
	V_D_SKIP_CAM 	= false;
	V_D_PIXDEBUG 	= false;
	V_CONSOLE_ONLY 	= false;

	TAG_DEBUG       = false;
	WIN_DEBUG     	= false;
	WIN_PERF_LOG   	= true;
	VISUAL_DEBUG  	= false;
	ANCHOR_DEBUG  	= false;
	//   All should be false for Production ]

	DBGPIXMAP = NULL;
	PIXBUF    = NULL;
	EDGE_MAP  = NULL;
	V_MAP     = NULL;

	TAG_IMAGE_FILE = "";
	ARGS_OK = false;
	fp = NULL;

	FUZZY = new Fuzzy();

#ifdef _WINDOWS
	if(WIN_DEBUG) redirectIO();
#endif /*_WINDOWS */

}

Config::~Config() 
{
#ifdef _WINDOWS
	closeRedirectIO();
#endif /*_WINDOWS */
	if(DBGPIXMAP != NULL) delete	DBGPIXMAP;
	if(PIXBUF != NULL)    delete [] PIXBUF;
	if(EDGE_MAP != NULL)  delete [] EDGE_MAP;
	if(V_MAP != NULL)	  delete []	V_MAP;
	if(FUZZY != NULL) 	  delete	FUZZY;
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

#ifdef _WINDOWS
/* Based on Code Snippet From:
 * http://www.halcyon.com/~ast/dload/guicon.htm
 * Adding Console I/O to a Win32 GUI App - Windows Developer Journal, December 1997
 * ANDREW S. TUCKER ast@halcyon.com http://www.halcyon.com/ast  
 */
void
Config::closeRedirectIO()
{
	if(fp != NULL) fclose(fp);
}

void 
Config::redirectIO()
{
	if(V_CONSOLE_ONLY){ 
		toConsole();
		return;
	}

	if( fopen_s( &fp, V_LOGFILE, "w" ) == 0 )  { 
		*stdout = *fp;
		setvbuf( stdout, NULL, _IONBF, 0 );
		*stderr = *fp;
		setvbuf( stderr, NULL, _IONBF, 0 );
	}
}
	
void 
Config::toConsole()
{
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	//maximum mumber of lines the output console should have
	coninfo.dwSize.Y = 5000;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );

	// redirect unbuffered STDIN to the console
	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "r" );
	*stdin = *fp;
	setvbuf( stdin, NULL, _IONBF, 0 );

	// redirect unbuffered STDERR to the console
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stderr = *fp;

	setvbuf( stderr, NULL, _IONBF, 0 );
	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	ios::sync_with_stdio();
}

void 
Config::setPath()
{
    char appPath[MAX_PATH] = "";
    GetModuleFileName(0, appPath, sizeof(appPath) - 1);
    fprintf(stdout, "PWD %s\n", appPath);
    char *appDir = strrchr(appPath, '\\');
    if(appDir) ++appDir;
    if(appDir) *appDir = 0;
    if(appDir) fprintf(stdout, "%s\n", appPath);
    if(appDir) SetCurrentDirectory(appPath);
}

#endif /* _WINDOWS */
