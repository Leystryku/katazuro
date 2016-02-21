#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN


#pragma comment (linker, "/NODEFAULTLIB:LIBCMT")
#pragma comment (linker, "/NODEFAULTLIB:LIBSTD")
#pragma comment (linker, "/NODEFAULTLIB:STDLIB")
#pragma comment (linker, "/NODEFAULTLIB:LIBCD")
#pragma comment (linker, "/NODEFAULTLIB:LIBCMTD")
#pragma comment (linker, "/NODEFAULTLIB:MSVCRTD")



#pragma comment (linker, "/NODEFAULTLIB:LIBCMT.lib")
#pragma comment (linker, "/NODEFAULTLIB:LIBSTD.lib")
#pragma comment (linker, "/NODEFAULTLIB:STDLIB.lib")
#pragma comment (linker, "/NODEFAULTLIB:LIBCD.lib")
#pragma comment (linker, "/NODEFAULTLIB:LIBCMTD.lib")
#pragma comment (linker, "/NODEFAULTLIB:MSVCRTD.lib")


#include <Windows.h>
#include <stdio.h>
#include <shlobj.h>
#include <shlguid.h>
