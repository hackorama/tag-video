
DirectShow Video Processing is Setup  in "Video"
(Frame Filtering/Rendering is done in "Frame")

Video::initDShow Create graph and control

    FilterGraph (graph)
    MediaControl

Video::initFrameFilter | Create Frame filter and pin

    Create FrameFilter (frame)  [+++]
    Select pin (renderpin) [###]
    Add to graph

Video::initCameraSource Create Video Source filter and pin

    Enumerate Video Input Devices (cameras)
    Select camera device
    Bind camera device to BaseFilter
    Enumerate camera pins in BaseFilter
    Select pin (camerapin)

    Set notify Window for all media events [***]

    Connect camerapin to renderpin in the grpah

Video::handleGraphEvent Video graph events

    [***] The Media Event Goes to Windows event loop
    Where its checked for by WM_GRAPHNOTIFY
    And gets handled back in Video::handleGraphEvent


    [+++] frame filter is based on CBaseVideoRenderer

DirecctShow Frame Filtering/Rendering is done in "Frame"

Frame::CheckMediaType Video Media Dimensions and Type

    Frame -> CBaseVideoRenderer -> CBaseRenderer::CheckMediaType
    Called from CRendererInputPin::CheckMediaType during graph connection or render pin

    [###] Render pin found at frame -> IBaseFiletr::FindPin in Video::initCameraSource
    Once we get the media type and video dimension calculate and set Window dimensions

    Frame -> CBaseVideoRenderer -> CBaseRenderer::DoRenderSample
    Is called whenever there is a sample frame ready to render

    Store the frame data pointer to Frame::data

    Process the frame data in Frame::processFrame

    Then call StretchDIBits to draw the video frame data on the Window DC
    Before that calls Frame::paintFrame to update the non video part of Window

Frame::processFrame Process the frame data (Application Logic)

    Called from Frame::CheckMediaType for each sample frame
    Processing of the frame data

