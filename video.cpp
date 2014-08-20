/*
 * DirectShow Video Processing is Setup  in "Video"
 * (Frame Filtering/Rendering is done in "Frame")
 * 
 * Video::initDShow Create graph and control
 * 
 *     FilterGraph (graph)
 *     MediaControl
 * 
 * Video::initFrameFilter | Create Frame filter and pin
 * 
 *     Create FrameFilter (frame)  [+++]
 *     Select pin (renderpin) 
 *     Add to graph
 * 
 * Video::initCameraSource Create Video Source filter and pin
 * 
 *     Enumerate Video Input Devices (cameras)
 *     Select camera device
 *     Bind camera device to BaseFilter
 *     Enumerate camera pins in BaseFilter
 *     Select pin (camerapin)
 * 
 *     Set notify Window for all media events [***]
 * 
 *    Connect camerapin to renderpin in the grpah
 * 
 * Video::handleGraphEvent Video graph events
 * 
 *     [***] The Media Event Goes to Windows event loop
 *     Where its checked for by WM_GRAPHNOTIFY
 *     And gets handled back in Video::handleGraphEvent
 * 
 * 
 *    [+++] Frame filter is based on CBaseVideoRenderer
 *  
 */
#include "video.h" 
#include "guicon.h"
#include "resource.h"

#define REGISTER_FILTERGRAPH

Video::Video(HWND _hwnd, Config *_config)
{
	config = _config;
	hwnd   = _hwnd;
	frame  = NULL;
	decoder = new Decoder();
	debug = false;
	if(config != NULL) debug  = config->WIN_DEBUG;
}


Video::~Video()
{
	if(decoder!= NULL) delete decoder;
	if(d_videofile != NULL) delete [] d_videofile;
}

/*
 * TODO : Redo with cleaner/better status check  
 * 
 * VIDEO_STATUS <  0 Video not Initilaized
 * VIDEO_STATUS == 0 Video OK
 * VIDEO_STATUS >  0 Video Initilaised, and then had Error
 */
int 
Video::getStatus()
{
	return VIDEO_STATUS;
}

bool 
Video::isPlaying()
{
	return (getStatus() == 0) ? true : false;
}

void
Video::setConfig(Config *_config)
{
    config = _config;
	debug  = config->WIN_DEBUG;
}

void
Video::showVideoAbort()
{
    MessageBox( hwnd,
		"Lost connection to the video Source.\n"
 		"The Mytago Technology Demo will not work without video.\n\n"
		"Restart application and try again when video source is available",
		"Lost Video",
		MB_OK | MB_ICONWARNING | MB_TASKMODAL );
}

void 
Video::paintFrame()
{
	frame->paintFrame();
}

void 
Video::handleGraphEvent()
{
    if (pevent == NULL) return;
    long evCode;
    LONG_PTR param1, param2;

    while (SUCCEEDED(pevent->GetEvent(&evCode, &param1, &param2, 0)))
    {
        pevent->FreeEventParams(evCode, param1, param2);
        switch (evCode)
        {
      		case EC_COMPLETE: {
				if(debug) printf("video::handleGraphEvent() EC_COMPLETE \n");
				VIDEO_STATUS = 1;
				frame->paintVideoFinish();
				showVideoAbort();
			} break;
       	 	case EC_USERABORT: {
				if(debug) printf("video::handleGraphEvent() EC_USERABORT \n");
				VIDEO_STATUS = 2;
				frame->paintVideoFinish();
				showVideoAbort();
			} break;
       		case EC_ERRORABORT: {
				VIDEO_STATUS = 3;
				if(debug) printf("video::handleGraphEvent() EC_ERRORABORT \n");
				frame->paintVideoFinish();
				showVideoAbort();
           		PostQuitMessage(0);
           		return;
			} break;
      	}
	} 
}

void 
Video::cleanupOnExit()
{
	if(!stopVideo()) { 
		if(debug) printf("Video::cleanupOnExit() : Failed stopping video properly\n");
	}
	cleanupInterfaces();
}

/* 
 * NOTE:  
 * Smart COM pointers are recommneded, but to keep this 
 * a Win32 only app, not linking to ATL libs to use smart pointers.
 */
void 
Video::cleanupInterfaces()
{
	if(cameraPin) 		{ cameraPin->Release();	 				 cameraPin = NULL; }
	if(renderPin) 		{ renderPin->Release();					 renderPin  = NULL; }
	if(aviPin) 			{ aviPin->Release();					 aviPin = NULL; }
	if(cameraPins) 		{ cameraPins->Release();				 cameraPins = NULL; }
	if(mediaControl) 	{ mediaControl->Release();				 mediaControl = NULL; }
	if(aviBaseFilter) 	{ aviBaseFilter->Release();				 aviBaseFilter = NULL; }
	if(cameraBaseFilter){ cameraBaseFilter->Release();			 cameraBaseFilter = NULL; }
	if(devices) 		{ devices->Release();					 devices	 = NULL; }
	if(pevent) 			{ pevent->Release();					 pevent = NULL; }
	if(cameras) 		{ cameras->Release();					 cameras = NULL; }
	if(camera) 			{ camera->Release();					 camera = NULL; }
	if(mediaControl) 	{ mediaControl->Release(); 				 mediaControl = NULL; }
	if(d_graphedit && d_graphRegister) 
						{ d_removeGraphFromRot(d_graphRegister); d_graphRegister=NULL; }
	if(graph) 			{ graph->Release(); 					 graph = NULL; } //releasing "graph" releases "frame" also
	CoUninitialize();
}


char*
Video::d_getTestVideoPath()
{
    char appPath[MAX_PATH] = "";
	char filePath[MAX_PATH] = "";
    GetModuleFileName(0, appPath, sizeof(appPath) - 1);
    char *appDir = strrchr(appPath, '\\');
    if(appDir) *(appDir+1) = '\0';
    if(appDir) { 
    	if(debug) printf("Video::d_getTestVideoPath path %s\n", appPath);
		if(d_videofile != NULL) { delete [] d_videofile; d_videofile = NULL; } //multiple entry
		d_videofile = new char[strlen(appPath) + 1 + strlen(config->V_TESTFILE) + 1 ];	
		sprintf_s(d_videofile, strlen(appPath) + 1 + strlen(config->V_TESTFILE) + 1, "%s\%s", appPath, config->V_TESTFILE );
		if(debug) printf("Video::d_getTestVideoPath file %s\n", d_videofile);
		return d_videofile; 
	}
	return NULL;
}


void
Video::initVideoParams()
{
	pevent 			= NULL;  //IMediaEventEx
	graph 			= NULL;  //IGraphBuilder
	mediaControl 	= NULL;  //IMediaControl
	devices 		= NULL;  //ICreateDevEnum
	cameras 		= NULL;  //IEnumMoniker
	camera 			= NULL;  //IMoniker
	cameraBaseFilter = NULL; //IBaseFilter
	aviBaseFilter   = NULL;  //IBaseFilter
	cameraPins 		= NULL;  //IEnumPins
	cameraPin 		= NULL;	 //IPin
	renderPin 		= NULL;	 //IPin
	aviPin 			= NULL;	 //IPin

	VIDEO_STATUS 	= -1;
	d_graphedit 	= false;
	d_graphRegister = NULL;

}

bool
Video::initDShow()
{

    hr = CoInitialize(0);
	if(FAILED(hr)) return cleanupFailedDShow();
    hr = CoCreateInstance( CLSID_FilterGraph, 0, CLSCTX_INPROC,IID_IGraphBuilder, (void **)&graph );
	if(FAILED(hr)) return cleanupFailedDShow();
    hr = graph->QueryInterface( IID_IMediaControl, (void **)&mediaControl );
	if(FAILED(hr)) return cleanupFailedDShow();
	return true;
}

bool
Video::cleanupFailedDShow()
{
	if(mediaControl) 	{ mediaControl->Release(); 	mediaControl = NULL; }
	if(graph) 			{ graph->Release(); 		graph = NULL; }
	//CoUninitialize(); //do it only once in cleanupInterfaces() as part of destruction 
	return false;
}

bool
Video::initFrameFilter()
{
    frame = new Frame(0, &hr); 
	if(FAILED(hr)) return cleanupFailedFrameFilter(); 
	frame->setWindow(hwnd);
	frame->setConfig(config);
	frame->setDecoder(decoder);

    hr = frame->FindPin(L"In", &renderPin);
	if(FAILED(hr)) return cleanupFailedFrameFilter();
    hr = graph->AddFilter((IBaseFilter*)frame, L"Frame Filter");
	if(FAILED(hr)) return cleanupFailedFrameFilter();
	return true;
}

bool
Video::cleanupFailedFrameFilter()
{
	if(renderPin) { renderPin->Release(); renderPin = NULL; }
	if(frame)	  { frame->Release();     frame = NULL; }
	return false;
}

bool
Video::initSource()
{
	bool result = false;
	if(config->V_D_SKIP_CAM){
		result = d_initAVISource();
	}else{
		result = initCameraSource();
		if(!result) result = d_initAVISource();
	}
	return result;
}

bool
Video::initCameraSource()
{
    hr = CoCreateInstance (CLSID_SystemDeviceEnum, 0, CLSCTX_INPROC, IID_ICreateDevEnum, (void **) &devices);
	if(FAILED(hr))  return cleanupFailedCaptureSource();
	hr = devices->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &cameras, 0);
	if( hr != S_OK ){ 
		VIDEO_STATUS = -2;
		return cleanupFailedCaptureSource();
	}

	VIDEO_STATUS = -3; //preset untill succeeds
    hr = cameras->Next (1, &camera, 0);
	if(FAILED(hr)) return cleanupFailedCaptureSource();
	hr = camera->BindToObject(NULL,NULL,IID_IBaseFilter, (void**)&cameraBaseFilter);
	if(FAILED(hr)) return cleanupFailedCaptureSource();
   	hr = cameraBaseFilter->EnumPins(&cameraPins); 
	if(FAILED(hr)) return cleanupFailedCaptureSource();

	hr = cameraPins->Next(1,&cameraPin, 0);
	if(SUCCEEDED(hr))  {
   		hr = graph->AddFilter(cameraBaseFilter, L"Capture Source"); 
		if(FAILED(hr)) return cleanupFailedCaptureSource();
	}
	if( cameraPin ){
		hr = graph->QueryInterface(IID_IMediaEventEx, (void **)&pevent);
		if(FAILED(hr)) return cleanupFailedCaptureSource();
		pevent->SetNotifyWindow((OAHWND)hwnd, WM_GRAPHNOTIFY, 0);
		hr = graph->Connect(cameraPin , renderPin);
		if(SUCCEEDED(hr)){  
			//free all we can, rest will be done at cleanuponexit()
			if(cameraPins) 	{ cameraPins->Release();	cameraPins = NULL; } 
			if(devices) 	{ devices->Release();		devices	 = NULL; }
			return true;
		}
	}
	return cleanupFailedCaptureSource();
}

bool
Video::cleanupFailedCaptureSource()
{
		if(pevent)  			{ pevent->Release();			pevent = NULL; }
		if(cameraPin)  			{ cameraPin->Release(); 		cameraPin = NULL; }
		if(cameraPins)  		{ cameraPins->Release();		cameraPins = NULL; }
		if(cameraBaseFilter)	{ cameraBaseFilter->Release(); 	cameraBaseFilter = NULL; }
		if(camera)  			{ camera->Release(); 			camera = NULL; }
		if(cameras) 			{ cameras->Release();			cameras = NULL; }
		if(devices) 			{ devices->Release(); 			devices = NULL; }
		return false;
}

bool
Video::d_initAVISource()
{
	if(!config->V_D_TESTING) return false;
	//hr = graph->AddSourceFilter ((LPCWSTR)config->V_TESTFILE, L"File Source", &aviBaseFilter);
	//hr = graph->AddSourceFilter ((LPCWSTR)d_getTestVideoPath(), L"File Source", &aviBaseFilter);
	hr = graph->AddSourceFilter (L"C:\\CODE\\WEBCAM\\test.avi", L"File Source", &aviBaseFilter);
	if(aviBaseFilter){
		hr = graph->QueryInterface(IID_IMediaEventEx, (void **)&pevent);
		if(FAILED(hr)) return d_cleanupFailedAVISource();
		pevent->SetNotifyWindow((OAHWND)hwnd, WM_GRAPHNOTIFY, 0);

  		hr = aviBaseFilter->FindPin(L"Output", &aviPin);
		if(FAILED(hr)) return d_cleanupFailedAVISource();

		hr = graph->Connect(aviPin, renderPin);
		if(SUCCEEDED(hr)) return true;
	}
	return d_cleanupFailedAVISource();
}

bool
Video::d_cleanupFailedAVISource()
{
	if(aviPin)			{ aviPin->Release(); aviPin = NULL; }
	if(pevent)			{ pevent->Release(); pevent = NULL; }
	if(aviBaseFilter) { aviBaseFilter->Release(); aviBaseFilter = NULL; }
	return false;
}


bool
Video::startVideo()
{
	if(VIDEO_STATUS < 0 ) return false;

	     if(cameraPin) 	hr = graph->Render(cameraPin);
	else if(aviPin) 	hr = graph->Render(aviPin);
	else 				return false;
	//if(FAILED(hr)) return false; //FIXME fails for AVI but not Video

	hr = mediaControl->Run();
	if(SUCCEEDED(hr)) return true;

	VIDEO_STATUS = -3;
	return false;
}


bool
Video::stopVideo()
{
	if(VIDEO_STATUS < 0 ) { 
		if(debug) printf("Video::stopVideo Video was not initialized properly\n");
		return false;
	}

	while(frame->waitforProcessing()) { 
		if(debug) printf("Video::stopVideo Waiting for OK from frame processing\n");
	}
	//frame->Stop();
	
	if(debug) printf("Video::stopVideo Enter StopWhenReady\n");
	hr = mediaControl->StopWhenReady();
	if(debug) printf("Video::stopVideo Exit StopWhenReady\n");
	return (SUCCEEDED(hr)) ? true :  false;
}

void 
Video::exitVideo()
{
	VIDEO_STATUS = 4;
	cleanupOnExit();	
}

bool 
Video::initVideo()
{

	d_videofile = NULL;
	initVideoParams();
	bool result = false;
	result = initDShow();
	if(result)  result = initFrameFilter();
	if(result)  result = initSource();
	if(result) VIDEO_STATUS = 0;
	if(result && d_graphedit ) { 
		hr = d_addGraphToRot(graph, &d_graphRegister);
		if(FAILED(hr)) d_graphRegister = NULL;	
	}
	
	return startVideo();
}

void 
Video::loopVideo()
{
	if(VIDEO_STATUS == 1 ) {
		VIDEO_STATUS = startVideo() ? 0 : -6 ;
	}
}

HRESULT 
Video::d_addGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister)
{
	IMoniker * pMoniker;
	IRunningObjectTable *pROT;
	WCHAR wsz[128];
	HRESULT hr;

	if (FAILED(GetRunningObjectTable(0, &pROT)))
		return E_FAIL;

	if(debug) printf("Video::d_addGraphToRot FilterGraph %08x pid %08x \n", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());

	hr = CreateItemMoniker(L"!", wsz, &pMoniker);
	if (SUCCEEDED(hr)) {
		hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
		pMoniker->Release();
	}

	pROT->Release();
	return hr;
}

void 
Video::d_removeGraphFromRot(DWORD pdwRegister)
{
	IRunningObjectTable *pROT;

	if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
		pROT->Revoke(pdwRegister);
	pROT->Release();
	}
}

