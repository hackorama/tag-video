#include <windows.h>
#include "win.h"

int WINAPI WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show){
	Win *app = new Win(inst);
	if(app) delete app;
	return 0;
}
