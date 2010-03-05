prox.exe:prox.obj
	link prox.obj ws2_32.lib /out:E:\srcbin\proxy\prox.exe /LIBPATH:"C:\Program Files\Microsoft Visual C++ Toolkit 2003\lib" \
/LIBPATH:"E:\Program Files\PlatformSDK\Lib"

prox.obj:prox.cpp
	cl /c /EHsc  /MT /I "C:\Program Files\Microsoft Visual C++ Toolkit 2003\include" \
/I "E:\Program Files\PlatformSDK\Include" prox.cpp

clean:
	del *.obj
