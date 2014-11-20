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

#include "StdAfx.h"
#include "AppControl.h"

#include "Commctrl.h"
#include "Commdlg.h"
#include <string.h>
#include <fstream>
#include <sstream>
#include <set>

#include "niffile.h"
#include "sliderdata.h"


#include "tinyxml.h"
#include "shlobj.h"

struct PresetSaveInfo {
	string promptResult;
	vector<string> in_out_groups;
};

INT_PTR CALLBACK	SimpleInput(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ChooseBatchGroup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

AppControl::AppControl(void)
{
	preview0 = NULL;
	preview1 = NULL;
	previewBaseNif = NULL;
}

AppControl::AppControl(HWND mainWnd, HINSTANCE appInstance) {
	hMainWnd = mainWnd;
	hAppInstance = appInstance;
	
	preview0 = NULL;
	preview1 = NULL;
	previewBaseNif = NULL;
	INITCOMMONCONTROLSEX ccinit;
	ccinit.dwICC = ICC_LISTVIEW_CLASSES;
	ccinit.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCommonControlsEx(&ccinit);
	LoadConfig();
	curGroups.clear();
	curGroups.push_back("Body");
	LoadPresets("CalienteBody");
}

AppControl::~AppControl(void)
{
	if(preview0) 
		delete preview0;
	if(preview1) 
		delete preview1;

}

void AppControl::LoadConfig() {
	HKEY skyrimRegKey;
	char installPath[256];
	DWORD pathSize = 256;
	string buildpath;

	long result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Bethesda Softworks\\Skyrim",0,KEY_READ,&skyrimRegKey);
	if(result == ERROR_SUCCESS) {
		RegQueryValueEx(skyrimRegKey,"Installed Path",0,0,(BYTE*)installPath,&pathSize);
		buildpath = installPath;
		buildpath += "Data\\";
		config["SkyrimDataPath"] = buildpath;
	}

	TiXmlDocument doc("Config.xml");
	TiXmlElement* root;
	TiXmlElement* e;
	if(doc.LoadFile()) {
		root = doc.FirstChildElement("BodySlideConfig");
		if(!root) return;
		
		e = root->FirstChildElement("GameDataPath");
		if(e) {
			config["SkyrimDataPath"] = e->GetText();
		}
		
		e = root->FirstChildElement("WarnMissingGamePath");
		if(e) {
			config["WarnMissingSkyrimPath"] == e->GetText();
		} else {
			config["WarnMissingSkyrimPath"] = "true";
		}
		
		e = root->FirstChildElement("MeshOutputPath");
		if(e) {
			config["MeshOutputPath"] = e->GetText();
		} else {
			config["MeshOutputPath"] = config["SkyrimDataPath"];
		}	

		e = root->FirstChildElement("ShapeDataPath");
		if(e) {
			config["ShapeDataPath"] = e->GetText();
		} else {
			config["ShapeDataPath"] = ".\\ShapeData\\";
		}	

		e = root->FirstChildElement("EnableGenerator");
		if(e){
			string s = e->GetText();
			if(s == "true"){
				config["EnableGenerator"] = e->GetText();
			}
		} 
		e = root->FirstChildElement("GeneratorFile");
		if(e) {
			config["GeneratorFile"] = e->GetText();
		}
		e = root->FirstChildElement("GenBaseShapeName");
		if(e) {
			config["GenBaseShapeName"] = e->GetText();
		}
		e = root->FirstChildElement("BatchBuildGroup");
		if(e) {
			config["BatchBuildGroup"] = e->GetText();
		} else {
			config["BatchBuildGroup"] = "CalienteArmor";
		}

	}
}

void AppControl::LoadGroups(const string& setName) {
	TiXmlDocument doc;
	TiXmlElement* root;
	TiXmlElement* e;
	TiXmlElement* m;

	WIN32_FIND_DATA wfd;
	HANDLE hfind;
	hfind = FindFirstFile("SliderGroups\\*.xml", &wfd);
	DWORD searchStatus = 0;
	string filename;

	curGroups.clear();

	if(hfind ==INVALID_HANDLE_VALUE) {
		return;
	}
	while(searchStatus != ERROR_NO_MORE_FILES) {
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else{		
			filename = "SliderGroups\\";
			filename += wfd.cFileName;
			if(doc.LoadFile(filename.c_str())) {	
				root = doc.FirstChildElement("SliderGroups");
				if(!root) return ;
				e = root->FirstChildElement("Group");
				while(e) {
					m = e->FirstChildElement("Member");
					while(m) {
						if(m->Attribute("name") == setName) {
							curGroups.push_back(e->Attribute("name"));
							break;
						}
						m = m->NextSiblingElement("Member");
					}					
					e = e->NextSiblingElement("Group");
				}
			}		
		}
		if(!FindNextFile(hfind,&wfd)) {
			searchStatus = GetLastError();
		} else 
			searchStatus = 0;
	}
	FindClose(hfind);
}

void AppControl::LoadAllGroups(vector<string>& outGroups) {
	TiXmlDocument doc;
	TiXmlElement* root;
	TiXmlElement* e;
	TiXmlElement* m;

	WIN32_FIND_DATA wfd;
	HANDLE hfind;
	hfind = FindFirstFile("SliderGroups\\*.xml", &wfd);
	DWORD searchStatus = 0;
	string filename;

	curGroups.clear();
	set<string> uniqueGroups;

	if(hfind ==INVALID_HANDLE_VALUE) {
		return;
	}
	while(searchStatus != ERROR_NO_MORE_FILES) {
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else{		
			filename = "SliderGroups\\";
			filename += wfd.cFileName;
			if(doc.LoadFile(filename.c_str())) {	
				root = doc.FirstChildElement("SliderGroups");
				if(!root) return ;
				e = root->FirstChildElement("Group");
				while(e) {				
					if(uniqueGroups.find(e->Attribute("name")) == uniqueGroups.end()) {
						outGroups.push_back(e->Attribute("name"));
						uniqueGroups.insert(e->Attribute("name"));
					}
					e = e->NextSiblingElement("Group");
				}
			}		
		}
		if(!FindNextFile(hfind,&wfd)) {
			searchStatus = GetLastError();
		} else 
			searchStatus = 0;
	}
	FindClose(hfind);
}


void AppControl::GetGroupMembers(const string& groupName, vector<string>& outMemberNames) {
	TiXmlDocument doc;
	TiXmlElement* root;
	TiXmlElement* e;
	TiXmlElement* m;

	WIN32_FIND_DATA wfd;
	HANDLE hfind;
	hfind = FindFirstFile("SliderGroups\\*.xml", &wfd);
	DWORD searchStatus = 0;
	string filename;

	curGroups.clear();

	if(hfind ==INVALID_HANDLE_VALUE) {
		return;
	}
	while(searchStatus != ERROR_NO_MORE_FILES) {
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else{		
			filename = "SliderGroups\\";
			filename += wfd.cFileName;
			if(doc.LoadFile(filename.c_str())) {	
				root = doc.FirstChildElement("SliderGroups");
				if(!root) return ;
				e = root->FirstChildElement("Group");
				while(e) {
					if(e->Attribute("name") == groupName) {
						m = e->FirstChildElement("Member");
						while(m) {
							outMemberNames.push_back(m->Attribute("name"));
							m = m->NextSiblingElement("Member");
						}					
					}
					e = e->NextSiblingElement("Group");
				}
			}		
		}
		if(!FindNextFile(hfind,&wfd)) {
			searchStatus = GetLastError();
		} else 
			searchStatus = 0;
	}
	FindClose(hfind);
}

void AppControl::LoadPresets(const string& sliderSet) {
	
	TiXmlDocument doc;
	TiXmlElement* root;
	TiXmlElement* e;
	TiXmlElement* g;
	TiXmlElement* setslider;

	string presetName, sliderName, applyTo;
	double o, b, s;


	
	WIN32_FIND_DATA wfd;
	HANDLE hfind;
	hfind = FindFirstFile("SliderPresets\\*.xml", &wfd);
	DWORD searchStatus = 0;
	string filename;

	if(hfind ==INVALID_HANDLE_VALUE) {
		return;
	}
	while(searchStatus != ERROR_NO_MORE_FILES) {
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else{
			filename = "SliderPresets\\";
			filename += wfd.cFileName;
			if(doc.LoadFile(filename.c_str())) {		
				root = doc.FirstChildElement("SliderPresets");
				if(!root) return ;
				e = root->FirstChildElement("Preset");
				while(e) {
					bool skip = true;
					g = e->FirstChildElement("Group");
					while(g) {
						for(int i=0;i<curGroups.size();i++) {
							if(g->Attribute("name") == curGroups[i]) {
								skip = false;
								break;
							}
						}
						g=g->NextSiblingElement("Group");
					}
					if(e->Attribute("set") == sliderSet) {
						skip = false;
					}

					if(skip) {
						e = e->NextSiblingElement("Preset");
						continue;
					}
					presetName = e->Attribute("name");
					setslider = e->FirstChildElement("SetSlider");
					while(setslider) {
						sliderName = setslider->Attribute("name");
						applyTo = setslider->Attribute("size");
						setslider->Attribute("value",&o);
						o = o/100.0f;
						s=b=-10000;
						if(applyTo == "small") {
							s = o;
						} else if( applyTo =="big") {
							b = o;
						}else if(applyTo=="both") {
							s=b=o;
						}
						sliderManager.SetSliderPreset(presetName,sliderName,b,s);
						setslider = setslider->NextSiblingElement("SetSlider");
					}
					e = e->NextSiblingElement("Preset");
				}
			}
		}
		if(!FindNextFile(hfind,&wfd)) {
			searchStatus = GetLastError();
		} else 
			searchStatus = 0;
	}
	FindClose(hfind);

}
void AppControl::setupOutfit(const string& outfitName) {

	SendMessage(mOutfitList,CB_RESETCONTENT,0,0);
	SendMessage(mPreSetsList,CB_RESETCONTENT,0,0);
	if(preview0) {
		preview0->Close();
	}
	if(preview1) {
		preview1->Close();
	}
//	targetShapeNames.clear();
//	targetDataFolders.clear();
	sliderManager.ClearSliders();
	outfitNameSource.clear();
	sliderManager.ClearPresets();
	LoadGroups(outfitName);
	LoadPresets(outfitName);
	PopulatePresetList();
	createSliders(outfitName);
}
/*
int AppControl::createSliders(const string& outfit, bool hideAll) {
	sliderManager.SetOwnerInfo(hMainWnd,hAppInstance,hAppFont);
		
	TiXmlDocument doc;
	TiXmlElement* root;
	TiXmlElement* setsearch;
	TiXmlElement* e;
	TiXmlElement* datafile;
	TiXmlElement* shapename;
	TiXmlElement* requirement;
	string slidername;
	string slidersetname;
	string datatarget, datafilename, dataname;
	bool isInverse, isHidden, isZap; double bval, sval;

	dataSets.Clear();
	//sliderManager.ZapDataLinks.clear();

	WIN32_FIND_DATA wfd;
	HANDLE hfind;
	hfind = FindFirstFile("SliderSets\\*.xml", &wfd);
	DWORD searchStatus = 0;
	string filename;

	if(hfind ==INVALID_HANDLE_VALUE) {
		return 3;
	}

	while(searchStatus != ERROR_NO_MORE_FILES) {
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else{
			filename = "SliderSets\\";
			filename += wfd.cFileName;

			if(doc.LoadFile(filename.c_str())) {
				root = doc.FirstChildElement("SliderSetInfo");
				if(!root) goto getnext;
				setsearch = root->FirstChildElement("SliderSet");
				e = NULL;
				while(setsearch) {
					slidersetname = setsearch->Attribute("name");
					outfitNames.push_back(slidersetname);
					if(slidersetname == outfit)
						e = setsearch;
					setsearch = setsearch->NextSiblingElement("SliderSet");
				}
				if(!e) goto getnext;
				
				curOutfit = outfit;
				
				datafolder = e->FirstChildElement("SetFolder")->GetText();
				inputfile = e->FirstChildElement("SourceFile")->GetText();
				outputpath = e->FirstChildElement("OutputPath")->GetText();
				outputfile = e->FirstChildElement("OutputFile")->GetText();

				shapename = e->FirstChildElement("BaseShapeName");
				while(shapename) {
					targetShapeNames[shapename->Attribute("target")] = shapename->GetText();
					if(shapename->Attribute("DataFolder"))
						targetDataFolders[shapename->Attribute("target")] = shapename->Attribute("DataFolder");
					else
						targetDataFolders[shapename->Attribute("target")] = datafolder;
					shapename=shapename->NextSiblingElement("BaseShapeName");
				}

				e = e->FirstChildElement("Slider");
				while(e) {
					slidername = e->Attribute("name");
					if(e->Attribute("invert")) {
						isInverse = (_strnicmp(e->Attribute("invert"),"true",4) == 0);
					} else 
						isInverse = false;

					e->Attribute("big",&bval);
					e->Attribute("small",&sval);

					isHidden=hideAll;
					if(e->Attribute("hidden")) {
						if(_strnicmp(e->Attribute("hidden"),"true",4) == 0) {
							isHidden = true;
						}
					} 
					isZap = false;
					if(e->Attribute("zap")) {
						if(_strnicmp(e->Attribute("zap"),"true",4) == 0) {
							isZap = true;
						}
					} 
		 
					if(!isHidden)
						if(!isZap) {
							sliderManager.AddSlider(slidername,isInverse);						
						} else {
							sliderManager.AddZapSlider(slidername);	
						}
				
					else {
						sliderManager.AddHiddenSlider(slidername,isInverse,isZap);
					}
					sliderManager.SetSliderDefaults(slidername,bval/100.0f,sval/100.0f);
					
					if(e->Attribute("clamp")) {
						if(_strnicmp(e->Attribute("clamp"),"true",4) == 0) {
							sliderManager.SetClampSlider(slidername);
						}
					} 


					datafile = e->FirstChildElement("datafile");
					while(datafile) {
						datatarget = datafile->Attribute("target"); 
						if(datafile->Attribute("local")) {
							if(_strnicmp(datafile->Attribute("local"),"true",4)==0) {
								datafilename = config["ShapeDataPath"]+ "\\" + datafolder + "\\" + datafile->GetText();						
							}

						} else {
							datafilename = config["ShapeDataPath"]+ "\\" + targetDataFolders[datatarget] + "\\" + datafile->GetText();
						}
						if(datafile->Attribute("name")) {
							dataname = datafile->Attribute("name");
						} else dataname = slidername;
						
						dataSets.LoadSet(dataname,datatarget,datafilename);
						//if(!isZap) 
							sliderManager.AddSliderLink(slidername,dataname);
						//else 
						//	sliderManager.AddZapData(dataname);

						datafile = datafile->NextSiblingElement("datafile");
					} 

					requirement = e->FirstChildElement("require");
					while(requirement) {
						requirement->Attribute("value",&bval);
						sliderManager.AddSliderTrigger(requirement->Attribute("slidername"),
							slidername,(float)bval/100.0f,0);
						requirement = requirement->NextSiblingElement("require");
					}
					
					e=e->NextSiblingElement("Slider");
				}
				sliderManager.InitializeSliders("CBBE");
getnext:
				if(!FindNextFile(hfind,&wfd)) {
					searchStatus = GetLastError();
				} else 
					searchStatus = 0;

			}
		}
	}
	FindClose(hfind);
	for(int i=0;i<outfitNames.size();i++){ 
		SendMessage(mOutfitList,CB_ADDSTRING,0,(LPARAM)outfitNames[i].c_str());
	}
	int sel = SendMessage(mOutfitList,CB_FINDSTRINGEXACT,0,(LPARAM)outfit.c_str());
	SendMessage(mOutfitList,CB_SETCURSEL,sel,0);

	//const vector<vector3>* baseVerts;
	//vector<vector3> verts;
	//NifFile f;
	//f.Load("ShapeData\\CalienteBody\\CalienteBody.nif");
	//baseVerts = f.GetVertsForShape("BaseShape");
	//for(int j=0;j<baseVerts->size();j++) {
	//	verts.push_back(baseVerts->at(j));
	//}
	//dataSets.ApplyDiff("Breasts",1.0f,&verts);
	//dataSets.ApplyDiff("BreastsSmall",1.0f,&verts);
	////dataSets.ApplyDiff("BreastsSSH",1.0f,&verts);
	//f.SetVertsForShape("BaseShape",verts);
	//f.Save("OUTPUT.nif");
	return 0;

}
*/

int AppControl::createSetSliders(const string& outfit, bool hideAll) {
	if(outfitNameSource.find(outfit) == outfitNameSource.end()) {
		return 1;
	}

	SliderSetFile sliderDoc;
	sliderDoc.Open(outfitNameSource[outfit]);
	if(!sliderDoc.fail()) {
		activeSet.Clear();
		if(sliderDoc.GetSet(outfit,activeSet)) {
			curOutfit = outfit;
			activeSet.SetBaseDataPath(config["ShapeDataPath"]);

			activeSet.LoadSetDiffData(dataSets);

			sliderManager.AddSlidersInSet(activeSet,hideAll);

		} else {
			return 2;
		}
	}	
	return 0;

}


int AppControl::createSliders(const string& outfit, bool hideAll) {
	sliderManager.SetOwnerInfo(hMainWnd,hAppInstance,hAppFont);


	WIN32_FIND_DATA wfd;
	HANDLE hfind;
	string filename;
	DWORD searchStatus = 0;
	vector<string> outfitNames;

	dataSets.Clear();

	activeSet.Clear();

	hfind = FindFirstFile("SliderSets\\*.xml", &wfd);

	if(hfind ==INVALID_HANDLE_VALUE) {
		return 3;
	}

	while(searchStatus != ERROR_NO_MORE_FILES) {
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else{
			filename = "SliderSets\\";
			filename += wfd.cFileName;

			SliderSetFile sliderDoc;
			sliderDoc.Open(filename);

			if(!sliderDoc.fail()) {
				sliderDoc.GetSetNames(outfitNames,false);
				for(int i=0;i<outfitNames.size();i++) {
					outfitNameSource[outfitNames[i]] = filename;
				}
				if(!sliderDoc.GetSet(outfit,activeSet)) {
					curOutfit = outfit;
					activeSet.SetBaseDataPath(config["ShapeDataPath"]);

					activeSet.LoadSetDiffData(dataSets);

					sliderManager.AddSlidersInSet(activeSet, hideAll);
					sliderManager.InitializeSliders("CBBE");

				}
			}
			if(!FindNextFile(hfind,&wfd)) {
				searchStatus = GetLastError();
			} else 
				searchStatus = 0;

		}
	}
	FindClose(hfind);
	map<string,string>::iterator outfitIter;
	for(outfitIter = outfitNameSource.begin();outfitIter!=outfitNameSource.end();++outfitIter){ 
		SendMessage(mOutfitList,CB_ADDSTRING,0,(LPARAM)outfitIter->first.c_str());
	}
	int sel = SendMessage(mOutfitList,CB_FINDSTRINGEXACT,0,(LPARAM)outfit.c_str());
	SendMessage(mOutfitList,CB_SETCURSEL,sel,0);

	return 0;

}

void  AppControl::PopulatePresetList() {
	vector<string> presets;
	sliderManager.GetPresetNames(presets);
	for(int i=0;i<presets.size();i++){ 
		SendMessage(mPreSetsList,CB_ADDSTRING,0,(LPARAM)presets[i].c_str());
	}
	SendMessage(mPreSetsList,CB_SELECTSTRING,0,(LPARAM)"CBBE");
}

int AppControl::CreateControls() {	
	
	int i;

	HWND setlistlabel = CreateWindow("STATIC","Outfit/Body:", WS_CHILD | WS_VISIBLE, 10, 13, 70, 30, hMainWnd, 0,hAppInstance,NULL);
	SendMessage(setlistlabel,WM_SETFONT,(WPARAM)hAppFont,0);

    mOutfitList = CreateWindow("ComboBox","OutfitList",WS_CHILD | WS_VISIBLE | WS_VSCROLL |CBS_DROPDOWNLIST , 80,10,200,500,hMainWnd,(HMENU)ID_OUTFITLIST,hAppInstance,NULL);
    SendMessage(mOutfitList,WM_SETFONT, (WPARAM)hAppFont,0);
	

	HWND typelistlabel = CreateWindow("STATIC","Preset:", WS_CHILD | WS_VISIBLE, 10, 43, 40, 30, hMainWnd, 0,hAppInstance,NULL);
	SendMessage(typelistlabel,WM_SETFONT,(WPARAM)hAppFont,0);

	mPreSetsList = CreateWindow("ComboBox","PreSetsList",WS_CHILD | WS_VISIBLE| WS_VSCROLL  | CBS_DROPDOWNLIST , 80,40,200,500,hMainWnd,(HMENU)ID_PRESETLIST,hAppInstance,NULL);
	SendMessage(mPreSetsList,WM_SETFONT, (WPARAM)hAppFont,0);

	HWND SavePresetBtn = CreateWindow("BUTTON","Save Preset...",WS_CHILD | WS_VISIBLE, 290, 38, 100, 25, hMainWnd, (HMENU)ID_SAVEPRESET,hAppInstance,NULL);
	SendMessage(SavePresetBtn,WM_SETFONT,(WPARAM)hAppFont,0);
	
	PopulatePresetList();

	HWND smallSideLabel = CreateWindowEx(WS_EX_TRANSPARENT,"STATIC","Low Weight", WS_CHILD | WS_VISIBLE | SS_CENTER, 80, 80, 200, 25, hMainWnd, 0,hAppInstance,NULL);
//	SendMessage(smallSideLabel,WM_SETFONT,(WPARAM)hAppFont,0);


	HWND bigSideLabel = CreateWindowEx(WS_EX_TRANSPARENT,"STATIC","High Weight", WS_CHILD | WS_VISIBLE | SS_CENTER, 450, 80, 200, 25, hMainWnd, 0,hAppInstance,NULL);
//	SendMessage(bigSideLabel,WM_SETFONT,(WPARAM)hAppFont,0);


    createSliders();
	mCreateButton = CreateWindow("BUTTON","Create Bodies", WS_CHILD | WS_VISIBLE, 300, 440, 128, 50, hMainWnd, (HMENU)ID_CREATE,hAppInstance,NULL);
	SendMessage(mGenerateButton,WM_SETFONT,(WPARAM)hAppFont,0);


	if(config.find("EnableGenerator") != config.end()) {
		mGenerateButton = CreateWindow("BUTTON","Open Slider Maker...", WS_CHILD | WS_VISIBLE, 570, 480, 128, 25, hMainWnd, (HMENU)ID_GENERATE,hAppInstance,NULL);
		SendMessage(mGenerateButton,WM_SETFONT,(WPARAM)hAppFont,0);
	}

		 mPreviewBtn = CreateWindow("BUTTON","Preview Small", WS_CHILD | WS_VISIBLE,120, 440, 128, 25, hMainWnd, (HMENU)ID_PREVIEW0,hAppInstance,NULL);
		SendMessage(mPreviewBtn ,WM_SETFONT,(WPARAM)hAppFont,0);	
		 mPreviewBtn1 = CreateWindow("BUTTON","Preview Big", WS_CHILD | WS_VISIBLE, 480, 440, 128, 25, hMainWnd, (HMENU)ID_PREVIEW1,hAppInstance,NULL);
		SendMessage(mPreviewBtn1 ,WM_SETFONT,(WPARAM)hAppFont,0);

		mBatchBuild = CreateWindow("BUTTON","BatchBuild...", WS_CHILD | WS_VISIBLE, 10, 480, 128, 25, hMainWnd, (HMENU)ID_BATCHBUILD,hAppInstance,NULL);
		SendMessage(mBatchBuild,WM_SETFONT,(WPARAM)hAppFont,0);
/*
		mCleanButton = CreateWindow("BUTTON","Clean Nif...", WS_CHILD | WS_VISIBLE, 10, 480, 128, 25, hMainWnd, (HMENU)ID_CLEANNIF,hAppInstance,NULL);
		SendMessage(mCleanButton,WM_SETFONT,(WPARAM)hAppFont,0);

	
   	mMessageView = CreateWindow("STATIC","  Ready", WS_CHILD | WS_VISIBLE | SS_SUNKEN, 0, 595, 714,17, hMainWnd,0,hAppInstance,NULL);
	SendMessage(mMessageView,WM_SETFONT,(WPARAM)hAppFont,0);
   */
   return 0;

}

	


LRESULT AppControl::ProcessNotify(NMHDR* nmHdr, WPARAM wParam) {
	switch(nmHdr->idFrom) {
	case 0:			
	default:
		break;
	}
	return TRUE;
}

LRESULT AppControl::ProcessScroll(HWND scrollID, int evt, int pos, LPARAM lParam) {
	int amt;
	RECT r;
	GetClientRect(hMainWnd,&r);
	int curScroll,min,max;
	
	r.top = r.top + 95;
	r.bottom = r.bottom - 70;
	if(scrollID == NULL ) {
		SendMessage(hMainWnd,WM_SETREDRAW,FALSE,0);
		amt=0;
		curScroll =  GetScrollPos(hMainWnd,SB_VERT);
		GetScrollRange(hMainWnd,SB_VERT,&min,&max);
		if(evt == SB_THUMBTRACK || evt==SB_THUMBPOSITION) 
				return TRUE; 
			//amt = curScroll-pos;
		if(evt == SB_LINEDOWN || evt == SB_PAGEDOWN) {
			if(curScroll+30>= max) 
				return TRUE;
			amt=-30;
		}
		if(evt == SB_LINEUP || evt == SB_PAGEUP) {
			if(curScroll <= 0)
				return TRUE;
			amt=30;
		}
		if(evt == SB_BOTTOM) {
			amt = max - curScroll;
		}
		sliderManager.ScrollSliders(amt);
		SendMessage(hMainWnd,WM_SETREDRAW,TRUE,0);
		RedrawWindow(hMainWnd,&r,NULL,RDW_ERASE | RDW_INVALIDATE);
		return TRUE;
	}
	if(evt == SB_THUMBTRACK) {
		sliderManager.SetSlider((HWND)scrollID,(float)pos/100.0f);
	} else {
		switch(evt){ 
			case TB_TOP:
			case TB_BOTTOM:
			case TB_PAGEDOWN:
			case TB_PAGEUP:
			case TB_LINEUP:
			case TB_LINEDOWN:
			case TB_ENDTRACK:
				amt = SendMessage(scrollID,TBM_GETPOS,0,0);
				sliderManager.SetSlider(scrollID,(float)amt/100.0f);
				break;
		}
	}
	if(sliderManager.SliderIsSmall(scrollID)) {
		UpdatePreview(SMALL_PREVIEW);
	} else 
		UpdatePreview(BIG_PREVIEW);
	return TRUE;
}
LRESULT AppControl::ProcessCommand(int controlID, int evt, LPARAM lParam) {
	char buffer[256];
	HCURSOR hc;
	bool localpath;
	int ret;
	if(controlID >= 200 && controlID <=400) {
		if(evt == EN_KILLFOCUS) {
			if(sliderManager.SetSliderFromEditControlID(controlID)) {				
				if(preview0) {
					UpdatePreview(SMALL_PREVIEW);
				}
				if(preview1) {
					UpdatePreview(BIG_PREVIEW);
				}
			}
		}	
	}
	switch(controlID) {
		case ID_OUTFITLIST:			
			if(evt == CBN_SELCHANGE) {				
				GetWindowTextA(mOutfitList,buffer,256);
				hc = SetCursor(LoadCursor(NULL,IDC_WAIT));
				SendMessage(hMainWnd,WM_SETREDRAW,FALSE,0);
				setupOutfit(buffer);
				SendMessage(hMainWnd,WM_SETREDRAW,TRUE,0);
				RedrawWindow(hMainWnd,NULL,NULL,RDW_ERASE|RDW_INVALIDATE);
				SetCursor(hc);
			}
			break;
		case ID_PRESETLIST:			
			if(evt == CBN_SELCHANGE) {				
				GetWindowTextA(mPreSetsList,buffer,256);
				sliderManager.InitializeSliders(buffer);
			}
			if(preview0) {
				UpdatePreview(SMALL_PREVIEW);
			}
			if(preview1) {
				UpdatePreview(BIG_PREVIEW);
			}
			break;
		case ID_SAVEPRESET:
			SaveSliderPositions();
			break;
		case ID_GENERATE:
			sliderMaker = new SliderMaker(hMainWnd);
			//MakeDiffFromObj	("","");
			//MakeDiffFile(config["GeneratorFile"],config["GenBaseShapeName"],"");
			//MessageBox(NULL,"Done!","Done!",MB_OK);
			break;
		case ID_CLEANNIF:
			BlenderNifToSkyrim();
			break;
		case ID_BATCHBUILD:
			ret = BuildAllInGroup(config["BatchBuildGroup"],(GetKeyState(VK_CONTROL) & 0x80));
			if(ret > 0){
				MessageBox(NULL,"Failure to save bodies.","Caliente's Body Slide",MB_OK|MB_ICONERROR);				
			} else if (ret==0) {
				MessageBox(NULL,"Bodies Created Successfully!","Caliente's Body Slide",MB_OK|MB_ICONINFORMATION);
			}
			break;
		case ID_CREATE:
			if(GetKeyState(VK_CONTROL) & 0x80)
				localpath = true;
			else
				localpath = false;

			if(BuildBodies(localpath) == 0) {
				MessageBox(NULL,"Bodies Created Successfully!","Caliente's Body Slide",MB_OK|MB_ICONINFORMATION);
			}else {
				MessageBox(NULL,"Failure to save bodies.","Caliente's Body Slide",MB_OK|MB_ICONERROR);
			}
			break;
		case ID_PREVIEW0:
			if(preview0 == NULL) {
				ShowPreview(SMALL_PREVIEW);
			} else {
				UpdatePreview(SMALL_PREVIEW);
			}
			break;
		case ID_PREVIEW1:
			if(preview1 == NULL) {
				ShowPreview(BIG_PREVIEW);
			} else {
				UpdatePreview(BIG_PREVIEW);
			}
			break;
		
	}
	return TRUE;
}
void AppControl::UpdatePreview(char PreviewType) {
	int i,j;
	if(PreviewType == SMALL_PREVIEW) {
		if(preview0 == NULL) return;
	} else {
		if(preview1 == NULL) return;
	}
	if(previewBaseNif == NULL) {
		return;
	}

	map<string,string>::iterator it;
	vector<vector3> verts0;	
	vector<ushort> ZapIdx;
	Slider* slider;
	float val;
	int idx = 0;
	for(it=activeSet.TargetShapesBegin();it!=activeSet.TargetShapesEnd();++it) {
		ZapIdx.clear();
		if(!previewBaseNif->GetVertsForShape(it->second,verts0))
			continue;
		if(PreviewType == SMALL_PREVIEW) {
			for(i=0;i<sliderManager.SlidersSmall.size();i++) {
				slider = &sliderManager.SlidersSmall[i];
				val = slider->Value;
				if(slider->zap) {
					if(val>0) {
						for( j =0; j<slider->linkedDataSets.size(); j++){
							dataSets.GetDiffIndices(slider->linkedDataSets[j],it->first,ZapIdx);
						}
					}
				} else {
					if(slider->invert) 
						val = 1.0f - val;
					for( j =0; j<slider->linkedDataSets.size(); j++){
						dataSets.ApplyDiff(slider->linkedDataSets[j],it->first,val,&verts0);			
					}
				}
			}	
		
			for(i=0;i<sliderManager.SlidersSmall.size();i++) {
				slider = &sliderManager.SlidersSmall[i];
				if(slider->clamp && slider->Value>0) {
					for( j =0; j<slider->linkedDataSets.size(); j++){
						dataSets.ApplyClamp(slider->linkedDataSets[j],it->first,&verts0);			
					}		
				}
			}
				// zap deleted verts before applying to the shape
			if(ZapIdx.size() > 0) {
				for(int z =ZapIdx.size()-1; z>=0; z--) {
					verts0.erase(verts0.begin() + ZapIdx[z]);
				}
			}
			preview0->Update(it->second,&verts0);
		} else {
			for(i=0;i<sliderManager.SlidersBig.size();i++) {
				slider = &sliderManager.SlidersBig[i];
				val = slider->Value;
				if(slider->zap) {
					if(val > 0) {
						for( j =0; j<slider->linkedDataSets.size(); j++){
							dataSets.GetDiffIndices(slider->linkedDataSets[j],it->first,ZapIdx);
						}
					}
				} else {
					if(slider->invert) 
						val = 1.0f - val;
					for( j =0; j<slider->linkedDataSets.size(); j++){
						dataSets.ApplyDiff(slider->linkedDataSets[j],it->first,val,&verts0);			
					}	
				}

			}	
					
			for(i=0;i<sliderManager.SlidersBig.size();i++) {
				slider = &sliderManager.SlidersBig[i];
				if(slider->clamp && slider->Value>0) {
					for( j =0; j<slider->linkedDataSets.size(); j++){
						dataSets.ApplyClamp(slider->linkedDataSets[j],it->first,&verts0);			
					}		
				}
			}
				// zap deleted verts before applying to the shape
			if(ZapIdx.size() > 0) {
				//dataSets.GetDiffIndices(sliderManager.ZapDataLinks[0],it->first,ZapIdx);
				for(int z =ZapIdx.size()-1; z>=0; z--) {
					verts0.erase(verts0.begin() + ZapIdx[z]);
				}
			}
			preview1->Update(it->second,&verts0);
		}
		
	}


}


void AppControl::WindowResized(WORD width, WORD height) {
	MoveWindow(mBatchBuild,10,height - 40,128,25,TRUE);
	MoveWindow(mPreviewBtn,120, height - 70,128,25,TRUE);
	
	MoveWindow(mCreateButton,width/2 - 64, height - 70,128,50,TRUE);

	MoveWindow(mPreviewBtn1,width - 230, height - 70,128,25,TRUE);
	MoveWindow(mGenerateButton,width - 140, height - 40,128,25,TRUE);
	
	int ht = height - 160;
	if(ht >= 30* sliderManager.VisibleSliderCount() ) {
		ShowScrollBar(hMainWnd,SB_VERT,FALSE);
		//SetScrollRange(this->owner,SB_VERT,0,0,TRUE);
	} else {
		//SetScrollRange(this->owner,SB_VERT,0,mSliderCount * 30,TRUE);
	//	ShowScrollBar(hMainWnd,SB_VERT,TRUE);
		int range = (30*(sliderManager.VisibleSliderCount()+1)) - ht;
		SetScrollRange(hMainWnd,SB_VERT,0,range,TRUE);
	}
	sliderManager.ScrollSliders(0);


}

void AppControl::ShowPreview(char PreviewType) {
	string inputFileName;
	bool freshload = false;
	int i, j;
	inputFileName = activeSet.GetInputFileName(); // config["ShapeDataPath"];
	//inputFileName += datafolder + "\\";
	//inputFileName += inputfile;
	if(previewBaseNif == NULL) {
		previewBaseNif = new NifFile;
		previewBaseNif->Load(inputFileName);
		previewBaseName = inputFileName;
		if(PreviewMod.Load(inputFileName) != 0) {
			return;
		}
		freshload = true;
		sliderManager.FlagReload(false);
	} else if((previewBaseName != inputFileName) || sliderManager.NeedReload()) {
		delete previewBaseNif;
		previewBaseNif = new NifFile;
		previewBaseNif->Load(inputFileName);
		previewBaseName = inputFileName;
		if(PreviewMod.Load(inputFileName) != 0) {
			return;
		}
		freshload = true;
		sliderManager.FlagReload(false);
	} 
	map<string,string>::iterator it;
	vector<vector3> verts0;	
	vector<ushort> ZapIdx;
	Slider* slider;
	float val;
	for(it=activeSet.TargetShapesBegin();it!=activeSet.TargetShapesEnd();++it) {
		ZapIdx.clear();
		if(!previewBaseNif->GetVertsForShape(it->second,verts0))
			continue;		
		if(PreviewType == SMALL_PREVIEW) {
			for(i=0;i<sliderManager.SlidersSmall.size();i++) {
				slider = &sliderManager.SlidersSmall[i];
				val = slider->Value;
				if(slider->zap) {
					if(val>0) {
						for( j =0; j<slider->linkedDataSets.size(); j++){
							dataSets.GetDiffIndices(slider->linkedDataSets[j],it->first,ZapIdx);
						}
					}
				} else {
					
					if(slider->invert) 
						val = 1.0f - val;
					for( j =0; j<slider->linkedDataSets.size(); j++){
						dataSets.ApplyDiff(slider->linkedDataSets[j],it->first,val,&verts0);			
					}		
				}
			}		
			for(i=0;i<sliderManager.SlidersSmall.size();i++) {
				slider = &sliderManager.SlidersSmall[i];
				if(slider->clamp && slider->Value>0) {
					for( j =0; j<slider->linkedDataSets.size(); j++){
						dataSets.ApplyClamp(slider->linkedDataSets[j],it->first,&verts0);			
					}		
				}
			}	
		} else {
			for(i=0;i<sliderManager.SlidersBig.size();i++) {
				slider = &sliderManager.SlidersBig[i];
				val = slider->Value;
				
				if(slider->zap) {
					if(val>0){
						for( j =0; j<slider->linkedDataSets.size(); j++){
							dataSets.GetDiffIndices(slider->linkedDataSets[j],it->first,ZapIdx);
						}
					}
				} else {					
					if(slider->invert) 
						val = 1.0f - val;
					for( j =0; j<slider->linkedDataSets.size(); j++){
						dataSets.ApplyDiff(slider->linkedDataSets[j],it->first,val,&verts0);			
					}		
				}
			}		
			for(i=0;i<sliderManager.SlidersBig.size();i++) {
				slider = &sliderManager.SlidersBig[i];
				if(slider->clamp && slider->Value>0) {
					for( j =0; j<slider->linkedDataSets.size(); j++){
						dataSets.ApplyClamp(slider->linkedDataSets[j],it->first,&verts0);			
					}		
				}
			}
		}
		
			// Zap deleted verts before preview
		if(freshload && ZapIdx.size() > 0) {
			// Freshly loaded, need to actually delete verts and tris in the modified .nif
			PreviewMod.SetVertsForShape(it->second,verts0);
			//dataSets.GetDiffIndices(sliderManager.ZapDataLinks[0],it->first,ZapIdx);
			PreviewMod.DeleteVertsForShape(it->second,ZapIdx);
		} else if (ZapIdx.size() > 0) {
			// Preview Window has been opened for this shape before, zap the diff verts before applying them to the shape
			//dataSets.GetDiffIndices(sliderManager.ZapDataLinks[0],it->first,ZapIdx);
			for(int z =ZapIdx.size()-1; z>=0; z--) {
				verts0.erase(verts0.begin() + ZapIdx[z]);
			}
			PreviewMod.SetVertsForShape(it->second,verts0);
		} else {
			// No zapping needed -- just show all the verts.  
			PreviewMod.SetVertsForShape(it->second,verts0);

		}

	}

	if(PreviewType == SMALL_PREVIEW)
		preview0 = new PreviewWindow(hMainWnd,&PreviewMod,SMALL_PREVIEW);
	else 
		preview1 = new PreviewWindow(hMainWnd,&PreviewMod,BIG_PREVIEW);
}

int AppControl::saveDiffData(const string& filename, map<int,vector3>& data) {
	fstream outFile(filename.c_str(),ios_base::out|ios_base::binary|ios_base::trunc);
	if(!outFile.is_open()) {
		return 1;
	}
	map<int,vector3>::iterator it;
	int sz = data.size();
	outFile.write((char*)&sz,4);
	for(it = data.begin();it!=data.end();++it) {
		outFile.write((char*)&it->first,sizeof(int));
		outFile.write((char*)&it->second,sizeof(vector3));
	}
	outFile.close();
	return 0;
}

int AppControl::loadDiffData(const string& filename, map<int,vector3>& data) {
	fstream inFile(filename.c_str(),ios_base::in|ios_base::binary);
	if(!inFile.is_open()) {
		return 1;
	}
	data.clear();
	int sz;
	int idx;
	vector3 v;
	inFile.read((char*)&sz,4);
	for(int i = 0;i<sz;i++) {
		inFile.read((char*)&idx,sizeof(int));
		inFile.read((char*)&v,sizeof(vector3));
		data[idx] = v;
	}
	inFile.close();
	return 0;

}


int AppControl::BuildAllInGroup(const string& groupName, bool dirPrompt) {
	int ret;
	char buffer[1024];
	HCURSOR hc;
	string presetName;
	string outfitSave;
	string gamedatapathSave;
	vector<string> outfits;

	
	PresetSaveInfo promptInfo;
	LoadAllGroups(promptInfo.in_out_groups);

	ret = DialogBoxParamA(hAppInstance,MAKEINTRESOURCE(IDD_BATCHGROUPCHOICE),hMainWnd,ChooseBatchGroup,(LPARAM)&promptInfo);
	if(ret != IDOK) 
		return -1;

	if(dirPrompt) {
		gamedatapathSave = config["SkyrimDataPath"];
		BROWSEINFO bi;
		char buffer[MAX_PATH];
		bi.hwndOwner = hMainWnd;
		bi.lpszTitle = "Chooose directory to save nif files";
		bi.pidlRoot = NULL;
		bi.lParam=NULL;
		bi.iImage=NULL;
		bi.lpfn =NULL;
		bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_UAHINT;
		bi.pszDisplayName = buffer;
		LPITEMIDLIST pItemIDList = SHBrowseForFolder(&bi);
		if(!pItemIDList) 
			return 1 ;
		
		SHGetPathFromIDList(pItemIDList,buffer);
		IMalloc* pMalloc = NULL;
		if (SHGetMalloc(&pMalloc) != NOERROR)
		{
			//return 3;
		} else {
			pMalloc->Free(pItemIDList);
			pMalloc->Release();
		}
		config["SkyrimDataPath"] = buffer;
		config["SkyrimDataPath"].append("\\");
	}

	//_snprintf(buffer,1024,"Apply current preset to all members of group '%s'?",groupName.c_str());
	//ret = MessageBox(hMainWnd,buffer,"Build Multiple Outfits",MB_YESNO | MB_ICONQUESTION);
	//if(ret== IDNO) 
	//	return -1;

	GetWindowTextA(mPreSetsList,buffer,256);
	presetName = buffer;

	GetWindowTextA(mOutfitList,buffer,256);
	outfitSave = buffer;

	hc = SetCursor(LoadCursor(NULL,IDC_WAIT));

	for(int j =0;j<promptInfo.in_out_groups.size();j++ ) {
		GetGroupMembers(promptInfo.in_out_groups[j], outfits);
	}

	for(int i=0;i<outfits.size();i++) {

		//targetShapeNames.clear();
		//targetDataFolders.clear();
		sliderManager.ClearSliders();
		//outfitNames.clear();
		//sliderManager.ClearPresets();
		//LoadGroups(buffer);
		//LoadPresets(buffer);
		//PopulatePresetList();
		createSetSliders(outfits[i], true);
		sliderManager.InitializeSliders(presetName);
		//SendMessage(hMainWnd,WM_SETREDRAW,TRUE,0);
		//RedrawWindow(hMainWnd,NULL,NULL,RDW_ERASE|RDW_INVALIDATE);

		ret = BuildBodies();
		if(ret) {
			setupOutfit(outfitSave);
			SetCursor(hc);
			if(dirPrompt) 
				config["SkyrimDataPath"] = gamedatapathSave;
			return ret;
		}
	}
	setupOutfit(outfitSave);
	SetCursor(hc);
	if(dirPrompt) 
		config["SkyrimDataPath"] = gamedatapathSave;
	return 0;
}


int AppControl::BuildBodies(bool localpath) {
	string inputFileName;
	NifFile input0;
	NifFile input1;
	string outfilename0;
	string outfilename1;
	char msg[256];
	char buffer[1024];
	int i; int j;
	int ret;
	inputFileName = activeSet.GetInputFileName();
	//inputFileName = config["ShapeDataPath"];
	//inputFileName += datafolder + "\\";
	//inputFileName += inputfile;
	if(input0.Load(inputFileName) != 0) {
		return 1;
	}
	if(input1.Load(inputFileName) != 0) {
		return 1;
	}
	map<string,string>::iterator it;
	vector<vector3> verts0;	
	vector<vector3> verts1;	
	vector<ushort> ZapIndices;
	Slider* slider;
	float val;
	for(it=activeSet.TargetShapesBegin();it!=activeSet.TargetShapesEnd();++it) {
		if(!input0.GetVertsForShape(it->second,verts0))
			continue;		
		if(!input1.GetVertsForShape(it->second,verts1))
			continue;

		for(i=0;i<sliderManager.SlidersSmall.size();i++) {
			slider = &sliderManager.SlidersSmall[i];
			if(slider->zap) continue;
			if(slider->clamp) continue;
			val = slider->Value;
			if(slider->invert) 
				val = 1.0f - val;
			for( j =0; j<slider->linkedDataSets.size(); j++){
				dataSets.ApplyDiff(slider->linkedDataSets[j],it->first,val,&verts0);			
			}		
		}	
		for(i=0;i<sliderManager.SlidersSmall.size();i++) {
			slider = &sliderManager.SlidersSmall[i];
			if(slider->clamp && slider->Value>0) {
				for( j =0; j<slider->linkedDataSets.size(); j++){
					dataSets.ApplyClamp(slider->linkedDataSets[j],it->first,&verts0);			
				}	
			}
		}	
		
		for(i=0;i<sliderManager.SlidersBig.size();i++) {
			slider = &sliderManager.SlidersBig[i];
			if(slider->zap) continue;
			if(slider->clamp) continue;
			val = slider->Value;
			if(slider->invert) 
				val = 1.0f - val;
			for( j =0; j<slider->linkedDataSets.size(); j++){
				dataSets.ApplyDiff(slider->linkedDataSets[j],it->first,val,&verts1);			
			}		
		}		
		for(i=0;i<sliderManager.SlidersBig.size();i++) {
			slider = &sliderManager.SlidersBig[i];
			if(slider->clamp && slider->Value>0) {
				for( j =0; j<slider->linkedDataSets.size(); j++){
					dataSets.ApplyClamp(slider->linkedDataSets[j],it->first,&verts1);			
				}		
			}
		}


		input0.SetVertsForShape(it->second,verts0);
		input1.SetVertsForShape(it->second,verts1);

		ZapIndices.clear();
		for(i=0;i<sliderManager.SlidersBig.size();i++) {
			slider = &sliderManager.SlidersBig[i];
			if(!slider->zap) continue;
			if(slider->Value == 0) continue;
			for( j =0; j<slider->linkedDataSets.size(); j++){
				dataSets.GetDiffIndices(slider->linkedDataSets[j],it->first, ZapIndices);
			}

		}
		input1.DeleteVertsForShape(it->second,ZapIndices);
		input0.DeleteVertsForShape(it->second,ZapIndices);
	}
	
	if(localpath ) {
		outfilename0 = outfilename1 = activeSet.GetOutputFile(); //outputfile;	
	} else {
		if(config.find("SkyrimDataPath") == config.end() || config["SkyrimDataPath"] == "") {	
			if(config["WarnMissingSkyrimPath"] == "true") {
				ret = MessageBox(hMainWnd,"Warning: Skyrim data path not configured. Continue saving files? \r\n(Files will be saved in the BodySlide\\meshes directory)", "Skyrim not found",MB_YESNO | MB_ICONQUESTION);
				if(ret == IDNO) {
					return 4;
				}
			}
			GetCurrentDirectory(1024,buffer);
			config["SkyrimDataPath"] = buffer;
			config["SkyrimDataPath"] += "\\";
		}
		outfilename0= config["SkyrimDataPath"] + activeSet.GetOutputFilePath();//outputpath + "\\" + outputfile;
		outfilename1 = outfilename0;
		string tmp = config["SkyrimDataPath"] + activeSet.GetOutputPath(); //outputpath;
		SHCreateDirectoryEx(NULL,tmp.c_str(), NULL);
	}

	outfilename0 += "_0.nif";
	outfilename1 += "_1.nif";

	_snprintf(buffer,1024,outfilename0.c_str());
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.hwndOwner = this->hMainWnd;
	ofn.lpstrFilter = "Nif Files\0*.nif\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	//ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR;
	ofn.lStructSize = sizeof(ofn);
	string custName= outfilename0;
	string message;
	int err;
	int pos;
	bool useCustName = false;

	if(input0.Save(outfilename0)) {
		do {
			message = "Unable to save bodies in the following location: \r\n";
			message += custName;
			_snprintf(msg,256,"Unable to save. (error %d)",GetLastError());
			MessageBox(hMainWnd,message.c_str(),msg,MB_OK | MB_ICONERROR);	
			if(!::GetSaveFileName(&ofn)) 
				return 4;
			custName = ofn.lpstrFile;
			useCustName = true;
		} while (input0.Save(custName));
	}
	pos = custName.rfind("_0.nif");
	custName = custName.substr(0,pos) + "_1.nif";

	if(!useCustName) {
		outfilename1 = custName;
	}
	if(input1.Save(outfilename1)) {
		do {
			_snprintf(buffer,1024,outfilename1.c_str());
			message = "Unable to save bodies in the following location: \r\n";
			message += custName;
			_snprintf(msg,256,"Unable to save. (error %d)",GetLastError());
			MessageBox(hMainWnd,message.c_str(),msg,MB_OK | MB_ICONERROR);	
			if(!::GetSaveFileName(&ofn)) 
				return 4;
			custName = ofn.lpstrFile;
		} while (input1.Save(custName));
	}
	return 0;
}


int AppControl::SaveSliderPositions() {

	PresetSaveInfo promptInfo;
	LoadAllGroups(promptInfo.in_out_groups);

	int ret = DialogBoxParamA(hAppInstance,MAKEINTRESOURCE(IDD_PRESETINPUT),hMainWnd,SimpleInput,(LPARAM)&promptInfo);
	if(ret != IDOK) 
		return 0;


	string outfilename = "SliderPresets\\";
	outfilename += promptInfo.promptResult + ".xml";
	char buffer[1024];
	_snprintf(buffer,1024,outfilename.c_str());
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.hwndOwner = this->hMainWnd;
	ofn.lpstrFilter = "Preset Files\0*.xml\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	//ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	ofn.lStructSize = sizeof(ofn);
	if(!::GetSaveFileName(&ofn)) 
		return 0;

	vector<string> smallSliders;
	vector<string> bigSliders;
	sliderManager.GetSmallSliderList(smallSliders);
	sliderManager.GetBigSliderList(bigSliders);

	TiXmlDocument outdoc;
	TiXmlNode* slidersNode;	
	TiXmlElement* presetElem;
	TiXmlElement* tmpElem;
	TiXmlElement* sliderElem;
	float val;

	if(outdoc.LoadFile(ofn.lpstrFile)) {
		// file exists -- merge data
		slidersNode = outdoc.FirstChild("SliderPresets");
		presetElem = (TiXmlElement*) slidersNode->FirstChildElement("Preset");
		while (presetElem) {
			// replace preset if found in file.
			if(_stricmp(presetElem->Attribute("name"),promptInfo.promptResult.c_str()) == 0) {
				tmpElem = presetElem;
				presetElem = presetElem->NextSiblingElement("Preset");
				slidersNode->RemoveChild(tmpElem);
			} else {
				presetElem = presetElem->NextSiblingElement("Preset");
			}
		}	
	} else {
		slidersNode = outdoc.InsertEndChild(TiXmlElement("SliderPresets"));
	}	
	presetElem = slidersNode->InsertEndChild(TiXmlElement("Preset"))->ToElement();
	presetElem->SetAttribute("name",promptInfo.promptResult.c_str());
	presetElem->SetAttribute("set",curOutfit.c_str());
	for(int i =0;i<promptInfo.in_out_groups.size();i++ ){
		sliderElem = presetElem->InsertEndChild(TiXmlElement("Group"))->ToElement();
		sliderElem->SetAttribute("name",promptInfo.in_out_groups[i].c_str());
	}
	for(int i = 0;i<bigSliders.size();i++) {
		if(sliderManager.SliderHasChanged(bigSliders[i],true)) {
			sliderElem = presetElem->InsertEndChild(TiXmlElement("SetSlider"))->ToElement();
			sliderElem->SetAttribute("name",bigSliders[i].c_str());
			sliderElem->SetAttribute("size","big");
			val = sliderManager.SliderValue(bigSliders[i],true) * 100;
			sliderElem->SetAttribute("value",val);
		}
		if(sliderManager.SliderHasChanged(bigSliders[i],false)) {
			sliderElem = presetElem->InsertEndChild(TiXmlElement("SetSlider"))->ToElement();
			sliderElem->SetAttribute("name",bigSliders[i].c_str());
			sliderElem->SetAttribute("size","small");
			val = sliderManager.SliderValue(bigSliders[i],false) * 100;
			sliderElem->SetAttribute("value",val);
		}
	}
	if(!outdoc.SaveFile())
		return outdoc.ErrorId();

	
	SendMessage(mPreSetsList,CB_RESETCONTENT,0,0);
	sliderManager.ClearPresets();
	LoadPresets(curOutfit);
	PopulatePresetList();
	SendMessage(mPreSetsList,CB_SELECTSTRING,0,(LPARAM)promptInfo.promptResult.c_str());
	return 0;
}

int AppControl::BlenderNifToSkyrim() {
	fstream base;
	fstream mod;
//	istringstream iss;
	char buffer[1024];
	
	NifFile nif;
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.hwndOwner = this->hMainWnd;
	ofn.lpstrFilter = "Nif Files\0*.nif\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;	

	ofn.lStructSize = sizeof(ofn);
	if(!::GetOpenFileName(&ofn)) 
		return 0;

	nif.Load(string(ofn.lpstrFile));

	nif.DeleteBlockByType(string("NiSpecularProperty"));
	nif.DeleteBlockByType(string("BSShaderPPLightingProperty"));
	nif.DeleteBlockByType(string("BSShaderTextureSet"));
	nif.DeleteBlockByType(string("NiMaterialProperty"));
	nif.DeleteBlockByType(string("NiStencilProperty"));
	nif.DeleteBlock(0);
	nif.SetUserVersions(12,83);

	nif.Save("TestClean.nif");

	return 0;
}


void AppControl::MakeDiffFile(string shapesFile, string baseShapeName, string outDiffPath) {
	fstream base;
	fstream mod;
//	istringstream iss;
	char buffer[1024];
	vector <vector3> baseVerts;
	vector <vector3> vertOrder;
	vector <vector3> modVerts;
	map <int,vector3> diffVerts;
	
	NifFile nif;
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(OPENFILENAME));
	ofn.hwndOwner = this->hMainWnd;
	ofn.lpstrFilter = "Nif Files\0*.nif\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	

	ofn.lStructSize = sizeof(ofn);
	if(!::GetOpenFileName(&ofn)) 
		return;
	string f = ofn.lpstrFile;
	
	if(nif.Load(f)) {
		MessageBox(NULL,"Unable to open shapekey file", "Open Failed.",MB_OK | MB_ICONASTERISK);
		return;
	}
	string shapePath = ".\\GenerateOutput\\";//config["ShapeDataPath"];
	string shapeFile;
	GetCurrentDirectory(1024,buffer);
	string pathcreate = buffer;
	pathcreate += "\\GenerateOutput";
		SHCreateDirectoryEx(NULL,pathcreate.c_str(),NULL);

	vector<string> list;
	nif.GetShapeList(list);
	for(int i=0;i<list.size();i++) {
		if(list[i].compare(baseShapeName) !=0 ){
			shapeFile = shapePath + list[i] + ".bsd";
			if(!nif.ShapeDiff(baseShapeName, list[i], diffVerts)) 
				saveDiffData(shapeFile,diffVerts);

		}
	}
	return;
}
void  AppControl::MakeDiffFromObj(string baseShapeName, string objFile) {
	fstream base;
    vector3 v;
	vector2 uv;
	triangle tri;

	string dump;
	string facept1;
	string facept2;
	string facept3;
	string facept4;
	int f[4];
	int ft[4];
	vector<vector3> verts;
	vector<vector2> uvs;
	vector<triangle> tris;
	size_t pos;
	map<int,int> vertOrder;

	objFile = "D:\\proj\\skyrim\\dragonscale_skimp\\triModObj.obj";
	
	base.open(objFile.c_str(),ios_base::in | ios_base::binary);
	if(base.fail()) 
		return;
	while(!base.eof()) {
		base >> dump;
		if(dump.compare("v") == 0) {
			base >> v.x >> v.y >> v.z;
			v.x*=10.0f;
			v.y*=10.0f;
			v.z*=10.0f;
			verts.push_back(v);
		} else if (dump.compare("vt") == 0) {		
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
			if(f[0] == -1 || f[1] == -1 || f[2] == -1) {
				continue;
			}	
			vertOrder[ft[0]] = f[0];
			vertOrder[ft[1]] = f[1];
			vertOrder[ft[2]] = f[2];
			tri.p1 = ft[0];
			tri.p2 = ft[1];
			tri.p3 = ft[2];
			tris.push_back(tri);

			if(facept4 =="f") {
				base.putback('f');
			} else if(facept4.length() > 0) {
				pos = facept4.find('/');
				if(pos == string.npos) 
					continue;
				f[3] = atoi(facept4.c_str())-1;
				ft[3]=atoi(facept4.substr(pos+1).c_str())-1;
				vertOrder[ft[3]] = f[3];
				tri.p1 = ft[0];
				tri.p2 = ft[2];
				tri.p3 = ft[3];
				tris.push_back(tri);
			}
		}
	}
	base.close();

	vector<vector3> OutVerts;

	for(int i=0; i< vertOrder.size();i++) {
		OutVerts.push_back(verts[vertOrder[i]]);
	}

	string inputFileName;
	NifFile input0;
	string outfilename0;
	char msg[256];
	int i; int j;
	inputFileName = activeSet.GetInputFileName();//config["ShapeDataPath"];
	//inputFileName += datafolder + "\\";
	//inputFileName += inputfile;
	if(input0.Load(inputFileName) != 0) {
		return ;
	}
	input0.SetVertsForShape("BaseShape",OutVerts);

	input0.Save("ObjModified.nif");

	//mod.open(modFile.c_str(),ios_base::in | ios_base::binary);
	//if(mod.fail()) 
	//	return;
	//while(!mod.eof()) {
	//	mod >> dump;
	//	if(dump.compare("v") == 0) {
	//		mod >> v.x >> v.y >> v.z;
	//		modVerts.push_back(v);
	//	}
	//}
	//mod.close();

	//if(baseVerts.size() != modVerts.size()) {
	//	return;
	//}
	//
	//for(int i = 0; i< baseVerts.size(); i++) {
	//	if(baseVerts[i].x != modVerts[i].x ||
	//		baseVerts[i].y != modVerts[i].y ||
	//		baseVerts[i].z != modVerts[i].z) {
	//			v.x = modVerts[i].x-baseVerts[i].x;
	//			v.y = modVerts[i].y-baseVerts[i].y;
	//			v.z = modVerts[i].z-baseVerts[i].z;
	//			diffVerts[i] = v;
	//	}

	//}

	return;

}


// Message handler for about box.
INT_PTR CALLBACK SimpleInput(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];
	static PresetSaveInfo* s = NULL;
	HWND listview;
	LVITEM lvi;
	LVCOLUMN lvc;
	int i, n;
	switch (message)
	{
	case WM_INITDIALOG:
		s = (PresetSaveInfo*) lParam;
		::SetDlgItemText(hDlg,IDC_PRESETNAMEINPUT,"CustomPreset");
		SendDlgItemMessage(hDlg,IDC_PRESETNAMEINPUT,EM_SETSEL,0,-1);
		SendDlgItemMessage(hDlg,IDC_GROUPSEL,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_CHECKBOXES);

		listview = GetDlgItem(hDlg,IDC_GROUPSEL);

		LVCOLUMN lvc;
		lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.iSubItem = 0;
		lvc.pszText = "Groups";
		lvc.cx = 248;
		ListView_InsertColumn(listview,0,&lvc);

		lvi.mask = LVIF_TEXT;
		lvi.iSubItem = 0;
		for(i=0;i<s->in_out_groups.size();i++) {
			lvi.pszText = (LPSTR) s->in_out_groups[i].c_str();
			lvi.iItem = 0;
			ListView_InsertItem(listview,&lvi);
		}
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hDlg, LOWORD(wParam));
			::GetDlgItemText(hDlg, IDC_PRESETNAMEINPUT, buf,256);
			s->promptResult.assign(buf);
			
			listview = GetDlgItem(hDlg,IDC_GROUPSEL);

			n = ListView_GetItemCount(listview);
			s->in_out_groups.clear();
			for(i=0;i<n;i++) {
				if(ListView_GetCheckState(listview,i)) {
					ListView_GetItemText(listview,i,0,buf,256);
					s->in_out_groups.push_back(buf);
				}
			}

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




// Message handler for about box.
INT_PTR CALLBACK ChooseBatchGroup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[256];
	static PresetSaveInfo* s = NULL;
	HWND listview;
	LVITEM lvi;
	LVCOLUMN lvc;
	int i, n;
	switch (message)
	{
	case WM_INITDIALOG:
		s = (PresetSaveInfo*) lParam;
		//::SetDlgItemText(hDlg,IDC_PRESETNAMEINPUT,"CustomPreset");
		//SendDlgItemMessage(hDlg,IDC_PRESETNAMEINPUT,EM_SETSEL,0,-1);
		SendDlgItemMessage(hDlg,IDC_GROUPSEL,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_CHECKBOXES);

		listview = GetDlgItem(hDlg,IDC_GROUPSEL);

		LVCOLUMN lvc;
		lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.iSubItem = 0;
		lvc.pszText = "Groups";
		lvc.cx = 248;
		ListView_InsertColumn(listview,0,&lvc);

		lvi.mask = LVIF_TEXT;
		lvi.iSubItem = 0;
		for(i=0;i<s->in_out_groups.size();i++) {
			lvi.pszText = (LPSTR) s->in_out_groups[i].c_str();
			lvi.iItem = 0;
			ListView_InsertItem(listview,&lvi);
		}
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hDlg, LOWORD(wParam));
			//::GetDlgItemText(hDlg, IDC_PRESETNAMEINPUT, buf,256);
			//s->promptResult.assign(buf);
			
			listview = GetDlgItem(hDlg,IDC_GROUPSEL);

			n = ListView_GetItemCount(listview);
			s->in_out_groups.clear();
			for(i=0;i<n;i++) {
				if(ListView_GetCheckState(listview,i)) {
					ListView_GetItemText(listview,i,0,buf,256);
					s->in_out_groups.push_back(buf);
				}
			}

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
