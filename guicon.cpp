/* 
    Based on Code Snippet From:
	http://www.halcyon.com/~ast/dload/guicon.htm
	Adding Console I/O to a Win32 GUI App - Windows Developer Journal, December 1997
 	ANDREW S. TUCKER ast@halcyon.com http://www.halcyon.com/ast  
 */

#include "guicon.h"

#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

#ifndef _USE_OLD_IOSTREAMS
using namespace std;
#endif

//maximum mumber of lines the output console should have
static const WORD MAX_CONSOLE_LINES = 5000;

FILE *fp;

void closeRedirectIO()
{
	if(fp != NULL) fclose(fp);
}

void redirectIO(char *filename)
{
	if(filename == NULL ){ 
		toConsole();
		return;
	}

	fp = fopen(filename, "w");
	//if( fopen_s( &fp, filename, "w" ) == 0 )  { 
		*stdout = *fp;
		setvbuf( stdout, NULL, _IONBF, 0 );
		*stderr = *fp;
		setvbuf( stderr, NULL, _IONBF, 0 );
	//}
}
	
void toConsole()
{
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
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

void setPath()
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

