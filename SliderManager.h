#pragma once

#include "stdafx.h"
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include "niffile.h"
#include "diffdata.h"
#include "Commctrl.h"
#include "SliderData.h"

using namespace std;
//
//class Vert {
//public:
//	double x;
//	double y;
//	double z;
//};
//LRESULT CALLBACK SliderEditControlTabHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


class SliderNotifyTrigger {
public:
	float triggerValue;
	unsigned short triggerType;
	string targetSlider;
//	HWND targetSlider;
};

class Slider {
public:	
	//HWND hWnd;
	//HWND readoutWnd;
	//HWND labelWnd;
	string Name;
	bool invert;
	bool zap;
	bool clamp;
	bool uv;
	bool changed;
	float defValue;
	float Value;
	vector<string> linkedDataSets;
	vector<SliderNotifyTrigger> triggers;


	float Get() {
		return Value;
	}
	void Set(float val/*, bool scrollwnd = false*/) {
		Value = val;
		/*stringstream ss;
		ss << val * 100 << "%";
		SendMessage(readoutWnd,WM_SETTEXT,0,(LPARAM)ss.str().c_str());
		if(scrollwnd) {
			SendMessage(hWnd,TBM_SETPOS,TRUE,val*100);
		}*/
		DoTrigger();
	}
	void DoTrigger() {
		for(int i=0;i<triggers.size();i++) {
			if(triggers[i].triggerValue == Value) {
			//	ShowWindow(triggers[i].targetSlider, SW_SHOW);				
			} else { 
			//	ShowWindow(triggers[i].targetSlider, SW_HIDE);
			}
		} 

	}
};

class SliderManager
{
	//HWND owner;
	//RECT ownerStartRect;
	//RECT SliderClipRect;
	//HINSTANCE ownerInst;
	//HFONT font;
	//int baseCtlId;
	int mSliderCount;
	PresetCollection presetCollection;

	bool bNeedReload;

public:
	vector<Slider> SlidersBig;
	vector<Slider> SlidersSmall;

	map <int, int> sliderEdits;  // ControlID -> slider index.  
//	map<string, string> ZapDataLinks;


	SliderManager();
	/*
	SliderManager(HWND ownerhWnd, HINSTANCE ownerhInst, HFONT appfont, int basectlID = 500);
	*/
	/*
	void SetOwnerInfo(HWND ownerhWnd, HINSTANCE ownerhInst, HFONT appfont, int basectlID = 500) {
		owner = ownerhWnd;
		GetWindowRect(owner,&ownerStartRect);
		GetWindowRect(owner,&SliderClipRect);
		SliderClipRect.top += 75;
		SliderClipRect.bottom -= 90;
		ownerInst=ownerhInst;
		baseCtlId = basectlID;
		font = appfont;
	}*/

	~SliderManager(void);

	void ClearSliders() {
		/*
		for(int i=0;i<SlidersBig.size();i++ ){
			if(SlidersBig[i].hWnd != 0) {
				DestroyWindow(SlidersBig[i].hWnd);
				DestroyWindow(SlidersBig[i].readoutWnd);
				DestroyWindow(SlidersBig[i].labelWnd);
			}
		}

		for(int i=0;i<SlidersSmall.size();i++ ){
			if(SlidersSmall[i].hWnd != 0) {
				DestroyWindow(SlidersSmall[i].hWnd);
				DestroyWindow(SlidersSmall[i].readoutWnd);
				DestroyWindow(SlidersSmall[i].labelWnd);
			}
		}*/

		SlidersBig.clear();
		SlidersSmall.clear();

		mSliderCount = 0;
	}

	bool LoadPresets(const string& basePath, const string& sliderSet, vector<string>& groupFilters) {
		return presetCollection.LoadPresets(basePath, sliderSet, groupFilters);
	}

	int SavePreset(const string& filePath, const string& presetName, const string& sliderSetName, vector<string>& assignGroups) {
		int index = 0;
		for(auto s:SlidersBig) {
			if(SliderHasChanged(s.Name,true)) {
				presetCollection.SetSliderPreset(presetName,s.Name,s.Value);
			}
			if(SliderHasChanged(s.Name,false)) {
				presetCollection.SetSliderPreset(presetName,SlidersSmall[index].Name,-10000.0f, SlidersSmall[index].Value);
			}
			index ++;
		}
		return presetCollection.SavePreset(filePath, presetName, sliderSetName, assignGroups);
	}

	void SetSliderPreset(const string& presetName, const string& slider, float big = -1, float small = -1) {
		presetCollection.SetSliderPreset(presetName,slider,big,small);
	}

	float GetBigPresetValue(const string& presetName, const string& sliderName, float defVal = 0.0f);
	float GetSmallPresetValue(const string& presetName, const string& sliderName, float defVal = 0.0f);

	void GetPresetNames(vector<string>& outNames) {
		presetCollection.GetPresetNames(outNames);
	}

	void ClearPresets() {
		presetCollection.Clear();
	}

	void AddSlidersInSet(SliderSet& inSet, bool hideAll = false);

	void AddSlider(const string& name, bool invert = false, const string& dataSetName = "");
	void AddHiddenSlider(const string& name, bool invert = false, bool isZap=false, bool isUV = false, const string& dataSetName = "");
	void AddZapSlider(const string& name, const string& dataSetName = "");
	void AddUVSlider(const string& name, bool invert = false, const string& dataSetName = "");
	void SetSliderDefaults(const string& slider, float bigVal, float smallVal);
	void SetClampSlider(const string& slider);
	void AddSliderLink(const string& slider, const string& dataSetName);
	void AddSliderTrigger(const string& slider, const string& targetSlider, float triggerVal, unsigned int triggerType);
	
	float GetSlider(const string& slider, bool isSmall);
	void SetSlider(const string& slider, bool isSmall, float val);
	void SetChanged(const string& slider, bool isSmall);

	void InitializeSliders(const string& presetName = "");
 
	bool SliderHasChanged(const string& slider, bool getBig);
	float SliderValue(const string& slider, bool getBig);

	void FlagReload(bool needReload) {
		bNeedReload = needReload;
	}
	bool NeedReload() { return bNeedReload; } 

	void GetSmallSliderList(vector<string>& names);
	void GetBigSliderList(vector<string>& names);
	int VisibleSliderCount() { return mSliderCount; }
};