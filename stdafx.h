// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#pragma warning (disable : 4018; disable : 4800; disable : 4244; disable : 4305; disable : 4018; disable : 4996)

//#define _HAS_ITERATOR_DEBUGGING 0
/*#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
*/


#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "resource.h"

#define MSG_PREVIEWCLOSING WM_USER+42
#define MSG_BIGPREVIEWCLOSING WM_USER+43

#pragma warning (disable: 4018)

#ifdef _UNICODE 
#define char_t wchar_t
#else
#define char_t char
#endif
// TODO: reference additional headers your program requires here
