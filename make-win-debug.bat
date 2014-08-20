rc resource.rc
cl   /D "WIN32" /D "_DEBUG" /D "_WINDOWS"  /EHsc /I "IM\include"   /I "C:\Program Files\Microsoft SDKs\Windows\v6.1\Samples\Multimedia\DirectShow\BaseClasses"  /I  "C:\Program Files\Microsoft SDKs\Windows\v6.1\Include"  winmain.cpp guicon.cpp frame.cpp win.cpp video.cpp border.cpp config.cpp decoder.cpp matrix.cpp pattern.cpp shape.cpp threshold.cpp pixmap.cpp tagimage.cpp  /link /OUT:"test.exe" /nodefaultlib:libcmt.lib /LIBPATH:"C:\Program Files\Microsoft SDKs\Windows\v6.1\Samples\Multimedia\DirectShow\BaseClasses\Debug_MBCS"  /LIBPATH:"C:\Program Files\Microsoft SDKs\Windows\v6.1\Lib"  /LIBPATH:"IM\lib\"  resource.RES strmiids.lib winmm.lib strmbasd.lib user32.lib ole32.lib gdi32.lib oleaut32.lib advapi32.lib CORE_RL_magick_.lib


