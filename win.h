#ifndef _WIN_H_INCLUDED
#define _WIN_H_INCLUDED

#include <windows.h>
#include <commctrl.h>
#include <dshow.h>
#include "resource.h"
#include "video.h"
#include "config.h"

#define WM_GRAPHNOTIFY  WM_APP + 1

class Win
{

public:
	Win(HINSTANCE hInstance);
	~Win();

	void initWindow(HINSTANCE hInstance);
	void showVideoAbort();
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	Video *video;

private:
	Config *config;
	HRESULT hr;
	HWND hwnd; 
	HWND hstatus; 
	WNDCLASSEX wc;
	MSG msg; 
	LPCWSTR filename;
	const int MSG_DELAY_MILLISECS;
	static const char windowClassName[];

	void handleGraphEvent();
	int  initVideo();
	void showVideoFail();
	void showAbout();
};

#endif /* _WIN_H_INCLUDED */
