#pragma once
//#pragma warning (disable : 4018; disable : 4800; disable : 4244; disable : 4305; disable : 4996)

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <wx/wx.h>
#include "resource.h"

#define MSG_PREVIEWCLOSING WM_USER+42
#define MSG_BIGPREVIEWCLOSING WM_USER+43
