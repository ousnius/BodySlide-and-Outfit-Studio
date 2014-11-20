// BodySlide.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "BodySlide.h"
#include "appcontrol.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

HFONT windowFont;				

AppControl* AppController = NULL;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BODYSLIDE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	if(!AppController) return FALSE;
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BODYSLIDE));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if(AppController) delete AppController;

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= 0;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BODYSLIDE));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDI_BODYSLIDE);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable
   SCROLLINFO si;

  hWnd = CreateWindowEx(WS_EX_COMPOSITED,szWindowClass, szTitle, WS_CLIPCHILDREN | WS_VSCROLL | WS_OVERLAPPED|WS_CAPTION |WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX| WS_THICKFRAME,
      CW_USEDEFAULT, 0, 730, 550, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
   // Build Font
   HDC hdc = GetDC(NULL);
   int fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
   ReleaseDC(NULL, hdc);
   windowFont = CreateFont(fontheight, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, "Tahoma");


   si.nPage = 30;
   si.nMin = 0;
   si.nMax = 330;
   si.fMask = SIF_PAGE | SIF_RANGE;

   SetScrollInfo(hWnd,SB_VERT,&si,FALSE);

   AppController = new AppControl(hWnd,hInstance);
   AppController->SetApplicationFont(windowFont);
   AppController->CreateControls();


   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	int scrollPos;
	PAINTSTRUCT ps;
	HDC hdc;

	RECT r;
	GetClientRect(hWnd,&r);
	
	r.top = r.top + 75;
	r.bottom = r.bottom - 90;
	//static HRGN hr = Creter

	switch (message)
	{
	case WM_NOTIFY:
		AppController->ProcessNotify((NMHDR*)lParam,0);
		break;
	case WM_VSCROLL:
		wmEvent = LOWORD(wParam);
		scrollPos = HIWORD(wParam);
		AppController->ProcessScroll(NULL,wmEvent,scrollPos,lParam);

		break;
	case WM_HSCROLL:
		wmEvent = LOWORD(wParam);
		scrollPos = HIWORD(wParam);
		AppController->ProcessScroll((HWND)lParam,wmEvent,scrollPos,lParam);
		break;
	case WM_SIZE:
		AppController->WindowResized(LOWORD(lParam),HIWORD(lParam));
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		AppController->ProcessCommand(wmId,wmEvent,lParam);
		switch (wmId)
		{
		//case IDM_ABOUT:
			//DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
		//	break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case MSG_PREVIEWCLOSING:
		AppController->ClosePreview(SMALL_PREVIEW);
		break;
	case MSG_BIGPREVIEWCLOSING:
		AppController->ClosePreview(BIG_PREVIEW);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
