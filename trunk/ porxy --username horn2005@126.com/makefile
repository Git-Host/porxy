prox.exe:prox.obj work_threadinfo.obj
	link prox.obj work_threadinfo.obj ws2_32.lib /Debug /out:prox.exe /LIBPATH:"C:\Program Files\Microsoft Visual C++ Toolkit 2003\lib" \
/LIBPATH:"C:\Program Files\PlatformSDK\Lib"

prox.obj:prox.cpp
	cl /c /EHsc /Zi /MT /I "C:\Program Files\Microsoft Visual C++ Toolkit 2003\include" \
/I "C:\Program Files\PlatformSDK\Include" prox.cpp

work_threadinfo.obj:work_threadinfo.cpp work_threadinfo.h
	cl /c /EHsc /Zi /MT /I "C:\Program Files\Microsoft Visual C++ Toolkit 2003\include" \
/I "C:\Program Files\PlatformSDK\Include" work_threadinfo.cpp