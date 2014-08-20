#ifndef _FRAME_H_INCLUDED
#define _FRAME_H_INCLUDED

#include <windows.h>
#include <commctrl.h>
#include <dshow.h>
#include <streams.h>
#include <psapi.h>
#include "decoder.h"
#include "config.h"
#include "resource.h" //get statsusbar id for updating messages

#define WM_GRAPHNOTIFY  WM_APP + 1
struct __declspec(  uuid("{71771540-2017-11cf-ae26-0020afd79767}")  ) CLSID_Frame;

class Frame : public CBaseVideoRenderer
{
public:
    Frame( IUnknown* unk, HRESULT *hr );
    ~Frame();

	void setWindow(HWND hwnd);
	void setConfig(Config *config);
	void setDecoder(Decoder *decoder);
	void paintFrame();
	void paintVideoFinish();
	bool waitforProcessing();

private:
	int pixelbytes;
	int  v_width, v_height;
	int  v_display_width, v_display_height;
	int  w_width, w_height;
	int  c_width, c_height;
	BYTE* data;
	int FRAMEDELAY;
	Decoder *decoder;
	Config *config;
	int counter;
	int  tag[12];
	char tag_string[16];
	char last_tag_string[16];
	char msg_string[256];
	int  last_tag_age;
	int  min_memory;
	bool debug;

	bool FRAME_PROCESSING;
    bool FRAME_DECODING;
    bool STOP_REQUESTED;
    int  STOP_REQUESTED_COUNTER;
    const int  STOP_REQUESTED_COUNTER_MAX;
	const int  STOP_REQUESTED_WAIT_MILLISECS;

	HWND hwnd;
	HWND hstatus;
	VIDEOINFO* vi;
	BITMAPINFO bmi;
	BITMAPINFOHEADER bmih;
	HFONT font;
	LOGFONT tag_font;
	HBRUSH white_brush;
	RECT   font_rect;
	PROCESS_MEMORY_COUNTERS pmc;

	void showMsg(HDC dc, LPCSTR text);
	void    processFrame();
	void    initResources();
	void    releaseResources();
    HRESULT CheckMediaType(const CMediaType *media );
    HRESULT DoRenderSample(IMediaSample *sample);
    HRESULT ShouldDrawSampleNow(IMediaSample *sample, 
								REFERENCE_TIME *start,
								REFERENCE_TIME *stop);
	void paintFrame(char* text, bool newtag, bool newage);
	void paintFrame(bool newtag, bool newage);
	void statusMessage(char* msg, int part);

	void d_videoAlignmentTest();
};

#endif /* _FRAME_H_INCLUDED */

