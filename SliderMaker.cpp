/*
Caliente's BodySlide
by Caliente

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "slidermaker.h"
#include "Commctrl.h"
#include "Commdlg.h"
#include "previewWindow.h"
#include "shlobj.h"
#include "tinyxml.h"
#include "glSurface.h"	// for kdmatcher
#include "objfile.h"
#include "automorph.h"
#include "diffdata.h"
#include "SliderData.h"
	
bool SliderMaker::has_registered = false;
WNDPROC OldEditProc;
WNDPROC OldBtnProc;

void SliderMaker::registerClass() {
	WNDCLASSEXA wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW ;
	wcex.lpfnWndProc	= SliderMakerWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hAppInstance;
	wcex.hIcon			= LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= "BS_SLIDERMAKER_WINDOW";
	wcex.hIconSm		= LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));

	RegisterClassExA(&wcex);
	has_registered = true;
}
SliderMaker::SliderMaker(void) {
	shapePreview = NULL;
	sourcePreview = NULL;
	automorphPreview = NULL;
	config = new map<string, string>;
	selfconfig = true;
}
SliderMaker::~SliderMaker(void) {
	for(int i=0;i<diffTracker.size();i++) {
		delete diffTracker[i];
	}
	diffTracker.clear();
	if(shapePreview) {
		delete shapePreview;
	}
	if(sourcePreview) {
		delete sourcePreview;
	}
	if(selfconfig) {
		delete config;
	}
}

void SliderMaker::Close() {

}

SliderMaker::SliderMaker(HWND owner, map<string, string>* app_config){
	hAppInstance = GetModuleHandle(NULL);
	if(!has_registered) 
		registerClass ();
	
	shapePreview = NULL;
	sourcePreview = NULL;
	string title = "Caliente's BodySlide Slider Maker";
	selfconfig = false;

	config = app_config;

	RECT ownerRect;
	GetWindowRect(owner, &ownerRect);
	
	hOwner = owner;//WS_OVERLAPPEDWINDOW
	int sX = GetSystemMetrics(SM_CXDLGFRAME)*2;
	int sY = GetSystemMetrics(SM_CYDLGFRAME)*2;
	sY += GetSystemMetrics(SM_CYSMCAPTION);
	mHwnd = CreateWindowExA(0,"BS_SLIDERMAKER_WINDOW",title.c_str(),WS_OVERLAPPED | WS_DLGFRAME | WS_SYSMENU,
			ownerRect.left + 20,ownerRect.top+20,745+sX, 340+sY,NULL,NULL,GetModuleHandle(NULL),NULL);
	SetWindowLong(mHwnd,GWL_USERDATA,(LONG) this);

	HDC hdc = GetDC(NULL);
	int fontheight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(NULL, hdc);
	windowFont = CreateFontA(fontheight, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, "Tahoma");

	SetConfigDefaults();

	CreateControls();

	ShowWindow(mHwnd, SW_SHOW);
	UpdateWindow(mHwnd);

}

void SliderMaker::SetConfigDefaults() {
	if(!config){
		config = new map<string, string>;
		selfconfig = true;
	}
	if(config->find("DefaultObjScale")== config->end()) {
		(*config)["DefaultObjScale"] = "1.0";
	}
	if(config->find("SimilarityThreshold")== config->end()) {
		(*config)["SimilarityThreshold"] = "0.001";
	}
	if(config->find("GeneratorOutputFolder") == config->end()) {
		(*config)["GeneratorOutputFolder"] = "GeneratedOutput";
	} 
}

void SliderMaker::SetupToolTips() {
	mToolTipWnd = CreateWindow(TOOLTIPS_CLASS, NULL, WS_POPUP, 0,0,0,0, mHwnd, NULL, hAppInstance, 0);

	TOOLINFOA toolinfo; 
	memset(&toolinfo, 0, sizeof(TOOLINFO));
	toolinfo.cbSize = sizeof(TOOLINFO);
	toolinfo.hwnd = mHwnd;
	toolinfo.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	toolinfo.hinst = NULL;
		
	toolinfo.uId = (UINT_PTR)mBaseFileEdit; 
	toolinfo.lpszText ="A .nif file that will serve as the base shape. Sliders apply changes to the shapes within.";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mBaseFileLoadBtn; 
	toolinfo.lpszText ="Choose a .nif file to serve as the base shape. (the .nif can have many parts)";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mShapesList; 
	toolinfo.lpszText ="These are the shapes found in the chosen base .nif file";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mDiffList;
	toolinfo.lpszText ="List of the currently configured sliders. Each Slider applies to one shape, and is represented by two sliders in BodySlide (low weight and high weight)";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mAddBtn; 
	toolinfo.lpszText ="Add a new blank slider definition";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mDelBtn; 
	toolinfo.lpszText ="Remove the currently selected slider definition";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	
	toolinfo.uId = (UINT_PTR)mLoadDirBtn; 
	toolinfo.lpszText ="Select a directory and load all .obj files within to automatically create a large number of sliders.";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );


	toolinfo.uId = (UINT_PTR)mSliderNameEdit; 
	toolinfo.lpszText ="The name of the slider you wish to appear in BodySlide";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mSliderSrcEdit;
	toolinfo.lpszText ="Slider data source file. This should be an .obj file that mostly matches the .nif base shape, except for the portions you wish the slider to control. Typically, these should be modifications of the mesh data in the .nif";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mSliderSrcBtn; 
	toolinfo.lpszText ="Load a data source file for the currently selected slider";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mSmallPctEdit; 
	toolinfo.lpszText ="The default 'low weight' percentage contribution from the currently selected slider";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mBigPctEdit; 
	toolinfo.lpszText ="The default 'high weight' percentage contribution from the currently selected slider";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mInvertSliderCheck;  
	toolinfo.lpszText ="Invert the slider action. For instance, a slider that makes a portion of the mesh shrink should be inverted, so higher percentages represent larger weights";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mHiddenSliderCheck; 
	toolinfo.lpszText ="Hide the slider in BodySlide.  This is for representing shapes that are different between weights, but the user shouldn't change (like neck seams)";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mZapSliderCheck;
	toolinfo.lpszText ="Cause Vertexes adjusted by this source data to be deleted from final output. This enables you to specify areas that will not be drawn in game, thus avoiding clipping problems.";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mScaleSrcEdit;
	toolinfo.lpszText ="Scale to apply to the data source .obj file.  Ctl+Enter while editing this will set the default scale until the appliction exits";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR) mCancelBtn; 
	toolinfo.lpszText ="Abandon Hope!  RUN!  AAAaaaarrrggghh!";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );
	
	toolinfo.uId = (UINT_PTR) mCreateBtn;
	toolinfo.lpszText ="Make it so, #1";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mShapePreviewBtn; 
	toolinfo.lpszText ="Show a preview of the selected shape.  Hold control to see a preview of all the shapes in the .nif";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mSrcPreviewBtn; 
	toolinfo.lpszText ="Show a preview of the data source for this slider";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mSrcMakeDiffBtn; 
	toolinfo.lpszText ="Output a diff file between current slider src and current shape. Will not create slider information";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mGamePathEdit; 
	toolinfo.lpszText ="Game mesh path information starting from the Data directory. eg. meshes\\armor\\dragonbone";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mGameFileEdit; 
	toolinfo.lpszText ="Game mesh file name, without the size and file type.  eg. dragonbonearmorf";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mShapeExportBtn; 
	toolinfo.lpszText ="Export the currently selected shape as an .obj file for editing in a 3d program. Note the resulting mesh may need to be repositioned and scaled in a 3d editor.  Hold Control to export all shapes.";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );

	toolinfo.uId = (UINT_PTR)mShapeRefBtn; 
	toolinfo.lpszText ="Load a reference slider set for the currently selected shape.  For instance, reference a body set for the body mesh beneath some armor.";
	SendMessage ( mToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfo );
}

void SliderMaker::CreateControls() {
	

	int listWidth = 400;
	int listHeight = 240;
	int lColumnpos = 10;
	int rColumnpos = listWidth + lColumnpos + 10;

	mBaseFileEdit = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","Load a .nif file base...", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
		lColumnpos,10,400,25,mHwnd,(HMENU) ID_BASEFILEEDIT,hAppInstance,NULL);
	SendMessage(mBaseFileEdit,WM_SETFONT, (WPARAM)windowFont,0);
	mBaseFileLoadBtn = CreateWindowA("BUTTON","Load Base File...", WS_CHILD | WS_VISIBLE, 
		rColumnpos, 10, 128, 25, mHwnd, (HMENU)ID_BASEFILELOADBTN, hAppInstance,NULL);
	SendMessage(mBaseFileLoadBtn,WM_SETFONT,(WPARAM)windowFont,0);
	
	LVCOLUMNA lvc;
	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.iSubItem = 0;
	lvc.pszText = "Shape";
	lvc.cx = (listWidth-4)*.26;
	mDiffList = CreateWindowExA(WS_EX_CLIENTEDGE,"SysListView32","",WS_CHILD | WS_VISIBLE  | LVS_REPORT | LVS_NOSORTHEADER | LVS_SINGLESEL| LVS_SHOWSELALWAYS | WS_DISABLED, 
		lColumnpos,50, listWidth, listHeight, mHwnd,(HMENU) ID_DIFFLIST,hAppInstance,NULL);
	SendMessage(mDiffList,WM_SETFONT, (WPARAM)windowFont,0);
	ListView_SetExtendedListViewStyle(mDiffList,LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	ListView_InsertColumn(mDiffList,0,&lvc);	
	lvc.iSubItem = 1;	lvc.pszText = "Slider Name";	lvc.cx = (listWidth-4)*.265;
	ListView_InsertColumn(mDiffList,1,&lvc);	
	lvc.iSubItem = 2;	lvc.pszText = "Small %";	lvc.cx = (listWidth-4)*.15;
	ListView_InsertColumn(mDiffList,2,&lvc);	
	lvc.iSubItem = 3;	lvc.pszText = "Big %";	lvc.cx = (listWidth-4)*.15;
	ListView_InsertColumn(mDiffList,3,&lvc);	
	lvc.iSubItem = 4;	lvc.pszText = "Similarity %";	lvc.cx = (listWidth-4)*.18;
	ListView_InsertColumn(mDiffList,4,&lvc);	

    mAddBtn = CreateWindowA("BUTTON","Add Slider", WS_CHILD | WS_VISIBLE | WS_DISABLED, 
		rColumnpos, 50, 100, 25, mHwnd, (HMENU)ID_DIFFADDBTN,hAppInstance,NULL);
	SendMessage(mAddBtn,WM_SETFONT,(WPARAM)windowFont,0);
	OldBtnProc = (WNDPROC) SetWindowLong(mAddBtn,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mAddBtn,GWL_USERDATA,(LONG)this);

    mDelBtn = CreateWindowA("BUTTON","Delete Slider", WS_CHILD | WS_VISIBLE| WS_DISABLED, 
		rColumnpos+105, 50, 100, 25, mHwnd, (HMENU)ID_DIFFDELBTN,hAppInstance,NULL);
	SendMessage(mDelBtn,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mDelBtn,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mDelBtn,GWL_USERDATA,(LONG)this);

    mLoadDirBtn = CreateWindowA("BUTTON","Load Folder", WS_CHILD | WS_VISIBLE, 
		rColumnpos+210, 50, 100, 25, mHwnd, (HMENU)ID_LOADDIRBTN,hAppInstance,NULL);
	SendMessage(mLoadDirBtn,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mLoadDirBtn,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mLoadDirBtn,GWL_USERDATA,(LONG)this);

	mShapesList = CreateWindowExA(WS_EX_CLIENTEDGE,"ComboBox","ShapesList",WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_DISABLED , 
		rColumnpos,90,180,200,mHwnd,(HMENU)ID_SHAPESLIST,hAppInstance,NULL);
    SendMessage(mShapesList,WM_SETFONT, (WPARAM)windowFont,0);
    mShapePreviewBtn = CreateWindowA("BUTTON","Preview", WS_CHILD | WS_VISIBLE| WS_DISABLED, 
		rColumnpos+185, 90, 80, 20, mHwnd, (HMENU)ID_SHAPEPREVIEWBTN,hAppInstance,NULL);
	SendMessage(mShapePreviewBtn,WM_SETFONT,(WPARAM)windowFont,0);
    mShapeRefBtn = CreateWindowA("BUTTON","Ref", WS_CHILD | WS_VISIBLE| WS_DISABLED, 
		rColumnpos+265, 90, 30, 20, mHwnd, (HMENU)ID_SHAPEREFBTN,hAppInstance,NULL);
	SendMessage(mShapeRefBtn,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mShapeRefBtn,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mShapeRefBtn,GWL_USERDATA,(LONG)this);
    mShapeExportBtn = CreateWindowA("BUTTON","E", WS_CHILD | WS_VISIBLE| WS_DISABLED, 
		rColumnpos+295, 90, 20, 20, mHwnd, (HMENU)ID_SHAPEEXPORTBTN,hAppInstance,NULL);
	SendMessage(mShapeExportBtn,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mShapeExportBtn,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mShapeExportBtn,GWL_USERDATA,(LONG)this);

	mSliderSrcEdit = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","Load a slider source .obj",WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL| WS_DISABLED,
		rColumnpos,120,180,20,mHwnd,(HMENU) ID_SLIDERSRCEDIT,hAppInstance,NULL);
	SendMessage(mSliderSrcEdit,WM_SETFONT, (WPARAM)windowFont,0);
    mSliderSrcBtn = CreateWindowA("BUTTON","Load Slider Src", WS_CHILD | WS_VISIBLE| WS_DISABLED, 
		rColumnpos+185, 120, 90, 20, mHwnd, (HMENU)ID_SLIDERSRCBTN,hAppInstance,NULL);
	SendMessage(mSliderSrcBtn,WM_SETFONT,(WPARAM)windowFont,0);	
	SetWindowLong(mSliderSrcBtn,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mSliderSrcBtn,GWL_USERDATA,(LONG)this);
    mSrcPreviewBtn = CreateWindowA("BUTTON","P", WS_CHILD | WS_VISIBLE| WS_DISABLED, 
		rColumnpos+275, 120, 20, 20, mHwnd, (HMENU)ID_SRCPREVIEWBTN,hAppInstance,NULL);
	SendMessage(mSrcPreviewBtn,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mSrcPreviewBtn,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mSrcPreviewBtn,GWL_USERDATA,(LONG)this);
    mSrcMakeDiffBtn = CreateWindowA("BUTTON","D", WS_CHILD | WS_VISIBLE| WS_DISABLED, 
		rColumnpos+295, 120, 20, 20, mHwnd, (HMENU)ID_SRCMAKEDIFFBTN,hAppInstance,NULL);
	SendMessage(mSrcMakeDiffBtn,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mSrcMakeDiffBtn,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mSrcMakeDiffBtn,GWL_USERDATA,(LONG)this);

	mSliderNameEdit = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","SliderNameEdit",WS_CHILD | WS_VISIBLE  | ES_AUTOHSCROLL| WS_DISABLED,
		rColumnpos,145,180,20,mHwnd,(HMENU) ID_SLIDERNAMEEDIT,hAppInstance,NULL);
	SendMessage(mSliderNameEdit,WM_SETFONT, (WPARAM)windowFont,0);
	OldEditProc = (WNDPROC) SetWindowLong(mSliderNameEdit,GWL_WNDPROC,(LONG)EditControlTabHandler);
	SetWindowLong(mSliderNameEdit,GWL_USERDATA,(LONG)this);
	
	mSmallPctEdit = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","SmallPercent",WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL| WS_DISABLED | WS_TABSTOP,
		rColumnpos,180,75,20,mHwnd,(HMENU) ID_SMALLPCTEDIT,hAppInstance,NULL);
	SendMessage(mSmallPctEdit,WM_SETFONT, (WPARAM)windowFont,0);
	(WNDPROC) SetWindowLong(mSmallPctEdit,GWL_WNDPROC,(LONG)EditControlTabHandler);
	SetWindowLong(mSmallPctEdit,GWL_USERDATA,(LONG)this);
	mBigPctEdit = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","BigPercent",WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL| WS_DISABLED| WS_TABSTOP,
		rColumnpos+80,180,75,20,mHwnd,(HMENU) ID_BIGPCTEDIT,hAppInstance,NULL);
	SendMessage(mBigPctEdit,WM_SETFONT, (WPARAM)windowFont,0);
	(WNDPROC) SetWindowLong(mBigPctEdit,GWL_WNDPROC,(LONG)EditControlTabHandler);
	SetWindowLong(mBigPctEdit,GWL_USERDATA,(LONG)this);

	mInvertSliderCheck = CreateWindowA("BUTTON","Invert Slider Action", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX| WS_DISABLED, 
		rColumnpos, 205, 150, 20, mHwnd, (HMENU)ID_INVERTSLIDERCHECK,hAppInstance,NULL);
	SendMessage(mInvertSliderCheck,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mInvertSliderCheck,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mInvertSliderCheck,GWL_USERDATA,(LONG)this);

	mHiddenSliderCheck = CreateWindowA("BUTTON","Make Hidden Slider", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX| WS_DISABLED, 
		rColumnpos+ 155, 205, 220, 20, mHwnd, (HMENU)ID_HIDDENSLIDERCHECK,hAppInstance,NULL);
	SendMessage(mHiddenSliderCheck,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mHiddenSliderCheck,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mHiddenSliderCheck,GWL_USERDATA,(LONG)this);
	mZapSliderCheck = CreateWindowA("BUTTON","Vertex Zap", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX| WS_DISABLED, 
		rColumnpos+ 155, 225, 220, 20, mHwnd, (HMENU)ID_ZAPSLIDERCHECK,hAppInstance,NULL);
	SendMessage(mZapSliderCheck,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mZapSliderCheck,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mZapSliderCheck,GWL_USERDATA,(LONG)this);

	mScaleSrcEdit = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","ScaleSource",WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL| WS_DISABLED,
		rColumnpos,230,75,20,mHwnd,(HMENU) ID_SCALESRCEDIT,hAppInstance,NULL);
	SendMessage(mScaleSrcEdit,WM_SETFONT, (WPARAM)windowFont,0);
	(WNDPROC) SetWindowLong(mScaleSrcEdit,GWL_WNDPROC,(LONG)EditControlTabHandler);
	SetWindowLong(mScaleSrcEdit,GWL_USERDATA,(LONG)this);

	mGamePathEdit = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","Game Data Path",WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		lColumnpos,293,400,20,mHwnd,(HMENU) ID_GAMEPATHEDIT,hAppInstance,NULL);
	SendMessage(mGamePathEdit,WM_SETFONT, (WPARAM)windowFont,0);
	(WNDPROC) SetWindowLong(mGamePathEdit,GWL_WNDPROC,(LONG)EditControlTabHandler);
	SetWindowLong(mGamePathEdit,GWL_USERDATA,(LONG)this);

	mGameFileEdit = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT","Game File Name (leave off \"_0.nif\")",WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		lColumnpos,315,400,20,mHwnd,(HMENU) ID_GAMEFILEEDIT,hAppInstance,NULL);
	SendMessage(mGameFileEdit,WM_SETFONT, (WPARAM)windowFont,0);
	(WNDPROC) SetWindowLong(mGameFileEdit,GWL_WNDPROC,(LONG)EditControlTabHandler);
	SetWindowLong(mGameFileEdit,GWL_USERDATA,(LONG)this);


	
    mCancelBtn = CreateWindowA("BUTTON","Close", WS_CHILD | WS_VISIBLE, 
		rColumnpos+20, 300, 128, 30, mHwnd, (HMENU)ID_CANCELBTN,hAppInstance,NULL);
	SendMessage(mCancelBtn,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mCancelBtn,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mCancelBtn,GWL_USERDATA,(LONG)this);
    mCreateBtn = CreateWindowA("BUTTON","Create!", WS_CHILD | WS_VISIBLE | WS_DISABLED, 
		rColumnpos+180, 300, 128, 30, mHwnd, (HMENU)ID_CREATEBTN,hAppInstance,NULL);
	SendMessage(mCreateBtn,WM_SETFONT,(WPARAM)windowFont,0);
	SetWindowLong(mCreateBtn,GWL_WNDPROC,(LONG)ButtonControlTabHandler);
	SetWindowLong(mCreateBtn,GWL_USERDATA,(LONG)this);
	

	SetupToolTips();

}


int SliderMaker::LoadBaseFile() {
	char buffer[1024];
	OPENFILENAMEA ofn;
	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.hwndOwner = this->mHwnd;
	ofn.lpstrFilter = "Nif Files\0*.nif\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;	

	ofn.lStructSize = sizeof(ofn);
	if(!::GetOpenFileNameA(&ofn)) 
		return 1 ;
	string f = ofn.lpstrFile;
	
	if(baseNif.Load(f)) {
		MessageBoxA(NULL,"Unable to open base .nif file", "Open Failed.",MB_OK | MB_ICONASTERISK);
		return 2;
	}
	
	vector<string> baseShapes;
	baseNif.GetShapeList(baseShapes);

	SendMessage(mShapesList,CB_RESETCONTENT,0,0);

	for(int i=0;i<baseShapes.size();i++){ 
		SendMessage(mShapesList,CB_ADDSTRING,0,(LPARAM)baseShapes[i].c_str());
	}
	SendMessage(mShapesList,CB_SELECTSTRING,0,(LPARAM)baseShapes[0].c_str());

	SetWindowTextA(this->mBaseFileEdit,f.c_str());

	EnableWindow(mDiffList,TRUE);
	EnableWindow(mAddBtn,TRUE);
	EnableWindow(mShapesList,TRUE);
	EnableWindow(mShapePreviewBtn,TRUE);
	EnableWindow(mShapeRefBtn,TRUE);
	EnableWindow(mShapeExportBtn,TRUE);

	return 0;
}

int SliderMaker::ExportCurrentShape(bool exportAll) {
	float scale = atof((*config)["DefaultObjScale"].c_str());
	scale = 0.1f;
	vector3 offset(0, (2.54431), (-3.2879));
	char buffer[1024];
	char filetitlebuf[256];
	GetWindowTextA(mShapesList,buffer,1024);
	
	vector<float> optparams;
	optparams.push_back(scale);
	optparams.push_back(offset.x);
	optparams.push_back(offset.y);
	optparams.push_back(offset.z);

	int ret = DialogBoxParamA(hAppInstance,MAKEINTRESOURCEA(IDD_EXPORTSETTINGS),mHwnd,ExportSettingsInput,(LPARAM)&optparams);
	if(ret != IDOK) 
		return 0;

	scale = optparams[0];
	offset.x = optparams[1];
	offset.y = optparams[2];
	offset.z = optparams[3];
	
	vector<string> shapesToExport;
	if(exportAll) {
		baseNif.GetShapeList(shapesToExport);
	} else {
		shapesToExport.push_back(buffer);
	}
	
	strncat(buffer,".obj",4);

	OPENFILENAMEA ofn;
	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.hwndOwner = this->mHwnd;
	ofn.lpstrFilter = ".Obj Files\0*.obj\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	ofn.lpstrFileTitle = filetitlebuf;
	if(exportAll) {
		ofn.lpstrTitle = "Export all shapes";
	} else {
		ofn.lpstrTitle = "Export single shape";
	}
	//ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
	ofn.lStructSize = sizeof(ofn);
	if(!::GetSaveFileNameA(&ofn)) {
		return 0;
	}	
	string filenamebuild;
	string basePath = ofn.lpstrFile;
	string baseFileName = basePath.substr(ofn.nFileOffset);
	basePath = basePath.erase(ofn.nFileOffset);
	baseFileName = baseFileName.erase(baseFileName.rfind("."));
	if(shapesToExport.size() == 1){
		return baseNif.ExportShapeObj(string(ofn.lpstrFile),shapesToExport[0],scale,offset);
	} else {
		for(int i = 0; i < shapesToExport.size(); i++) {
			filenamebuild = basePath + baseFileName + "_" +shapesToExport[i]+".obj";
			int err = baseNif.ExportShapeObj(filenamebuild,shapesToExport[i],scale);
			if(err !=0) 
				return err;
		}
	}

	MessageBoxA(NULL,"Export completed successfully.","Complete",MB_OK);

	return 0;

}
int SliderMaker::LoadCurrentShapeRef() {
	char buffer[1024];
	
	GetWindowTextA(mShapesList,buffer,1024);

	DiffItemInfo sliderRef;
	//SliderRefInfo sliderRef;
	sliderRef.shape = buffer;

	OPENFILENAMEA ofn;
	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.hwndOwner = this->mHwnd;
	ofn.lpstrFilter = "Set Definition Files\0*.xml\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;	

	ofn.lStructSize = sizeof(ofn);
	if(!::GetOpenFileNameA(&ofn)) 
		return 0 ;
	string f = ofn.lpstrFile;

	string slidersetname;
	string slidername;
	TiXmlDocument doc;
	TiXmlElement* setsearch;
	TiXmlElement* e;
	TiXmlElement* shapename;
	TiXmlElement* datafile;
	TiXmlElement* root;

	if(doc.LoadFile(f.c_str())) {
		root = doc.FirstChildElement("SliderSetInfo");
		if(!root) return 1;
		setsearch = root->FirstChildElement("SliderSet");
		e = NULL;
		while(setsearch) {
			slidersetname = setsearch->Attribute("name");
			_snprintf(buffer,1024,"Reference Slider Set %s?",slidersetname.c_str());
			int ret = MessageBoxA(this->mHwnd,buffer,"Choose reference slider set",MB_YESNOCANCEL | MB_ICONQUESTION);
			if(ret==IDCANCEL) 
				return 0;
			if(ret==IDNO) {					
				setsearch = setsearch->NextSiblingElement("SliderSet");
			} else { 
				e = setsearch;
				break;
			}
		}
		if(!e) return 0;
		
		sliderRef.datafolder = e->FirstChildElement("SetFolder")->GetText();
		
		shapename = e->FirstChildElement("BaseShapeName");
		if(shapename->Attribute("DataFolder")) {
			MessageBoxA(mHwnd,"The referenced set cannot itself be a reference!","Please choose another set",MB_OK);
			return 2;
		}

		e = e->FirstChildElement("Slider");
		while(e) {			
			sliderRef.name = e->Attribute("name");
			if(e->Attribute("invert")) {
				sliderRef.invert = (_strnicmp(e->Attribute("invert"),"true",4) == 0);
			} else 
				sliderRef.invert = false;

			double d;
			e->Attribute("big",&d);
			sliderRef.defBig = d;
			e->Attribute("small",&d);
			sliderRef.defSmall = d;

			sliderRef.hidden=false;
			if(e->Attribute("hidden")) {
				if(_strnicmp(e->Attribute("hidden"),"true",4) == 0) {
					sliderRef.hidden = true;
				}
			}
			sliderRef.zap = false;
			if(e->Attribute("zap")) {
				if(_strnicmp(e->Attribute("zap"),"true",4) == 0) {
					sliderRef.zap = true;
				}
			} 
			sliderRef.clamp = false;
			if(e->Attribute("clamp")) {
				if(_strnicmp(e->Attribute("clamp"),"true",4) == 0) {
					sliderRef.clamp = true;
				}
			} 

			datafile = e->FirstChildElement("datafile");
			if(!datafile ) {
				return 3;
			}
			sliderRef.srcfilename = datafile->GetText();
			sliderRef.dataname = sliderRef.srcfilename.substr(0,sliderRef.srcfilename.rfind("."));
			
			sliderRefs.push_back(sliderRef);

			e=e->NextSiblingElement("Slider");
		}

	}
	return 0;
}

//struct VertUV {
//	int v;
//	int uv;
//	VertUV (int inV, int inUV):v(inV), uv(inUV) {}
//};

void SliderMaker::ConfigUVDupThreshold() {
	string promptResult;
	
	promptResult = (*config)["UVDupThreshold"];
	if(promptResult == "") 
		promptResult = "0.005";
	int ret = DialogBoxParamA(hAppInstance,MAKEINTRESOURCEA(IDD_SIMPLEINPUT),mHwnd,UVDupThresholdInput,(LPARAM)&promptResult);
	if(ret != IDOK) 
		return;

	(*config)["UVDupThreshold"] = promptResult;

}

int SliderMaker::LoadSlidersFromDirectory() {

	bool groupFound = false;

	char buffer[1024];
	char title [24];
	_snprintf(title,24,"%s","Choose a Directory");
	_snprintf(buffer,24,"%s","Open Directory");
	OPENFILENAMEA ofn;
	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.hwndOwner = this->mHwnd;
	ofn.lpstrFilter = ".Obj Files\0*.obj\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrTitle = title;
	ofn.nMaxFile = 1024;
	ofn.lpfnHook = DirSelectHook;
	ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR;	

	ofn.lStructSize = sizeof(ofn);
	if(!::GetOpenFileNameA(&ofn)) 
		return 0 ;
	string fname;
	string dirname = ofn.lpstrFile;
	dirname = dirname.substr(0,ofn.nFileOffset);

	WIN32_FIND_DATAA wfd;
	HANDLE hFind;
	int searchStatus = 0;
	string search  = dirname + "*.obj";

	hFind = FindFirstFileA(search.c_str(),&wfd);

	if(hFind ==INVALID_HANDLE_VALUE) {
		return 1;
	}
	while(searchStatus != ERROR_NO_MORE_FILES) {
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else{	
			fname = dirname;
			fname += wfd.cFileName;
			LoadSliderSource(fname,true);
		}
		if(!FindNextFileA(hFind,&wfd)) {
			searchStatus = GetLastError();
		} else 
			searchStatus = 0;
	}
	FindClose(hFind);
	return 0;
}

int SliderMaker::LoadSliderSource(const string& infn, bool createDiffEntry){LVITEM lvi;
		
	if(!createDiffEntry) {
		lvi.mask = LVIF_PARAM;	
		int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
		if(item == -1) return 1;
		lvi.iItem = item;
		if(!ListView_GetItem(mDiffList,&lvi)) 
			return 2;
	} 
	vector<string> objGroups;
	string word;
	bool groupFound = false;
	string fname;

	char buffer[1024];
	if(infn.length() == 0) {
		OPENFILENAMEA ofn;
		memset(&ofn,0,sizeof(OPENFILENAME));
		ofn.hwndOwner = this->mHwnd;
		ofn.lpstrFilter = ".Obj Files\0*.obj\0\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = buffer;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = 1024;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;	

		ofn.lStructSize = sizeof(ofn);
		if(!::GetOpenFileNameA(&ofn)) 
			return 0 ;
		 fname = ofn.lpstrFile;
	} else {
		fname = infn;
	}

	fstream base;
	base.open(fname.c_str(),ios_base::in | ios_base::binary);
	if(base.fail()) 
		return 1;

	GetWindowTextA(mShapesList,buffer,256);

	while(!base.eof()) {
		base >> word;
		if(word.compare("g") == 0) {
			base >> word;
			objGroups.push_back(word);
			if(word.compare(buffer) == 0) {
				groupFound = true;			
			}
		}
	}
	
	DiffItemInfo* dii;
	
	if(objGroups.size() > 1 && groupFound){ 		
		if(!createDiffEntry) {
			DelSliderEntry();
		}
		for(int i=0;i<objGroups.size();i++) {
			base.clear();
			base.seekg(0,std::ios_base::beg);
			int namestart = fname.rfind("\\");
			int nameend = fname.rfind(".");
			
			dii = AddSliderEntry(objGroups[i],fname.substr(namestart+1,nameend-namestart-1) );
			dii->srcfilename = fname;	
			dii->dataname = dii->shape + dii->name;

			LoadSliderSourceData(base,dii,objGroups[i],objGroups[i]);
			if(dii->name.find("BaseShape") == string::npos) {
				if(dii->similarity == 100.0f) {
					DelSliderEntry();
				}
			}
		}
	} else if(objGroups.size()) {
		if(!createDiffEntry) {
			DelSliderEntry();
		}
		for(int i=0;i<objGroups.size();i++) {
			base.clear();
			base.seekg(0,std::ios_base::beg);
			int namestart = fname.rfind("\\");
			int nameend = fname.rfind(".");
			
			dii = AddSliderEntry(objGroups[i],fname.substr(namestart+1,nameend-namestart-1) );
			dii->srcfilename = fname;	

			LoadSliderSourceData(base,dii,"__CALC__",objGroups[i]);

			dii->dataname = dii->shape + dii->name;

			if((dii->name.find("BaseShape") == string::npos) && (dii->name.find("PointCheck") == string::npos)) {
				if(dii->similarity == 100.0f) {
					DelSliderEntry();
				}
			}
		}
	} else {
		base.clear();
		base.seekg(0,std::ios_base::beg);
		if(createDiffEntry) {
			dii = AddSliderEntry(buffer);
		} else {
			dii = (DiffItemInfo*)lvi.lParam;
		}
		dii->srcfilename = fname;	
		
		int namestart = dii->srcfilename.rfind("\\");
		int nameend = dii->srcfilename.rfind(".");
		dii->name = dii->srcfilename.substr(namestart+1,nameend-namestart-1);
		dii->SetDefaults();
		dii->dataname = dii->name;

		LoadSliderSourceData(base, dii, buffer, "");
		UpdateFromSelection();
		SetBigPercent();
		SetSmallPercent();
	}

	base.close();

	SetWindowTextA(mSliderSrcEdit,fname.c_str());
	SetWindowTextA(mSliderNameEdit, dii->name.c_str());

	EnableWindow(mSrcPreviewBtn,TRUE);	
	EnableWindow(mSrcMakeDiffBtn,TRUE);	

	return 0;
}

int SliderMaker::LoadSliderSourceData(fstream& base, DiffItemInfo* dii, const string& shapename, const string& groupname) {

	dii->tris.clear();
	dii->verts.clear();
	dii->OrderedVerts.clear();
    vector3 v;
	

	triangle tri;

	string dump;
	string curgrp;
	string facept1;
	string facept2;
	string facept3;
	string facept4;
	
	
	int nPoints = 0;
	
	//vector3 offsetVec(0.0f,-0.25443f,0.32879f);
	vector3 offsetVec(0.0f,0.0f,0.0f);
	string offsetVecX =(*config)["offsetVecX"];
	string offsetVecY =(*config)["offsetVecY"];
	string offsetVecZ =(*config)["offsetVecZ"];
	offsetVec.x = atof(offsetVecX.c_str());
	offsetVec.y = atof(offsetVecY.c_str());
	offsetVec.z = atof(offsetVecZ.c_str());
	vector<vector3> verts;
	vector<vector2> uvs;
	vector<triangle> tris;
	
	map<int,vector<VertUV>> vertMap;
	map<int,vector<VertUV>>::iterator savedVert;

	float UVDupThreshold;
	string UVDupStr = (*config)["UVDupThreshold"];
	if(UVDupStr == "") { 
		UVDupThreshold = 0.005;
	} else 
		UVDupThreshold = atof(UVDupStr.c_str());

	bool gotface = false;
	bool readgroup = true;
/* */
	ObjFile objfile;
	objfile.UVDupThreshold = UVDupThreshold;
	objfile.LoadForNif(base,groupname);
	objfile.CopyDataForGroup(groupname,&dii->verts,&dii->tris,NULL);
/* */
/*
	while(!base.eof()) {
		if(!gotface)
			base >> dump;
		else gotface = false;

		if(dump.compare("v") == 0) {
			base >> v.x >> v.y >> v.z;
			verts.push_back(v);
		}else if(dump.compare("g") == 0) {
			base >> curgrp;
			if(groupname.length() > 0) {
				if(curgrp.compare(groupname)==0) {
					readgroup = true;
				} else {
					readgroup = false;
				}
			}
		}else if (dump.compare("vt") == 0) {		
			base >> uv.u >> uv.v;
			uvs.push_back(uv);
		} else if(dump.compare("f") ==0) {
			base >> facept1 >> facept2 >> facept3;
			pos = facept1.find('/');
			f[0]=atoi(facept1.c_str())-1;
			ft[0]=atoi(facept1.substr(pos+1).c_str())-1;
			pos = facept2.find('/');
			f[1]=atoi(facept2.c_str())-1;
			ft[1]=atoi(facept2.substr(pos+1).c_str())-1;
			pos = facept3.find('/');
			f[2]=atoi(facept3.c_str())-1;
			ft[2]=atoi(facept3.substr(pos+1).c_str())-1;
			base >> facept4;

			if(facept4 =="f") {
				gotface = true;
				dump="f";
				nPoints = 3;
			} else if(facept4 == "g") {			
				gotface = true;
				dump="g";
				nPoints = 3;
			} else if(facept4.length() > 0) {
				pos = facept4.find('/');
				if(pos == string.npos) {
					gotface = true;
					dump="f";
					nPoints = 3;
				} else {
					f[3] = atoi(facept4.c_str())-1;
					ft[3]=atoi(facept4.substr(pos+1).c_str())-1;
					nPoints = 4;
				}
			}

			if(f[0] == -1 || f[1] == -1 || f[2] == -1) {
				continue;
			}
			
			if(!readgroup) 
				continue;

			for(int i =0;i<nPoints;i++) {
				v_idx[i] = dii->verts.size();
				if((savedVert = vertMap.find(f[i])) != vertMap.end()) {
					for(int j =0;j<savedVert->second.size();j++) {
						if(savedVert->second[j].uv == ft[i])
							v_idx[i]=savedVert->second[j].v;
						else {
							uv = uvs[ft[i]];
							uv2 = uvs[savedVert->second[j].uv];
							if(fabs(uv.u - uv2.u) > UVDupThreshold) {
								v_idx[i]=v_idx[i];
								continue;
							} else if (fabs(uv.v - uv2.v) > UVDupThreshold) {
								v_idx[i]=v_idx[i];
								continue;
							}
							v_idx[i]=savedVert->second[j].v;
						}
					}
				}

				if(v_idx[i] == dii->verts.size()) {
					vertMap[f[i]].push_back(VertUV(v_idx[i],ft[i]));
					if(dii->name != "PointCheck") {
						verts[f[i]].x += offsetVec.x;
						verts[f[i]].y += offsetVec.y;
						verts[f[i]].z += offsetVec.z;
					}
					dii->verts.push_back(verts[f[i]]);
				}


			}
			tri.p1 = v_idx[0];
			tri.p2 = v_idx[1];
			tri.p3 = v_idx[2];
			dii->tris.push_back(tri);
			if(nPoints == 4) {
				tri.p1 = v_idx[0];
				tri.p2 = v_idx[2];
				tri.p3 = v_idx[3];
				dii->tris.push_back(tri);
			}

		}
	}
//*/

	char buffer[256];
	const vector<vector3>* baseverts;
	//GetWindowText(mShapesList,buffer,256);
	if(shapename == "__CALC__") {
		vector <string> shapelist;
		baseNif.GetShapeList(shapelist);
		for(int i = 0;i<shapelist.size();i++){
			baseverts = baseNif.GetRawVertsForShape(shapelist[i]);
			if(baseverts->size() == dii->verts.size()) {
				dii->shape = shapelist[i];
				//ListView_SetItemText(mDiffList,dii->listItem,0,(LPSTR)dii->shape.c_str());
				break;
			}
		}
	} else {
		baseverts = baseNif.GetRawVertsForShape(shapename);
	}
	double dx,dy,dz;
	double bx,by,bz;
	if(baseverts->size() != dii->verts.size()) {
		_snprintf(buffer,1024,"Warning: this .obj does not have the same number of vertices as the selected shape.  You may need to adjust the UV match threshold (currently %f).\r\n shape=%d, obj=%d",UVDupThreshold,baseverts->size(),dii->verts.size());
		MessageBoxA(NULL,buffer,"Vertex Match",MB_OK | MB_ICONWARNING);
	} else {
		int a = 0;
		float sx;
		float sy;
		float sz;
		while(a<dii->verts.size()) {
			while(dii->verts[a].x == 0 || dii->verts[a].y == 0 || dii->verts[a].z == 0) {
				a++;
			} // calc scale in 3 axes and round away the tiny differences. 
			sx = ( baseverts->at(a).x / dii->verts[a].x ) * 10000.0f;
			sx = (sx > 0.0) ? floor(sx+0.5) : ceil(sx-.05);
			sy = ( baseverts->at(a).y / dii->verts[a].y ) * 10000.0f;
			sy = (sy > 0.0) ? floor(sy+0.5) : ceil(sy-.05);
			sz = ( baseverts->at(a).z / dii->verts[a].z ) * 10000.0f;
			sz = (sz > 0.0) ? floor(sz+0.5) : ceil(sz-.05);
			// compare the axes scales -- if they match there is a good likelyhood that the scale value is the overall
			// scale (since any non-scale transformation would have to be exactly 45 degrees in all three axes. Possible, 
			// but overall unlikely.
			if(sx == sy && sy == sz) {
				dii->scale = sx/10000.0f;  // save scale for shape and remove the rounding factor.
				break;
			}	
			a++;
		}
		if(a >= dii->verts.size()) {
			if(dii->name == "PointCheck") {
				// assume correct scale and calculate offset
				dx = baseverts->at(0).x - dii->verts[0].x*dii->scale;
				dy = baseverts->at(0).y - dii->verts[0].y*dii->scale;
				dz = baseverts->at(0).z - dii->verts[0].z*dii->scale;

				//  Centroid offset calculation -- doesn't normalize scale so it's worthless.
				dx=dy=dz=bx=by=bz=0.0f;
				int npoints = dii->verts.size();
				for(int i=0;i<npoints;i++) {
					dx += dii->verts[i].x*dii->scale;
					dy += dii->verts[i].y*dii->scale;
					dz += dii->verts[i].z*dii->scale;
					bx += baseverts->at(i).x;
					by += baseverts->at(i).y;
					bz += baseverts->at(i).z;
				}
				dx /= npoints;
				dy /= npoints;
				dz /= npoints;
				bx /= npoints;
				by /= npoints;
				bz /= npoints;
				dx = bx - dx;
				dy = by - dy;
				dz = bz - dz;
				for(int i=0;i<npoints;i++) {
					dii->verts[i].x+=(dx/dii->scale);
					dii->verts[i].y+=(dy/dii->scale);
					dii->verts[i].z+=(dz/dii->scale);			
				}
				
				char offsetbuf[24];
				_snprintf(offsetbuf,24,"%f",dx/dii->scale);
				(*config)["offsetVecX"] = offsetbuf;
				_snprintf(offsetbuf,24,"%f",dy/dii->scale);
				(*config)["offsetVecY"] = offsetbuf;
				_snprintf(offsetbuf,24,"%f",dz/dii->scale);
				(*config)["offsetVecZ"] = offsetbuf;

			}
		}
	}
	
	CalcSimilarity(dii,baseverts);
	
	return 0;
}

int SliderMaker::AutoShapeBaseFile() {
	Automorph morpher;
	//string refFileName = "D:\\proj\\skyrim\\BodySlide\\BodySlide\\SliderSets\\CalienteSets.xml";
	//string refSliderSet = "CalienteBody";
	string refFileName = "D:\\proj\\skyrim\\BodySlide\\BodySlide\\SliderSets\\CalConvert.xml";
	string refSliderSet = "Body Convert Skyrim2Base";
	string baseDataPath = "D:\\proj\\skyrim\\BodySlide\\BodySlide\\ShapeData\\";


/*
	SliderSetFile SrcSliderSetFile;
	SliderSet SrcSet;
	DiffDataSets SrcDiff;
	SrcSliderSetFile.Open();
	SrcSliderSetFile.GetSet("CalienteBody",SrcSet);
	SrcSet.SetBaseDataPath("");
	SrcSet.LoadSetDiffData(SrcDiff);
*/

	char str[256];
	GetWindowTextA(mShapesList,str,256);
	string curshape = str;
	NifFile ref;
	ref.Load("D:\\proj\\skyrim\\BodySlide\\BodySlide\\ShapeData\\CalConvert\\CalienteBodyVanillashape.nif");
	morpher.SetRef(ref);

	morpher.InitRefDiffData(refFileName,refSliderSet,baseDataPath);

	morpher.SourceShapesFromNif(baseNif);
	
	LARGE_INTEGER li1, li2, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&li1);
	morpher.BuildProximityCache(curshape);
	QueryPerformanceCounter(&li2);
	char m [256];
	_snprintf(m,256,"avg: %f\nmin: %d\nmax: %d\n%f", morpher.avgCount, morpher.minCount, morpher.maxCount, (li2.QuadPart-li1.QuadPart)/(double)freq.QuadPart);


	vector<vector3> testverts;
	vector<vector3> armorverts;
	ref.GetVertsForShape("BaseShape",testverts);
	morpher.ApplyDiffToVerts("VanillaToCBBE","BaseShape",&testverts);

	automorphPreview = new PreviewWindow(mHwnd,&ref);
	automorphPreview->Update(0,&testverts);
	//automorphPreview->AddMeshDirect(morpher.MorphRef);
	baseNif.GetVertsForShape("Monk", armorverts);
	automorphPreview->AddMeshFromNif(&baseNif,"Monk");

	morpher.GenerateResultDiff("Monk", "VanillaToCBBE", "VanillaToCBBE");
	morpher.ApplyResultToVerts("VanillaToCBBE","Monk",&armorverts);
	automorphPreview->Update(1,&armorverts);
	baseNif.SetVertsForShape("Monk", armorverts);
	baseNif.ExportShapeObj(string("C:\\Temp\\monk.obj"),string("Monk"),0.1f);

	//&baseNif,0,"IronArmor");
	//automorphPreview->
	return 0;
	
}

float SliderMaker::CalcSimilarity(DiffItemInfo* dii,const vector<vector3>* baseverts) {
	int count = dii->verts.size();
	if(count == 0 ) return 0.0f;
	if(baseverts->size() < count) count = baseverts->size();

	int matchcount = 0;

	float threshold = atof((*config)["SimilarityThreshold"].c_str());

	for(int i = 0; i< count ; i++ ){
		if((fabs((dii->verts[i].x * dii->scale) - (*baseverts)[i].x) < threshold) &&
			(fabs((dii->verts[i].y * dii->scale)- (*baseverts)[i].y) < threshold) &&
			(fabs((dii->verts[i].z * dii->scale)- (*baseverts)[i].z) < threshold))
			matchcount++;
		else 
			matchcount = matchcount;
	}

	dii->similarity = (float)matchcount / (float)count*100.0f;

	char buf[25];
	_snprintf(buf,25,"%0.2f",dii->similarity);

	//ListView_SetItemText(mDiffList,dii->listItem,4,buf);

	return dii->similarity;

}

void SliderMaker::UpdateFromSelection()  {
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	char str[256];
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	SetWindowTextA(mSliderSrcEdit,dii->srcfilename.c_str());
	SetWindowTextA(mSliderNameEdit,dii->name.c_str());
	_snprintf(str,256,"%0.2f",dii->defBig*100);
	SetWindowTextA(mBigPctEdit,str);	
	_snprintf(str,256,"%0.2f",dii->defSmall*100);
	SetWindowTextA(mSmallPctEdit,str);
	_snprintf(str,256,"%0f",dii->scale);
	SetWindowTextA(mScaleSrcEdit,str);
	if(dii->invert) {
		SendMessage(mInvertSliderCheck,BM_SETCHECK,BST_CHECKED,0);
	} else {
		SendMessage(mInvertSliderCheck,BM_SETCHECK,BST_UNCHECKED,0);
	}
	if(dii->hidden) {
		SendMessage(mHiddenSliderCheck,BM_SETCHECK,BST_CHECKED,0);
	} else {
		SendMessage(mHiddenSliderCheck,BM_SETCHECK,BST_UNCHECKED,0);
	}

	if(dii->zap) {
		SendMessage(mZapSliderCheck,BM_SETCHECK,BST_CHECKED,0);
	} else {
		SendMessage(mZapSliderCheck,BM_SETCHECK,BST_UNCHECKED,0);
	}

	if(dii->srcfilename != "Load slider source data from .obj"){
		EnableWindow(mSrcPreviewBtn,TRUE);
	} else {
		EnableWindow(mSrcPreviewBtn,FALSE);
	}

	SendMessage(mShapesList,CB_SELECTSTRING,0,(LPARAM)dii->shape.c_str());


}


DiffItemInfo* SliderMaker::AddSliderEntry(const string& shape, const string& slidername) {
	
	LVITEMA lvi;
	DiffItemInfo* dii = new DiffItemInfo();
	dii->scale = atof((*config)["DefaultObjScale"].c_str());
	char str[256];
	if(shape.length()==0) {
		GetWindowTextA(mShapesList,str,256);
		dii->shape = str;
	} else dii->shape = shape;
	if(slidername.length()>0) {
		dii->name = slidername;
	}
	dii->SetDefaults();

	int n = diffTracker.size();
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.lParam = (LPARAM)dii;
	lvi.iItem = n;
	lvi.iSubItem = 0;
	lvi.pszText = (LPSTR) dii->shape.c_str();
	dii->listItem = ListView_InsertItem(mDiffList,&lvi);
	lvi.mask = LVIF_TEXT; lvi.lParam= 0;
	lvi.iSubItem = 1; lvi.pszText = (LPSTR) dii->name.c_str();
	ListView_SetItem(mDiffList,&lvi);
	lvi.iSubItem = 2; lvi.pszText = (LPSTR) "0";
	ListView_SetItem(mDiffList,&lvi);
	lvi.iSubItem = 3; lvi.pszText = (LPSTR) "100";
	ListView_SetItem(mDiffList,&lvi);


	diffTracker.push_back(dii);
	SetFocus(mDiffList);
	ListView_SetItemState(mDiffList,dii->listItem,(LVIS_SELECTED|LVIS_FOCUSED), LVIS_SELECTED|LVIS_FOCUSED);
	//ListView_Update(mDiffList,dii->listItem);
	EnableEditingControls(true);

	return dii;

}
void SliderMaker::DelSliderEntry() {
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	int newItemSel = item;
	int count = ListView_GetItemCount(mDiffList);
	ListView_DeleteItem(mDiffList,item);
	SetFocus(mDiffList);
	if(newItemSel == count-1) {
		newItemSel --;		
	} 
	ListView_SetItemState(mDiffList,newItemSel,(LVIS_SELECTED|LVIS_FOCUSED), LVIS_SELECTED|LVIS_FOCUSED);
	if(count <= 1) 
		EnableEditingControls(FALSE);

	for(int i=0;i<diffTracker.size();i++) {
		if(diffTracker[i]->listItem == item) {
			delete diffTracker[i];
			diffTracker.erase(diffTracker.begin()+i);
			if(i<diffTracker.size() && (diffTracker[i]->listItem > item)) {
				diffTracker[i]->listItem--;
			}
		} else if(diffTracker[i]->listItem > item) {
			diffTracker[i]->listItem--;
		}
	}
	return;
}

void SliderMaker::SetCurrentSliderName() {
	char buf[256];
	LVITEMA lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	GetWindowTextA(mSliderNameEdit,buf,256);
	dii->name = buf;

	lvi.mask = LVIF_TEXT;
	lvi.pszText = buf;
	lvi.iSubItem = 1;
	ListView_SetItem(mDiffList,&lvi);

}

void SliderMaker::SetCurrentSliderScale() {
	char buf[256];
	LVITEMA lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	GetWindowTextA(mScaleSrcEdit,buf,256);
	float newval = atof(buf);
	if(dii->scale != newval) {
		dii->scale = atof(buf);
		CalcSimilarity(dii,baseNif.GetRawVertsForShape(dii->shape));
	}


}
void SliderMaker::SetCurrentShape() {
	char buf[256];
	LVITEMA lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	GetWindowTextA(mShapesList,buf,256);
	dii->shape = buf;

	lvi.mask = LVIF_TEXT;
	lvi.pszText = buf;
	lvi.iSubItem = 0;
	ListView_SetItem(mDiffList,&lvi);

	if(dii->verts.size() > 0 ) {
		CalcSimilarity(dii,baseNif.GetRawVertsForShape(dii->shape));
	}

}

void  SliderMaker::SetBigPercent() {
	char buf[256];
	LVITEMA lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	GetWindowTextA(mBigPctEdit,buf,256);
	dii->defBig = atof(buf) / 100;
	_snprintf(buf,256,"%0.2f",dii->defBig*100);

	lvi.mask = LVIF_TEXT;
	lvi.pszText = buf;
	lvi.iSubItem = 3;
	ListView_SetItem(mDiffList,&lvi);

}

void  SliderMaker::SetSmallPercent() {
	char buf[256];
	LVITEMA lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	GetWindowTextA(mSmallPctEdit,buf,256);
	dii->defSmall = atof(buf) / 100;
	_snprintf(buf,256,"%0.2f",dii->defSmall*100);

	lvi.mask = LVIF_TEXT;
	lvi.pszText = buf;
	lvi.iSubItem = 2;
	ListView_SetItem(mDiffList,&lvi);

}
void SliderMaker::SetObjScale() {

	char buf[256];
	LVITEMA lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	GetWindowTextA(mScaleSrcEdit,buf,256);
	dii->scale = atof(buf);

}

void SliderMaker::SetSliderInvert() {

	LVITEM lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	if(SendMessage(mInvertSliderCheck,BM_GETCHECK,0,0) == BST_CHECKED) {
		dii->invert =true;
	} else {
		dii->invert =false;
	}

}

void SliderMaker::SetSliderHidden() {

	LVITEM lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	if(SendMessage(mHiddenSliderCheck,BM_GETCHECK,0,0) == BST_CHECKED) {
		dii->hidden =true;
	} else {
		dii->hidden =false;
	}

}

void SliderMaker::SetZapSource() {

	LVITEM lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	if(SendMessage(mZapSliderCheck,BM_GETCHECK,0,0) == BST_CHECKED) {
		dii->zap =true;
	} else {
		dii->zap =false;
	}

}

void SliderMaker::PreviewCurrentShape(bool showOnlySelected) {
	char buf[256];
	GetWindowTextA(mShapesList,buf,256);
	if(!shapePreview) {
		if(showOnlySelected)
			shapePreview = new PreviewWindow(mHwnd,&baseNif,0,buf);
		else
			shapePreview = new PreviewWindow(mHwnd,&baseNif);
	}
}

int SliderMaker:: MakeCurrentSrcDiff() {
	char buffer[1024];
	LVITEMA lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return 0;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return 0;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;

	_snprintf(buffer,1024,"%s.bsd",dii->dataname.c_str());

	OPENFILENAMEA ofn;
	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.hwndOwner = this->mHwnd;
	ofn.lpstrFilter = "BodySlide Shape Diff files\0*.bsd\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	//ofn.lpstrFileTitle = dfn.c_str()
	ofn.lpstrTitle = "Export all shapes";
	//ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
	ofn.lStructSize = sizeof(ofn);
	if(!::GetSaveFileNameA(&ofn)) {
		return 0;
	}	

	unordered_map<int,vector3> outDiffData;
	if(dii->verts.size() > 0) {		
		baseNif.CalcShapeDiff(dii->shape,&dii->verts, outDiffData,dii->scale);
		if(saveDiffData(ofn.lpstrFile,outDiffData) ){
			MessageBoxA(NULL,ofn.lpstrFile,"Failed to save .bsd file",MB_OK|MB_ICONERROR);
			return GetLastError();
		}
	}
	
	return 0;

}

void SliderMaker::PreviewSrcZap(DiffItemInfo* dii) {
	vector<vector3> verts;
	vector<triangle> tris;
	unordered_map<int, vector3> diffData;
	int remCount=0;
	if(dii->verts.size() == 0) {
		return;
	}
	baseNif.CalcShapeDiff(dii->shape,&dii->verts,diffData,dii->scale);	
	if(diffData.size() == 0) {		
		sourcePreview = new PreviewWindow(mHwnd,&dii->verts,&dii->tris,dii->scale);
		return;
	}

	vector<int> indexCollapse(dii->verts.size(),0);	

	for (int i = 0, j=0; i<indexCollapse.size(); i++ ) {
		if(j < dii->verts.size() && diffData.find(i)!=diffData.end()) {			// found one to remove
			indexCollapse[i] = -1;		// flag delete
			remCount++;
			j++;
		} else {
			indexCollapse[i] = remCount;
		}
	}
	verts = dii->verts;
	tris = dii->tris;

	for (int i = verts.size()-1; i>=0; i--) {
		if(indexCollapse[i] == -1) {
			verts.erase(verts.begin() + i);
		}
	}

	for (int i = tris.size()-1; i>=0; i--) {
		if(indexCollapse[tris[i].p1] == -1 ||
			indexCollapse[tris[i].p2] == -1 ||
			indexCollapse[tris[i].p3] == -1) {
			tris.erase(tris.begin() + i);
		} else {
			tris[i].p1 = tris[i].p1 - indexCollapse[tris[i].p1];			
			tris[i].p2 = tris[i].p2 - indexCollapse[tris[i].p2];
			tris[i].p3 = tris[i].p3 - indexCollapse[tris[i].p3];
		}
	}

	sourcePreview = new PreviewWindow(mHwnd,&verts,&tris,dii->scale);

}

void SliderMaker::PreviewCurrentSrc() {
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;	
	int item = ListView_GetNextItem(mDiffList,-1,LVIS_FOCUSED);
	if(item == -1) return;
	lvi.iItem = item;
	if(!ListView_GetItem(mDiffList,&lvi)) 
		return;
	DiffItemInfo* dii = (DiffItemInfo*)lvi.lParam;	

	if(dii->verts.size() > 0) {
		if(!sourcePreview) 
			if(dii->zap) {
				PreviewSrcZap(dii);
			} else {
				//sourcePreview = new PreviewWindow(mHwnd,&dii->OrderedVerts,&dii->tris,dii->scale);
				sourcePreview = new PreviewWindow(mHwnd,&dii->verts,&dii->tris,dii->scale);
			}
	}
		
}

void SliderMaker::ClosePreview(PreviewWindow* which) {
	if(shapePreview && shapePreview == which) {
		delete shapePreview;
		shapePreview=NULL;
	} else if(sourcePreview && sourcePreview == which) {
		delete sourcePreview;
		sourcePreview = NULL;
	} else if(automorphPreview ){
		delete automorphPreview;
		automorphPreview = NULL;
	}
}

int SliderMaker::CreateSliderSet() {
	string promptResult;
	char buffer[1024];
	int i,j;

	GetWindowTextA(mBaseFileEdit,buffer,1024);
	string srcfilename = buffer;

	int namestart = srcfilename.rfind("\\");
	int nameend = srcfilename.rfind(".");
	promptResult = srcfilename.substr(namestart+1,nameend-namestart-1);

	int ret = DialogBoxParamA(hAppInstance,MAKEINTRESOURCEA(IDD_SIMPLEINPUT),mHwnd,SliderSetNameInput,(LPARAM)&promptResult);
	if(ret != IDOK) 
		return 0;

	string genOutFolder = (*config)["GeneratorOutputFolder"] + "\\";
	string shapePath = ".\\" + genOutFolder +  promptResult + "\\";
	string setDefFile = shapePath + promptResult + "Set.xml";
	string shapeFile = srcfilename.substr(namestart+1);
	GetCurrentDirectoryA(1024,buffer);
	string pathcreate = buffer;
	pathcreate += "\\" + genOutFolder + promptResult ;
		SHCreateDirectoryExA(NULL,pathcreate.c_str(),NULL);

	TiXmlDocument doc (setDefFile.c_str());
	TiXmlElement* pDataFileElem;
	TiXmlNode* pNode = doc.InsertEndChild(TiXmlElement("SliderSetInfo"));
	TiXmlElement* pElem = pNode->InsertEndChild(TiXmlElement("SliderSet"))->ToElement();
	pElem->SetAttribute("name",promptResult.c_str());

	TiXmlElement* pDataElem = pElem->InsertEndChild(TiXmlElement("SetFolder"))->ToElement();
	pDataElem->InsertEndChild(TiXmlText(promptResult.c_str()));

	pDataElem = pElem->InsertEndChild(TiXmlElement("SourceFile"))->ToElement();
	pDataElem->InsertEndChild(TiXmlText(shapeFile.c_str()));

	pDataElem = pElem->InsertEndChild(TiXmlElement("OutputPath"))->ToElement();
	GetWindowTextA(mGamePathEdit,buffer,1024);
	pDataElem->InsertEndChild(TiXmlText(buffer));

	pDataElem = pElem->InsertEndChild(TiXmlElement("OutputFile"))->ToElement();
	GetWindowTextA(mGameFileEdit,buffer,1024);
	pDataElem->InsertEndChild(TiXmlText(buffer));

	vector<string> shapeList;
	baseNif.GetShapeList(shapeList);
	for(i=0;i<shapeList.size();i++) {
		pDataElem = pElem->InsertEndChild(TiXmlElement("BaseShapeName"))->ToElement();
		pDataElem->SetAttribute("target",shapeList[i].c_str());
		for(j=0;j<sliderRefs.size();j++) {
			if(sliderRefs[j].shape == shapeList[i]) {
				pDataElem->SetAttribute("DataFolder",sliderRefs[j].datafolder.c_str());
			}
		}
		pDataElem->InsertEndChild(TiXmlText(shapeList[i].c_str()));
	}
	
	map<string, TiXmlElement*> sliderElements;
	string dfn;
	//map<string,SliderRefInfo>::iterator refIt;
	//map<string,string>::iterator refDataFile;

	map<int,bool> PendingSliderRefs;
	for(i=0;i<sliderRefs.size();i++) {
		PendingSliderRefs[i] = true;
	}

	for(i=0;i<diffTracker.size();i++) {
		DiffItemInfo* dii = diffTracker[i];
		if(sliderElements.find(dii->name) == sliderElements.end()) {
			pDataElem = pElem->InsertEndChild(TiXmlElement("Slider"))->ToElement();
			pDataElem->SetAttribute("name",dii->name.c_str());
			if(dii->invert) pDataElem->SetAttribute("invert","true");
			else pDataElem->SetAttribute("invert","false");			
			if(dii->hidden) pDataElem->SetAttribute("hidden","true");
			if(dii->zap) pDataElem->SetAttribute("zap","true");
			_snprintf(buffer,25,"%0.f",dii->defSmall * 100);
			pDataElem->SetAttribute("small",buffer);
			_snprintf(buffer,25,"%0.f",dii->defBig * 100);
			pDataElem->SetAttribute("big",buffer);
			sliderElements[dii->name] =  pDataElem;
		} else {
			pDataElem = sliderElements[dii->name];
		}
		pDataFileElem = pDataElem->InsertEndChild(TiXmlElement("datafile"))->ToElement();
		pDataFileElem->SetAttribute("name",dii->dataname.c_str());
		pDataFileElem->SetAttribute("target",dii->shape.c_str());
		for(j=0;j<sliderRefs.size();j++) {
			if(sliderRefs[j].shape == dii->shape) {
				pDataFileElem->SetAttribute("local","true");
				break;
			}
		}
		dfn = dii->dataname + ".bsd";
		pDataFileElem->InsertEndChild(TiXmlText(dfn.c_str()));
		unordered_map<int,vector3> outDiffData;
		baseNif.CalcShapeDiff(dii->shape,&dii->verts, outDiffData,dii->scale);
		if(saveDiffData(shapePath+dfn,outDiffData) ){
			MessageBoxA(NULL,(shapePath+dfn).c_str(),"Failed to save .bsd file",MB_OK|MB_ICONERROR);
		}

		for(j = 0;j<sliderRefs.size();j++) {
			if(sliderRefs[j].name == dii->name) {
				pDataFileElem = pDataElem->FirstChildElement("datafile");
				bool skip = false;
				while(pDataFileElem) {
					if(sliderRefs[j].shape == pDataFileElem->Attribute("target")) {
						 skip = true;
					}
					pDataFileElem = pDataFileElem->NextSiblingElement("datafile");
				}
				if(skip) 
					continue;
				pDataFileElem = pDataElem->InsertEndChild(TiXmlElement("datafile"))->ToElement();		
				pDataFileElem->SetAttribute("name",sliderRefs[j].dataname.c_str());
				pDataFileElem->SetAttribute("target",sliderRefs[j].shape.c_str());
				pDataFileElem->InsertEndChild(TiXmlText(sliderRefs[j].srcfilename.c_str()));

				PendingSliderRefs[j] = false;

			}
		}
	}

	for(i = 0 ; i<sliderRefs.size();i++ ){
		if(!PendingSliderRefs[i]) continue;
		pDataElem = pElem->InsertEndChild(TiXmlElement("Slider"))->ToElement();
		pDataElem->SetAttribute("name",sliderRefs[i].name.c_str());
		if(sliderRefs[i].invert) pDataElem->SetAttribute("invert","true");
		else pDataElem->SetAttribute("invert","false");			
		if(sliderRefs[i].hidden) pDataElem->SetAttribute("hidden","true");
		if(sliderRefs[i].zap) pDataElem->SetAttribute("zap","true");
		if(sliderRefs[i].clamp) pDataElem->SetAttribute("clamp","true");
		_snprintf(buffer,25,"%0.f",sliderRefs[i].defSmall);
		pDataElem->SetAttribute("small",buffer);
		_snprintf(buffer,25,"%0.f",sliderRefs[i].defBig);
		pDataElem->SetAttribute("big",buffer);

		pDataFileElem = pDataElem->InsertEndChild(TiXmlElement("datafile"))->ToElement();		
		pDataFileElem->SetAttribute("name",sliderRefs[i].dataname.c_str());
		pDataFileElem->SetAttribute("target",sliderRefs[i].shape.c_str());
		pDataFileElem->InsertEndChild(TiXmlText(sliderRefs[i].srcfilename.c_str()));

	}

	baseNif.Save(shapePath+shapeFile);

	doc.SaveFile();

	MessageBoxA(NULL,"Files created!","Complete",MB_OK);

	return 0;
	
}

int SliderMaker::saveDiffData(const string& filename, unordered_map<int,vector3>& data) {
	fstream outFile(filename.c_str(),ios_base::out|ios_base::binary|ios_base::trunc);
	if(!outFile.is_open()) {
		return 1;
	}
	unordered_map<int,vector3>::iterator it;
	int sz = data.size();
	outFile.write((char*)&sz,4);
	for(it = data.begin();it!=data.end();++it) {
		outFile.write((char*)&it->first,sizeof(int));
		outFile.write((char*)&it->second,sizeof(vector3));
	}
	outFile.close();
	return 0;
}


LRESULT CALLBACK EditControlTabHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	SliderMaker* sm = (SliderMaker*)GetWindowLong(hWnd,GWL_USERDATA);
	switch(msg) {
		case WM_SETFOCUS:
			SendMessage(hWnd,EM_SETSEL,0,-1);
		case WM_CHAR:
			if(wParam == 0x09) {				
				if(GetKeyState(VK_SHIFT) & 0x80)
					sm->NextControl(hWnd, true);
				else 
					sm->NextControl(hWnd, false);
				return TRUE;
			}if(wParam == 0x0D) {
				sm->NextControl(hWnd, false);
				return TRUE;
			}if(wParam == 0x0A) {
				sm->NextControl(hWnd, true);
				return TRUE;
			}
		default:
			return CallWindowProc(OldEditProc,hWnd,msg,wParam, lParam);
	}
}
LRESULT CALLBACK ButtonControlTabHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	SliderMaker* sm = (SliderMaker*)GetWindowLong(hWnd,GWL_USERDATA);
	switch(msg) {
		case WM_CHAR:
			if(wParam == 0x09) {				
				if(GetKeyState(VK_SHIFT) & 0x80)
					sm->NextControl(hWnd, true);
				else 
					sm->NextControl(hWnd, false);
				return TRUE;
			} 
		default:
			return CallWindowProc(OldBtnProc,hWnd,msg,wParam, lParam);
	}
}


LRESULT CALLBACK SliderMakerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	SliderMaker* sm = (SliderMaker*)GetWindowLong(hWnd,GWL_USERDATA);
	int id;
	NMHDR* nHdr = (NMHDR*)lParam;
	NMLISTVIEW* nmLV = (NMLISTVIEW*) lParam;
	switch (msg) {
		case MSG_BIGPREVIEWCLOSING:
		case MSG_PREVIEWCLOSING:
			sm->ClosePreview((PreviewWindow*)lParam);
			break;
		case WM_NOTIFY:
			switch(nHdr->idFrom) {
				case ID_DIFFLIST:
					if(nmLV->hdr.code == LVN_ITEMCHANGED && (nmLV->uNewState & LVIS_SELECTED)) {
						sm->UpdateFromSelection();
					}
					break;
			}
			break;
		case WM_DESTROY:
			delete sm;
			break;
		case WM_COMMAND:
			id = LOWORD(wParam);
			switch(id) {
				case ID_SHAPESLIST:
					if(HIWORD(wParam) == CBN_SELCHANGE) {
						sm->SetCurrentShape();
					}
					break;
				case ID_SLIDERNAMEEDIT:
					if(HIWORD(wParam) ==EN_CHANGE) {
						sm->SetCurrentSliderName();
					}
					break;
				case ID_BIGPCTEDIT:
					sm->SetBigPercent();
					break;
				case ID_SMALLPCTEDIT:
					sm->SetSmallPercent();
					break;
				case ID_SCALESRCEDIT:
					if(HIWORD(wParam) == EN_KILLFOCUS) {
						sm->SetCurrentSliderScale();
						
						if(GetKeyState(VK_CONTROL) & 0x80) {
							sm->SetDefaultSliderScale();
						}
					}
					//sm->SetObjScale();
					break;
				case ID_INVERTSLIDERCHECK:
					sm->SetSliderInvert();
					break;
				case ID_HIDDENSLIDERCHECK:
					sm->SetSliderHidden();
					break;
				case ID_ZAPSLIDERCHECK:
					sm->SetZapSource();
					break;
				case ID_SHAPEPREVIEWBTN:
							
					if(GetKeyState(VK_CONTROL) & 0x80)
						sm->PreviewCurrentShape(false);
					else 
						sm->PreviewCurrentShape(true);
					break;
				case ID_SHAPEREFBTN:
					sm->LoadCurrentShapeRef();
					break;
				case ID_SHAPEEXPORTBTN:
					if(GetKeyState(VK_CONTROL) & 0x80)
						sm->ExportCurrentShape(true);
					else
						sm->ExportCurrentShape(false);
					break;
				case ID_SRCPREVIEWBTN:
					sm->PreviewCurrentSrc();
					break;
				case ID_SLIDERSRCBTN:
					if(GetKeyState(VK_CONTROL) & 0x80) {
						sm->ConfigUVDupThreshold();
					} 
					sm->LoadSliderSource();
					break;
				case ID_SRCMAKEDIFFBTN:	
					sm->MakeCurrentSrcDiff();
					break;
				case ID_DIFFADDBTN:
					sm->AddSliderEntry();
					break;
				case ID_DIFFDELBTN:
					sm->DelSliderEntry();
					break;
				case ID_LOADDIRBTN:
					sm->AutoShapeBaseFile();
					//sm->LoadSlidersFromDirectory();
					break;
				case ID_BASEFILELOADBTN:
					sm->LoadBaseFile();
					break;
				case ID_CANCELBTN:
					DestroyWindow(hWnd);
					return 0;
				case ID_CREATEBTN:
					sm->CreateSliderSet();					
					break;			
			}
			break;	
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}


INT_PTR CALLBACK UVDupThresholdInput(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];
	static string* s = NULL;
	switch (message)
	{
	case WM_INITDIALOG:
		s = (string*) lParam;
		SetWindowTextA(hDlg,"Set UV Duplicate Threshold");
		::SetDlgItemTextA(hDlg,IDC_STATIC,"Please enter a threshold value for duplicating verts based on UV:");
		::SetDlgItemTextA(hDlg,IDC_PROMPTINPUT,s->c_str());
		SendDlgItemMessage(hDlg,IDC_PROMPTINPUT,EM_SETSEL,0,-1);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hDlg, LOWORD(wParam));
			::GetDlgItemTextA(hDlg, IDC_PROMPTINPUT, buf,256);
			s->assign(buf);
			return (INT_PTR)TRUE;
			break;
		case IDC_CANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)FALSE;
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hDlg,IDC_CANCEL);
	}

	return (INT_PTR)FALSE;
}



INT_PTR CALLBACK SliderSetNameInput(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];
	static string* s = NULL;
	switch (message)
	{
	case WM_INITDIALOG:
		s = (string*) lParam;
		SetWindowTextA(hDlg,"Create Slider Set");
		::SetDlgItemTextA(hDlg,IDC_STATIC,"Please enter a name for the new Slider Set:");
		::SetDlgItemTextA(hDlg,IDC_PROMPTINPUT,s->c_str());
		SendDlgItemMessage(hDlg,IDC_PROMPTINPUT,EM_SETSEL,0,-1);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hDlg, LOWORD(wParam));
			::GetDlgItemTextA(hDlg, IDC_PROMPTINPUT, buf,256);
			s->assign(buf);
			return (INT_PTR)TRUE;
			break;
		case IDC_CANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)FALSE;
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hDlg,IDC_CANCEL);
	}

	return (INT_PTR)FALSE;
}


INT_PTR CALLBACK ExportSettingsInput(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];
	static vector<float>* params = NULL;
	switch (message)
	{
	case WM_INITDIALOG:
		params = (vector<float>*) lParam;
		_snprintf(buf,256,"%0.5f",params->at(0));
		::SetDlgItemTextA(hDlg,IDC_EXPORTSCALE,buf);
		_snprintf(buf,256,"%0.5f",params->at(1));
		::SetDlgItemTextA(hDlg,IDC_EXPORTOFFSETX,buf);
		_snprintf(buf,256,"%0.5f",params->at(2));
		::SetDlgItemTextA(hDlg,IDC_EXPORTOFFSETY,buf);
		_snprintf(buf,256,"%0.5f",params->at(3));
		::SetDlgItemTextA(hDlg,IDC_EXPORTOFFSETZ,buf);

		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hDlg, LOWORD(wParam));
			::GetDlgItemTextA(hDlg, IDC_EXPORTSCALE, buf,256);
			params->at(0) = atof(buf);
			::GetDlgItemTextA(hDlg, IDC_EXPORTOFFSETX, buf,256);
			params->at(1) = atof(buf);
			::GetDlgItemTextA(hDlg, IDC_EXPORTOFFSETY, buf,256);
			params->at(2) = atof(buf);
			::GetDlgItemTextA(hDlg, IDC_EXPORTOFFSETZ, buf,256);
			params->at(3) = atof(buf);
			return (INT_PTR)TRUE;
			break;
		case IDC_CANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)FALSE;
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hDlg,IDC_CANCEL);
	}

	return (INT_PTR)FALSE;
}

UINT CALLBACK DirSelectHook( HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam )
{

	switch( uiMsg )
	{
	//case WM_INITDIALOG:
	//return TRUE;

	case WM_NOTIFY:
		if( ( (LPNMHDR)lParam )->code == CDN_INITDONE )
		{
	//	ShowWindow( GetDlgItem( GetParent( hdlg ), 1152 ), FALSE );
	//	ShowWindow( GetDlgItem( GetParent( hdlg ), 1090 ), FALSE );
	//	ShowWindow( GetDlgItem( GetParent( hdlg ), 1089 ), FALSE );
	//	ShowWindow( GetDlgItem( GetParent( hdlg ), 1136 ), FALSE );
		}
		return TRUE;
	}
	return TRUE;
}