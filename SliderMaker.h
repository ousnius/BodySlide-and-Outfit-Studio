/*
Caliente's Body Slide
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

#pragma once
#include "Stdafx.h"
#include "NifFile.h"
#include "previewWindow.h"

#define ID_SHAPESLIST 550
#define ID_DIFFLIST 551
#define ID_DIFFADDBTN 552
#define ID_DIFFDELBTN 553
#define ID_BASEFILELOADBTN 554
#define ID_SLIDERNAMEEDIT 555
#define ID_SLIDERSRCEDIT 556
#define ID_SLIDERSRCBTN 557
#define ID_SMALLPCTEDIT 558
#define ID_BIGPCTEDIT 559
#define ID_INVERTSLIDERCHECK 560
#define ID_SCALESRCEDIT 561
#define ID_BASEFILEEDIT 562
#define ID_CANCELBTN 563
#define ID_CREATEBTN 564
#define ID_SHAPEPREVIEWBTN 565
#define ID_HIDDENSLIDERCHECK 566
#define ID_SRCPREVIEWBTN 567
#define ID_GAMEPATHEDIT 568
#define ID_GAMEFILEEDIT 569
#define ID_SHAPEEXPORTBTN 570
#define ID_SHAPEREFBTN 571
#define ID_SRCMAKEDIFFBTN 572
#define ID_ZAPSLIDERCHECK 573
#define ID_LOADDIRBTN 574
 
LRESULT CALLBACK SliderMakerWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditControlTabHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ButtonControlTabHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK UVDupThresholdInput(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SliderSetNameInput(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ExportSettingsInput(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

class DiffItemInfo {
public:
	string shape;
	string name;
	string dataname;
	string srcfilename;
	string datafolder;
	bool invert;
	bool hidden;
	bool zap;
	bool clamp;
	float defSmall;
	float defBig;
	
	float similarity;
	float scale;

	int listItem;
	vector<vector3> verts;
	vector<vector3> OrderedVerts;
	vector<triangle> tris;

	DiffItemInfo () {
		srcfilename = "Load slider source data from .obj";
		shape = "<Shape Name>";
		name = "<Slider Name>";
		invert = false;
		hidden = false;
		zap = false;
		clamp=false;
		defSmall = 0;
		defBig = 1;
		similarity = 0;
		scale = 1.0f;
	}

	void SetDefaults() {
		if(name.find("BreastsSmall")!=string::npos) {
			defSmall = 1.0f; defBig=1.0f; invert=true; name = "BreastsSmall";
		} else 
		if(name.find("BreastCleavage")!=string::npos) {
			defSmall = 00.0f; defBig=00.0f; invert=false; name = "BreastCleavage";
		} else
		if(name.find("BreastsSSH")!=string::npos) {
			defSmall = 00.0f; defBig=00.0f; invert=false; name = "BreastsSSH";
		} else
		if(name.find("BreastsSH")!=string::npos) {
			defSmall = 00.0f; defBig=00.0f; invert=false; name = "BreastsSH";
		} else 
		if(name.find("Breasts")!=string::npos) {
			defSmall = .20f; defBig=1.0f; invert=true; name = "Breasts";
		} else 
		if(name.find("NippleSize")!=string::npos) {
			defSmall = 1.0f; defBig=1.0f; invert=false; name = "NippleSize";
		} else 
		if(name.find("ButtSmall")!=string::npos) {
			defSmall = 1.0f; defBig=1.0f; invert=true; name = "ButtSmall";
		} else 
		if(name.find("MakButt")!=string::npos) {
			defSmall = 0.0; defBig=0.0f; invert=false; name = "ButtShape2";
		} else 
		if(name.find("ButtShape2")!=string::npos) {
			defSmall = 0.0; defBig=0.0f; invert=false; name = "ButtShape2";
		} else 
		if(name.find("Butt")!=string::npos) {
			defSmall = 0.0f; defBig=1.0f; invert=true; name = "Butt";
		} else 
		if(name.find("Waist")!=string::npos) {
			defSmall = 0.0f; defBig=1.0f; invert=false; name = "Waist";
		} else 
		if(name.find("Legs")!=string::npos) {
			defSmall = 0.0f; defBig=1.0f; invert=true; name = "Legs";
		} else 
		if(name.find("Arms")!=string::npos) {
			defSmall = 0.0f; defBig=1.0f; invert=true; name = "Arms";
		} else 
		if(name.find("ShoulderWidth")!=string::npos) {
			defSmall = 1.0f; defBig=1.0f; invert=true; name = "ShoulderWidth";
		} else 
		if(name.find("Belly")!=string::npos) {
			defSmall = 0.0; defBig=0.0f; invert=false; name = "Belly";
		} else 
		if(name.find("Thighs")!=string::npos) {
			defSmall = 0.0; defBig=0.0f; invert=false; name = "Thighs";
		} else 
		if(name.find("VanillaLo")!=string::npos) {
			defSmall = 1.0; defBig=0.0f; invert=false; name = "Vanilla0";
		} else 
		if(name.find("Vanilla0")!=string::npos) {
			defSmall = 1.0; defBig=0.0f; invert=false; name = "Vanilla0";
		} else 
		if(name.find("VanillaHi")!=string::npos) {
			defSmall = 0.0; defBig=1.0f; invert=false; name = "Vanilla1";
		} else 
		if(name.find("Vanilla1")!=string::npos) {
			defSmall = 0.0; defBig=1.0f; invert=false; name = "Vanilla1";
		} else
		if(name.find("PointCheck")!=string::npos) {
			defSmall = 0.0; defBig=0.0f; invert=false; name = "PointCheck";
		} else
		if(name.find("SmallSeam")!=string::npos) {
			defSmall = 1.0; defBig=0.0f; invert=false; hidden=true; name = "SmallSeam";
		} else
		if(name.find("seamsize")!=string::npos) {
			defSmall = 1.0; defBig=0.0f; invert=false; hidden=true; name = "SmallSeam";
		}
	}
};

class SliderRefInfo {
public:
	string targetName;
	string datafolder;
	map<string,string> sliderdatafiles;
};

class SliderMaker
{
	static bool has_registered;
	HWND mHwnd;
	HINSTANCE hAppInstance;
	HFONT windowFont;
	
	HWND mToolTipWnd;
	HWND mBaseFileEdit;
	HWND mBaseFileLoadBtn;
	HWND mShapesList;
	HWND mDiffList;
	HWND mAddBtn;
	HWND mDelBtn;
	HWND mLoadDirBtn;
	HWND mSliderNameEdit;
	HWND mSliderSrcEdit;
	HWND mSliderSrcBtn;
	HWND mSmallPctEdit;
	HWND mBigPctEdit;
	HWND mInvertSliderCheck;
	HWND mHiddenSliderCheck;
	HWND mScaleSrcEdit;
	HWND mCancelBtn;
	HWND mCreateBtn;
	HWND mShapePreviewBtn;
	HWND mShapeRefBtn;
	HWND mShapeExportBtn;
	HWND mSrcPreviewBtn;
	HWND mSrcMakeDiffBtn;
	HWND mGameFileEdit;
	HWND mGamePathEdit;
	HWND mZapSliderCheck;


	NifFile baseNif;
	PreviewWindow* shapePreview;
	PreviewWindow* sourcePreview;
	PreviewWindow* automorphPreview;

	map<string, string>* config;
	bool selfconfig;
	
	void registerClass();

	//map<string,SliderRefInfo> sliderRefs;
	vector<DiffItemInfo> sliderRefs;

public:
	HWND hOwner;
	SliderMaker(void);
	SliderMaker(HWND owner, map<string, string>* app_config = NULL);
	void SetConfigDefaults();
	~SliderMaker(void);
	
	void EnableEditingControls(BOOL bEnable) {
		EnableWindow(mDelBtn,bEnable);
		EnableWindow(mSliderNameEdit,bEnable);
		EnableWindow(mSliderSrcEdit,bEnable);
		EnableWindow(mSliderSrcBtn,bEnable);
		EnableWindow(mSmallPctEdit,bEnable);
		EnableWindow(mBigPctEdit,bEnable);
		EnableWindow(mInvertSliderCheck,bEnable);
		EnableWindow(mHiddenSliderCheck,bEnable);
		EnableWindow(mZapSliderCheck,bEnable);
		EnableWindow(mScaleSrcEdit,bEnable);
		EnableWindow(mCreateBtn,bEnable);

	}

	void SetupToolTips();

	void UpdateFromSelection() ;

	void CreateControls();
	void NextControl(HWND curControl, bool back) {
		if(back)
			SetFocus(GetNextWindow(curControl,GW_HWNDPREV));
		else
			SetFocus(GetNextWindow(curControl,GW_HWNDNEXT));
	}

	int LoadBaseFile();
	int LoadSlidersFromDirectory();
	int LoadSliderSource(const string& filename="", bool createDiffEntry = false);
	int LoadSliderSourceData(fstream& base, DiffItemInfo* dii, const string& shapename, const string& groupname);

	int AutoShapeBaseFile();

	DiffItemInfo* AddSliderEntry(const string& shape="", const string& slidername="");
	void DelSliderEntry();
	void SetCurrentShape();
	void SetCurrentSliderName();
	void SetCurrentSliderScale();
	void SetDefaultSliderScale() {
		char buf[25];
		GetWindowTextA(mScaleSrcEdit,buf,25);
		(*config)["DefaultObjScale"] = buf;
	}
	void SetBigPercent();
	void SetSmallPercent();
	void SetObjScale();
	void SetSliderInvert();
	void SetSliderHidden();
	void SetZapSource();
	void PreviewCurrentShape(bool showOnlySelected = true);
	void PreviewSrcZap(DiffItemInfo* dii);
	void PreviewCurrentSrc();
	void ClosePreview(PreviewWindow* which) ;
	
	int MakeCurrentSrcDiff();
	int ExportCurrentShape(bool exportAll=false);
	void ConfigUVDupThreshold();
	int LoadCurrentShapeRef();
	
	int CreateSliderSet();
	int saveDiffData(const string& filename, unordered_map<int,vector3>& data);

	float CalcSimilarity(DiffItemInfo* dii,const vector<vector3>* baseverts);

	vector<DiffItemInfo*> diffTracker;

	void Close();

	bool HasFocus() {
		return(mHwnd == GetFocus());
	}

};

UINT CALLBACK DirSelectHook( HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam );