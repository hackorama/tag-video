#include "win.h" 
#include "guicon.h"

const char Win::windowClassName[] = "VideoWindow";

Win::Win(HINSTANCE hInstance)
	: MSG_DELAY_MILLISECS(30)
	
{
	config = new Config();
	if(config->WIN_DEBUG) config->redirectIO();
	video = NULL;
	initWindow(hInstance);
}

Win::~Win()
{
	if(video != NULL ) delete video;
	if(config != NULL) delete config;
}

void 
Win::handleGraphEvent()
{
	video->handleGraphEvent();
}

int
Win::initVideo()
{
	assert(hwnd != NULL);
	video = new Video(hwnd, config);
	video->initVideo();
	return video->getStatus();
}

BOOL CALLBACK 
AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_COMMAND:
            EndDialog(hwnd, TRUE);
            return TRUE;
        case WM_INITDIALOG:
            return TRUE;
    }
    return FALSE;
}


//http://www.codeproject.com/KB/winsdk/win32windowwrapperclass.aspx
//http://www.codeproject.com/KB/winsdk/wndproctoclassproc.aspx
LRESULT CALLBACK 
Win::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Win* winptr = (Win*)GetWindowLongPtr(hwnd, GWLP_USERDATA); 
	switch(msg)
	{
		case WM_INITMENU: {
			EnableMenuItem((HMENU)wParam, ID_VIDEO_STOP,
					winptr->video->isPlaying() ? 
					MF_ENABLED : MF_GRAYED);
			EnableMenuItem((HMENU)wParam, ID_VIDEO_START,
					winptr->video->isPlaying() ? 
					MF_GRAYED : MF_ENABLED );
			CheckMenuItem((HMENU)wParam, ID_OPTIONS_LOG,
					winptr->config->LOGGING ?
					MF_CHECKED : MF_UNCHECKED);
		} break;

		case WM_COMMAND: {
			switch(LOWORD(wParam))
			{
				case ID_VIDEO_STOP:
					winptr->video->exitVideo();
				break;
				case ID_VIDEO_START:
					winptr->video->initVideo();
				break;
				case ID_OPTIONS_LOG:
					winptr->config->LOGGING = !winptr->config->LOGGING;
				break;
				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
				case ID_HELP_HELP:
					winptr->showAbout();
				break;
				case ID_ABOUT_HELP:
					DialogBox(0, MAKEINTRESOURCE(IDD_ABOUT), hwnd, (DLGPROC)AboutDlgProc);
				break;
			}
		} break;

		case WM_PAINT:
			winptr->video->paintFrame();
			return DefWindowProc(hwnd, msg, wParam, lParam);
		break;

		case WM_CLOSE:
			DestroyWindow(hwnd);
		break;

		case WM_DESTROY:
			winptr->video->exitVideo();
			PostQuitMessage(0);
		break;

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

void
Win::showAbout()
{
	MessageBox( hwnd,
				"This is a Technology Demo for Mytago Tag System\n"
				"Decoder V7, Video V3",	
                "About Mytago Technology Demo",
                MB_OK | MB_ICONINFORMATION | MB_TASKMODAL );
	
}

void
Win::showVideoFail()
{
	MessageBox( hwnd,
				"Sorry, we could not find/use a video source.\n"
				"The Mytago Technology Demo will not work without video.\n\n"
				"Please verify you have a working video source\n"	
				"like a webcam or digital video camera connected.\n\n"
				"If you have a working video source,\n" 
				"please check it's not being used by another application.",	
                "No Video Source",
                MB_OK | MB_ICONWARNING | MB_TASKMODAL );
}

void
Win::showVideoAbort()
{
	MessageBox( hwnd,
				"Lost connection to the video Source.\n"
				"The Mytago Technology Demo will not work without video.\n\n"
				"Application will exit, Try again when video source is available",	
				"Lost Video",
                MB_OK | MB_ICONWARNING | MB_TASKMODAL );
}


void
Win::initWindow(HINSTANCE hInst)
{
	//for statusbar
	INITCOMMONCONTROLSEX initctrls;
	initctrls.dwSize = sizeof( INITCOMMONCONTROLSEX );
	initctrls.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx( &initctrls );

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(long);
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON));
	wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 16, 16, 0);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = windowClassName;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MYMENU);
	RegisterClassEx(&wc);

    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, windowClassName, "Mytago Tech Demo",
						WS_OVERLAPPEDWINDOW&~WS_MAXIMIZEBOX&~WS_SIZEBOX,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
						CW_USEDEFAULT, 0, 0, 0, NULL); 


	SetWindowLongPtr(hwnd, GWLP_USERDATA, (long)this);
	int status = initVideo();
	
	if( status != 0 ) { 
			SetWindowPos(hwnd, 0, CW_USEDEFAULT, CW_USEDEFAULT, 
						config->V_WIN_WIDTH, config->V_WIN_HEIGHT, 
						SWP_NOZORDER|SWP_NOMOVE);	
			showVideoFail();
	}
    ShowWindow(hwnd,SW_SHOW);
	int statwidths[] = {120, -1}; 
	hstatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
                    WS_CHILD | WS_VISIBLE , 0, 0, 0, 0,
                    hwnd, (HMENU)IDC_MAIN_STATUS, GetModuleHandle(NULL), NULL);

	SendMessage(hstatus, SB_SETPARTS, sizeof(statwidths)/sizeof(int), (LPARAM)statwidths);
	if( status == 0 ) 
    	SendMessage(hstatus, SB_SETTEXT, 0, (LPARAM)"Ready to scan tags");
	else 
    	SendMessage(hstatus, SB_SETTEXT, 0, (LPARAM)"No video source");

	/*
	UpdateWindow(hwnd);
	while(GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if(msg.message == WM_GRAPHNOTIFY) handleGraphEvent();
	} 
	*/

    while ( msg.message != WM_QUIT) {
        if( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) ) {
            TranslateMessage( &msg );
            DispatchMessage(  &msg );
            if(msg.message == WM_KEYDOWN && msg.wParam==VK_ESCAPE ) break;
            if(msg.message == WM_GRAPHNOTIFY) handleGraphEvent();
        }
        Sleep(MSG_DELAY_MILLISECS);
    }
}
