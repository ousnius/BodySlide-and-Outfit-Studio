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

#include <vector>
#include <string>
#include <map>

#include "Commctrl.h"
#include <string.h>
#include "slidermanager.h"
#include "PreviewWindow.h"
#include "SliderMaker.h"
#include "DiffData.h"
#include "SliderData.h"

#define ID_PREVIEW0 494
#define ID_PREVIEW1 495
#define ID_CREATE 496
#define ID_GENERATE 497
#define ID_OUTFITLIST 498
#define ID_PRESETLIST 499
#define ID_SAVEPRESET 500
#define ID_CLEANNIF 501
#define ID_BATCHBUILD 502

using namespace std;

class AppControl
{
/*  HFONT hAppFont;
	HWND hMainWnd;
	HINSTANCE hAppInstance;


	HWND mGenerateButton;
	HWND mCreateButton;
	HWND mMessageView;
	HWND mPreSetsList;
	HWND mOutfitList;
	HWND mCleanButton;
	HWND mBatchBuild;
	HWND mPreviewBtn;
	HWND mPreviewBtn1;
	string msgAppend;

	SliderManager sliderManager;
	DiffDataSets dataSets;

	map<string, string> config;
	map<string, string> outfitNameSource;
	vector<string> curGroups;
	string curOutfit;
// REPLACING with SliderData
	//map<string,string> targetShapeNames;
	//map<string,string> targetDataFolders;

	//string datafolder;
	//string inputfile;
	//string outputpath;
	//string outputfile;

	SliderSet activeSet;
	
	PreviewWindow* preview0;
	PreviewWindow* preview1;
	
	SliderMaker* sliderMaker;

	string previewBaseName;
	NifFile* previewBaseNif;
	NifFile PreviewMod;
*/
// 
/*	void setupOutfit(const string& outfitName);
	int createSliders(const string& outfit="CalienteBody", bool hideAll = false);
	int createSetSliders(const string& outfit, bool hideAll = false);
	int saveDiffData(const string& filename, map<int,vector3>& data);
	int loadDiffData(const string& filename, map<int,vector3>& data);
*/
public:
//	AppControl(void);
//	AppControl(HWND mainWnd, HINSTANCE appInstance);

/*	void LoadConfig();
	void LoadGroups(const string& setName);
	void LoadAllGroups(vector<string>& outGroups);
	void GetGroupMembers(const string& groupName, vector<string>& outMemberNames);
	void LoadPresets(const string& sliderSet);
	void PopulatePresetList();

	void SetApplicationFont(HFONT font) { hAppFont = font; }
	
	void ShowPreview(char PreviewType = SMALL_PREVIEW);
	void ClosePreview(char PreviewType = SMALL_PREVIEW) {
		if(PreviewType == SMALL_PREVIEW) {
			if(preview0) {
				delete preview0;	
				preview0=NULL;	
			}
			return;
		}
		if(preview1)
			delete preview1;
		preview1 = NULL;
	}
	void UpdatePreview(char PreviewType = SMALL_PREVIEW);

	int CreateControls();

	LRESULT ProcessScroll(HWND scrollID, int evt, int pos, LPARAM lParam);
	LRESULT ProcessNotify(NMHDR* nmHdr,WPARAM wParam);
	LRESULT ProcessCommand(int controlID, int evt, LPARAM lParam);
	void WindowResized(WORD width, WORD height);

	void SetMessage(string msg);

	void MakeDiffFile(string shapesFile, string baseShapeName, string outDiffPath);

	void MakeDiffFromObj(string baseShapeName, string objFile);

	int BlenderNifToSkyrim();
	
	int BuildBodies(bool localPath = false);
	int BuildAllInGroup(const string& groupName, bool dirPrompt = false);
	
	int SaveSliderPositions();
*/
//	~AppControl(void);
};
