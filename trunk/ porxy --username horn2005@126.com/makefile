prox.exe:prox.obj
	link prox.obj ws2_32.lib /out:..\bin\prox.exe /LIBPATH:"C:\Program Files\Microsoft Visual C++ Toolkit 2003\lib" \
/LIBPATH:"C:\Program Files\PlatformSDK\Lib"

prox.obj:prox.cpp
	cl /c /EHsc /Zi /MT /I "C:\Program Files\Microsoft Visual C++ Toolkit 2003\include" \
/I "C:\Program Files\PlatformSDK\Include" prox.cpp
