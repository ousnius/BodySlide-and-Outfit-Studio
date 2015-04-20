// Include file for standard system include files or project specific include files
// that are used frequently, but are changed infrequently.

#pragma once
#pragma warning (disable : 4018; disable : 4800; disable : 4244; disable : 4305; disable : 4996)

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <Windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "Resource.h"

#define MSG_PREVIEWCLOSING WM_USER+42
#define MSG_BIGPREVIEWCLOSING WM_USER+43

#ifdef _UNICODE 
#define char_t wchar_t
#else
#define char_t char
#endif
