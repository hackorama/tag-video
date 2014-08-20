#ifndef _VIDEO_H_INCLUDED
#define _VIDEO_H_INCLUDED

#include <windows.h>
#include <dshow.h>
#include "frame.h"
#include "config.h"

class Video
{

public:
	Video(HWND hwnd, Config *config);
	~Video();

	Frame *frame; //FIXME make private, provide accessors 

	void handleGraphEvent();
	int  getStatus();
	bool isPlaying();
	bool initVideo();
	void exitVideo();
	void loopVideo();
	void paintFrame();

private:
	HWND hwnd; 
	Config *config;
	Decoder *decoder;
	HRESULT hr;
	BITMAPINFOHEADER bmih; 
	int VIDEO_STATUS;
	bool debug;
	char *d_videofile;
	bool d_graphedit;
	DWORD d_graphRegister;

    IGraphBuilder*  graph;
    IMediaControl*  mediaControl;
    ICreateDevEnum* devices;
    IEnumMoniker*   cameras;
    IMoniker*       camera;
    IBaseFilter*    aviBaseFilter;
    IEnumPins*      cameraPins;
    IBaseFilter*    cameraBaseFilter;
    IPin*           cameraPin;
    IPin*           renderPin;
    IPin*           aviPin;

	IMediaEventEx *pevent;

	void setConfig(Config *_config);
	void initVideoParams();
	bool initDShow();
	bool cleanupFailedDShow();
	bool initFrameFilter();
	bool cleanupFailedFrameFilter();
	bool initSource();
	bool initCameraSource();
	bool cleanupFailedCaptureSource();
	void cleanupInterfaces();
	bool startVideo();
	bool stopVideo();
	void cleanupOnExit();

	void showVideoAbort();

	//debug and test only 
	bool d_initAVISource();
	bool d_cleanupFailedAVISource();
	char* d_getTestVideoPath();
	HRESULT d_addGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
	void d_removeGraphFromRot(DWORD pdwRegister);

};

#endif /* _VIDEO_H_INCLUDED */
