/*
 * DirecctShow Frame Filtering/Rendering is done in "Frame"
 * 
 * Frame::CheckMediaType Video Media Dimensions and Type
 * 
 *    Frame -> CBaseVideoRenderer -> CBaseRenderer::CheckMediaType
 *    Called from CRendererInputPin::CheckMediaType during graph connection or render pin
 *
 *   [###] Render pin found at frame -> IBaseFiletr::FindPin in Video::initCameraSource
 *    Once we get the media type and video dimension calculate and set Window dimensions
 *
 *    Frame -> CBaseVideoRenderer -> CBaseRenderer::DoRenderSample
 *    Is called whenever there is a sample frame ready to render
 *
 *    Store the frame data pointer to Frame::data
 *
 *    Process the frame data in Frame::processFrame
 *
 *    Then call StretchDIBits to draw the video frame data on the Window DC
 *    Before that calls Frame::paintFrame to update the non video part of Window
 *
 * Frame::processFrame Process the frame data (Application Logic)
 *
 *    Called from Frame::CheckMediaType for each sample frame
 *    Processing of the frame data
 * 
 */
#include "frame.h"


Frame::Frame( IUnknown* unk, HRESULT *hr ) 
	: CBaseVideoRenderer(__uuidof(CLSID_Frame), NAME("Frame Sampler"), unk, hr),
	  STOP_REQUESTED_COUNTER_MAX(100),
	  STOP_REQUESTED_WAIT_MILLISECS(10)
{
	debug = false;
	pixelbytes = 0;
	v_width = 0;
	v_height = 0;
	v_display_width = 0;
	v_display_height = 0;
	w_width = 0;
	w_height = 0;
	c_width = 0;
	c_height = 0;
	data = 0;
	FRAMEDELAY = 0;
	counter = 0;
	config = NULL;
	decoder = NULL;
	font = NULL;
	white_brush = NULL;
	min_memory = 0;
	hwnd = NULL;
	hstatus = NULL;

	for(int i=0; i < 12; i++) tag[i] = -1;
	for(int i=0; i < 16; i++) tag_string[i] = ' ';
	tag_string[15] = '\0';
	for(int i=0; i < 16; i++) last_tag_string[i] = ' ';
	last_tag_string[15] = '\0';
	last_tag_age = -1;
	for(int i=0; i < 256; i++) msg_string[i] = ' ';

	sprintf_s( tag_string, sizeof(tag_string), "MYTAGO" ); 

	FRAME_PROCESSING  = false;
	FRAME_DECODING  = false;
	STOP_REQUESTED = false;
	STOP_REQUESTED_COUNTER = 0;
}

Frame::~Frame()
{
	releaseResources();
}

void
Frame::setConfig(Config *_config)
{
	config = _config;
	if(config != NULL )  { 
		FRAMEDELAY = config->V_FRAME_DELAY;
		debug 	   = config->WIN_DEBUG;
	}
}

void
Frame::setDecoder(Decoder *_decoder)
{
	decoder = _decoder;
}

void
Frame::setWindow(HWND _hwnd)
{
	hwnd = _hwnd;
	//hdc  = GetDC(hwnd);
}

bool
Frame::waitforProcessing()
{
	STOP_REQUESTED = true;
	if(STOP_REQUESTED_COUNTER++ > STOP_REQUESTED_COUNTER_MAX) { 
			FRAME_PROCESSING = false; //FIXME
			return false; //Timeout
	}
	if(STOP_REQUESTED_COUNTER > 10) Sleep(STOP_REQUESTED_WAIT_MILLISECS);   //Wait and check
	return FRAME_PROCESSING;
}


HRESULT
Frame::CheckMediaType(const CMediaType *media ) 
{
	vi = (VIDEOINFO *)media->Format();
	if(!vi) return E_FAIL;
	if(!IsEqualGUID( *media->Subtype(), MEDIASUBTYPE_RGB24)) return E_FAIL;
	bmih 	   = vi->bmiHeader;
	pixelbytes = vi->bmiHeader.biBitCount / 8;
	v_width    = vi->bmiHeader.biWidth;
	v_height   = vi->bmiHeader.biHeight;

	if( config->V_WIN_WIDTH < v_width ){
        /*float scale = (float)v_width / (float)config->V_WIN_WIDTH;
        v_display_width  = config->V_WIN_WIDTH;
        v_display_height = (int) ((float)v_height/scale);
		if(v_display_height%2) v_display_height++; //multiple of two*/
		config->FUZZY->sizeFrame(	&v_display_width, &v_display_height, 
					config->V_WIN_WIDTH, config->V_WIN_HEIGHT, 
					v_width, v_height, 1);
    }else{
        v_display_width  = v_width;
        v_display_height = v_height;
    }

	w_width  = v_display_width < config->V_WIN_WIDTH-(2*config->V_WIN_BORDER) ? 
				config->V_WIN_WIDTH  :
				v_display_width  + (2*config->V_WIN_BORDER);
	w_height = v_display_height < config->V_WIN_HEIGHT-(2*config->V_WIN_BORDER)-config->V_MSG_STRIP_SIZE ? 
				config->V_WIN_HEIGHT : 
				v_display_height + (2*config->V_WIN_BORDER) + config->V_MSG_STRIP_SIZE ;
	
	w_width  += ( 2 * GetSystemMetrics(SM_CYFRAME) );
	w_height += ( 2 * GetSystemMetrics(SM_CYMENU)) + 
				( 2 * GetSystemMetrics(SM_CYFRAME)) +
				GetSystemMetrics(SM_CYCAPTION);

	SetWindowPos(hwnd,0,CW_USEDEFAULT, CW_USEDEFAULT, w_width, w_height, SWP_NOZORDER|SWP_NOMOVE);

	if(debug) { 
		printf("Frame::CheckMediaType xoffset = %d, yoffset = %d\n",  
					GetSystemMetrics(SM_CXFRAME) * 2,
	 				GetSystemMetrics(SM_CYMENU) + 
					GetSystemMetrics(SM_CYFRAME) + 
					GetSystemMetrics(SM_CYCAPTION) );	
		printf("Frame::CheckMediaType Video  %dx%d (bytes per pixel rgb %d )\n", v_width, v_height, pixelbytes);
		printf("Frame::CheckMediaType Window %dx%d \n", w_width, w_height);
	}

	RECT r;
	GetClientRect(hwnd, &r);
	c_width = r.right - r.left;
	c_height = r.bottom - r.top;

	if(debug) { 
		printf("Frame::CheckMediaType Client %dx%d \n", c_width, c_height);
		printf("Frame::CheckMediaType Client %d,%d %d,%d\n", r.right, r.left, r.bottom, r.top);
	}

	initResources();

	return  S_OK;
}

void
Frame::initResources()
{
    tag_font.lfStrikeOut = 0;
    tag_font.lfUnderline = 0;
    tag_font.lfHeight = 30;
    tag_font.lfEscapement = 0;
    tag_font.lfItalic = FALSE;
	tag_font.lfWeight = FW_BOLD;
	tag_font.lfPitchAndFamily = FF_SWISS;
    font = CreateFontIndirect(&tag_font);
	//GetSysColor(COLOR_BACKGROUND);
	white_brush = (HBRUSH)CreateSolidBrush(RGB(255,255,255));
	hstatus = GetDlgItem(hwnd, IDC_MAIN_STATUS);

}

void
Frame::releaseResources()
{
	if(font != NULL ) 	    DeleteObject(font);
	if(white_brush != NULL)  DeleteObject(white_brush);
}

void
Frame::statusMessage(char* msg, int part)
{
	if(hstatus == NULL) hstatus = GetDlgItem(hwnd, IDC_MAIN_STATUS);
	if(hstatus == NULL || msg == NULL) return;
	SendMessage(hstatus, SB_SETTEXT, part, (LPARAM)msg);
}

void
Frame::paintVideoFinish()
{
	HDC hdc = GetDC(hwnd);	
	RECT rect;
	rect.right  = 0;
	rect.left   = c_width;
	rect.top    = 0;
	rect.bottom = c_height - GetSystemMetrics(SM_CYMENU);
	FillRect(hdc, &rect, (HBRUSH)COLOR_BACKGROUND);
	ReleaseDC(hwnd, hdc);
	paintFrame();
}

void
Frame::paintFrame()
{
	paintFrame(tag_string, true, true);
}

void
Frame::paintFrame(bool newtag, bool newage)
{
	paintFrame(tag_string, newtag, newage);
}

void
Frame::paintFrame(char* text, bool newtag, bool newage)
{
	if(text==NULL && !newtag && !newage) return;
	
	HDC hdc = GetDC(hwnd);	
	if( newtag || newage ){ //redraw backhround only as required
		font_rect.right  = 0;
		font_rect.left   = c_width;
		font_rect.top    = c_height - GetSystemMetrics(SM_CYMENU) - config->V_MSG_STRIP_SIZE;
		font_rect.bottom = c_height - GetSystemMetrics(SM_CYMENU) ;
		FillRect(hdc, &font_rect, white_brush);
	}
	if(text != NULL ){
		SelectObject(hdc, font);
		SetTextAlign(hdc, TA_CENTER | TA_BASELINE );
		SetBkMode(hdc, TRANSPARENT);
		switch(decoder->tagAge())
		{
			case 0: SetTextColor(hdc, RGB(30, 144, 255));  break;
			case 1: SetTextColor(hdc, RGB(105, 105, 105));  break;
			case 2: SetTextColor(hdc, RGB(190, 190, 190));  break;
			case 3: SetTextColor(hdc, RGB(211, 211, 211));  break;
		}
    	TextOut(hdc, c_width/2 , c_height - GetSystemMetrics(SM_CYMENU) - (config->V_MSG_STRIP_SIZE/2) + 10, text, strlen(text));
	}
	ReleaseDC(hwnd, hdc);
}

void
Frame::processFrame()
{
	FRAME_PROCESSING  = false;
	if(STOP_REQUESTED) return;
	FRAME_PROCESSING  = true;
	FRAME_DECODING = false;
	//if(counter % config->V_SKIP_FRAMES == 0){ 
		FRAME_DECODING = true;
	 	if(decoder->processFrame((unsigned char*)data, v_width, v_height)){
			//statusMessage("Scanning ...", 0);
			decoder->copyTag(tag);
			sprintf_s( tag_string, sizeof(tag_string), "%d%d%d%d %d%d%d%d %d%d%d%d", 
						tag[0], tag[1], tag[2], tag[3],
						tag[4], tag[5], tag[6], tag[7],
						tag[8], tag[9], tag[10], tag[11] );
		}else{
			decoder->copyTag(tag);
		}
		sprintf_s(msg_string, sizeof(msg_string), "%d%d%d%d %d%d%d%d %d%d%d%d [%d]",
				tag[0], tag[1], tag[2], tag[3],
				tag[4], tag[5], tag[6], tag[7],
				tag[8], tag[9], tag[10], tag[11], decoder->validFor() );
		//statusMessage("Scanning ...", 0);
		statusMessage(msg_string, 0);

		FRAME_DECODING = false;
		if(config->WIN_PERF_LOG) { 
			if ( GetProcessMemoryInfo( GetCurrentProcess(), &pmc, sizeof(pmc)) ){
				int wss = 0, pwss = 0;
				char statmsg[256];
				wss = pmc.WorkingSetSize >= 1024 ? pmc.WorkingSetSize/1024 : 1;	
				pwss = pmc.PeakWorkingSetSize >= 1024 ? pmc.PeakWorkingSetSize/1024 : 1;	
				if(wss < min_memory || min_memory == 0 ) min_memory = wss;
				sprintf_s( statmsg, sizeof(statmsg), "Memory %dK (%dK-%dK)", wss, min_memory, pwss);
				//if(debug) printf( "WorkingSetSize: %dK (%dK-%dK)\n", wss, min_memory, pwss);
				statusMessage(statmsg, 1);
			}
	  	}
	//}else{
	 	//decoder->frameOverlayOnly((unsigned char*)data, v_width, v_height);
	//}
	if(FRAMEDELAY > 0 ) Sleep(FRAMEDELAY);
	//counter++;
	FRAME_PROCESSING  = false;
}

void
Frame::d_videoAlignmentTest()
{
	RGBBYTE *thispixel;
	if(data==NULL) return;
	for(int y = 5; y < 10; y++){
		for(int x = 10; x < 20; x++){
			int i = ( (v_height-y) * v_width) + x; 
    		thispixel = (RGBBYTE*) data + i;
     		thispixel->R = 255; 
			thispixel->G = 0;
			thispixel->B = 0;
		}
	}
}

HRESULT
Frame::DoRenderSample(IMediaSample *sample)
{
	if(FRAME_DECODING) { 
		if(debug) printf("Frame::DoRenderSample Cancelling DoRenderSample while FRAME_DECODING\n");
		return S_FALSE;
	}
	FRAME_PROCESSING  = false;
	if(STOP_REQUESTED) return S_FALSE;
	FRAME_PROCESSING  = true;
	data = 0;
	sample->GetPointer( &data );
	processFrame();
	bool newtag = false;
	bool newage = (last_tag_age == decoder->tagAge()) ? false : true ; 
	for(int i=0; i < 15; i++) { if( tag_string[i] != last_tag_string[i] ) newtag = true; }
	paintFrame(newtag, newage);
	for(int i=0; i < 15; i++) last_tag_string[i] = tag_string[i];
	last_tag_age = decoder->tagAge();
	//d_videoAlignmentTest();
	bmi.bmiHeader = bmih;
	HDC hdc = GetDC(hwnd);	
	HRESULT hr;
	SetStretchBltMode(hdc, HALFTONE);
	hr = StretchDIBits(hdc, 
		(c_width-v_display_width)/2, 
		(c_height - GetSystemMetrics(SM_CYMENU) - config->V_MSG_STRIP_SIZE - v_display_height)/2,
		v_display_width, v_display_height,
		0, 0, 
		v_width, v_height,
		data, &bmi, DIB_RGB_COLORS, SRCCOPY);
	ReleaseDC(hwnd, hdc);
	/* OVERLAY TEST
	RECT rect;
	rect.right  = 10;
	rect.left   = 20;
	rect.top    = 5;
	rect.bottom = 10;
	FillRect(hdc, &rect, debug_brush);
	OVERLAY TEST */
	if(config->V_D_PIXDEBUG) Sleep(600);	
	FRAME_PROCESSING  = false;
	return hr;
}

HRESULT
Frame::ShouldDrawSampleNow(IMediaSample *sample, REFERENCE_TIME *start, REFERENCE_TIME *stop) 
{
	return S_FALSE;
	/*if(STOP_REQUESTED) return S_FALSE;
	return S_OK; */
}
